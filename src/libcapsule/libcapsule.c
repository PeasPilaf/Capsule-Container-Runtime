#define _GNU_SOURCE

#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/wait.h>
#include <unistd.h>

#include "libcapsule.h"

#ifndef CLONE_FLAGS
#define CLONE_FLAGS (CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD)
#endif

static int
child_shim_func(void* arg)
{
    char** original_argv = (char**)arg;
    int original_argc = 0;
    while (original_argv[original_argc] != NULL) {
        original_argc++;
    }

    char* new_argv[original_argc + 3];
    new_argv[0] = "/proc/self/exe";
    new_argv[1] = "init";
    for (int i = 0; i < original_argc; i++) {
        new_argv[i + 2] = original_argv[i];
    }
    new_argv[original_argc + 2] = NULL;

    extern char** environ;
    execve("/proc/self/exe", new_argv, environ);

    perror("[Shim] execve failed");
    _exit(EXIT_FAILURE);
}

int
create_container_argv(char* const argv[], libcapsule_container_state_t* state)
{
    if (!argv || !argv[0] || !state) {
        errno = EINVAL;
        return -1;
    }

    state->host_pid = -1;
    state->exit_status = -1;
    state->child_stack = NULL; // Important initialization

    void* child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("[Parent] Failed to allocate stack");
        return -1;
    }

    state->child_stack = child_stack;

    void* stack_top = child_stack + STACK_SIZE;
    int clone_flags = CLONE_FLAGS;

    pid_t child_pid =
      clone(child_shim_func, stack_top, clone_flags, (void*)argv);

    if (child_pid == -1) {
        perror("[Parent] clone failed");
        free(state->child_stack);
        state->child_stack = NULL;
        return -1;
    }

    printf("[Parent] Created container process with Host PID: %ld\n",
           (long)child_pid);

    state->host_pid = child_pid;

    return 0; // Return success
}

int
create_container_simple(const char* command,
                        libcapsule_container_state_t* state)
{
    if (!command || !state) {
        errno = EINVAL;
        return -1;
    }
    char* argv[] = { "/bin/sh", "-c", (char*)command, NULL };
    return create_container_argv(argv, state);
}

int
init_container(const char* program_name, char* const command_argv[])
{
    char old_root_path[256];

    printf("[Init:PID %ld] Running inside container namespace\n",
           (long)getpid());

    // set hostname
    if (sethostname(CONTAINER_HOSTNAME, strlen(CONTAINER_HOSTNAME)) == -1) {
        perror("[Init] sethostname failed");
    }

    // make mounts private
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("[Init] Failed to make root private");
        _exit(EXIT_FAILURE);
    }

    // bind mount the new root onto itself before pivot
    if (mount(NEW_ROOT_PATH, NEW_ROOT_PATH, NULL, MS_BIND | MS_REC, NULL) ==
        -1) {
        perror("[Init] Failed to bind mount new root");
        _exit(EXIT_FAILURE);
    }

    // create directory for old root inside the new root
    snprintf(
      old_root_path, sizeof(old_root_path), "%s/old_root", NEW_ROOT_PATH);
    if (mkdir(old_root_path, 0700) == -1 && errno != EEXIST) {
        perror("[Init] Failed to create old_root directory");
        umount2(NEW_ROOT_PATH, MNT_DETACH);
        _exit(EXIT_FAILURE);
    }

    // pivot root
    if (syscall(SYS_pivot_root, NEW_ROOT_PATH, old_root_path) == -1) {
        perror("[Init] Failed to pivot_root (syscall)");
        umount2(NEW_ROOT_PATH, MNT_DETACH);
        rmdir(old_root_path);
        _exit(EXIT_FAILURE);
    }

    // change to the new root directory
    if (chdir("/") == -1) {
        perror("[Init] Failed to change directory to new root");
        umount2("/old_root", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }

    // unmount old root
    if (umount2("/old_root", MNT_DETACH) == -1) {
        perror("[Init] Warning: Failed to unmount /old_root");
        // Continue, might leave clutter
    } else {
        printf("[Init] Unmounted /old_root\n");
        if (rmdir("/old_root") == -1) {
            perror("[Init] Warning: Failed to remove /old_root directory");
        } else {
            printf("[Init] Removed /old_root directory\n");
        }
    }

    // mount proc
    if (mount(
          "proc", "/proc", "proc", MS_NOSUID | MS_NODEV | MS_NOEXEC, NULL) ==
        -1) {
        perror("[Init] Failed to mount /proc");
        _exit(EXIT_FAILURE);
    }

    // mount sys read-only
    if (mkdir("/sys", 0555) == -1 && errno != EEXIST) {
        perror("[Init] Failed to create /sys directory");
        _exit(EXIT_FAILURE);
    }
    if (mount("sysfs",
              "/sys",
              "sysfs",
              MS_RDONLY | MS_NOSUID | MS_NODEV | MS_NOEXEC,
              NULL) == -1) {
        perror("[Init] Failed to mount /sys");
        // _exit(EXIT_FAILURE);
    }
    printf("[Init] Mounted /sys.\n");

    // mount devtmpfs
    if (mkdir("/dev", 0755) == -1 && errno != EEXIST) {
        perror("[Init] Failed to create /dev directory");
        _exit(EXIT_FAILURE);
    }

    if (mount("tmpfs",
              "/dev",
              "tmpfs",
              MS_NOSUID | MS_STRICTATIME,
              "mode=0755,size=65536k") == -1) {
        perror("[Init] Failed to mount tmpfs on /dev");
        _exit(EXIT_FAILURE);
    }
    printf("[Init] Mounted tmpfs on /dev\n");

    if (mkdir("/dev/pts", 0755) == -1 && errno != EEXIST) {
        perror("[Init] Failed to create /dev/pts");
        umount2("/dev", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }
    if (mkdir("/dev/shm", 0755) == -1 && errno != EEXIST) {
        perror("[Init] Failed to create /dev/shm");
        umount2("/dev", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }

    if (mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3)) == -1 ||
        mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5)) == -1 ||
        mknod("/dev/full", S_IFCHR | 0666, makedev(1, 7)) == -1 ||
        mknod("/dev/random", S_IFCHR | 0666, makedev(1, 8)) == -1 ||
        mknod("/dev/urandom", S_IFCHR | 0666, makedev(1, 9)) == -1 ||
        mknod("/dev/tty", S_IFCHR | 0666, makedev(5, 0)) == -1 ||
        mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1)) == -1) {
        perror("[Init] Failed to create standard device nodes in /dev");
        umount2("/dev/shm", MNT_DETACH);
        umount2("/dev/pts", MNT_DETACH);
        umount2("/dev", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }
    printf("[Init] Created /dev/null, zero, full, random, urandom, tty.\n");

    if (mount("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC, NULL) ==
        -1) {
        perror("[Init] Failed to mount devpts on /dev/pts");
        umount2("/dev", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }
    printf("[Init] Mounted devpts on /dev/pts\n");

    if (mount(
          "tmpfs", "/dev/shm", "tmpfs", MS_NOSUID | MS_NODEV, "mode=1777") ==
        -1) {
        perror("[Init] Failed to mount tmpfs on /dev/shm");
        umount2("/dev/pts", MNT_DETACH);
        umount2("/dev", MNT_DETACH);
        _exit(EXIT_FAILURE);
    }
    printf("[Init] Mounted tmpfs on /dev/shm\n");

    if (symlink("/proc/self/fd/0", "/dev/stdin") == -1 && errno != EEXIST) {
        perror("[Init] Warning: Failed to create /dev/stdin symlink");
    } else {
        printf("[Init] Created /dev/stdin symlink\n");
    }

    if (symlink("/proc/self/fd/1", "/dev/stdout") == -1 && errno != EEXIST) {
        perror("[Init] Warning: Failed to create /dev/stdout symlink");
    } else {
        printf("[Init] Created /dev/stdout symlink\n");
    }

    if (symlink("/proc/self/fd/2", "/dev/stderr") == -1 && errno != EEXIST) {
        perror("[Init] Warning: Failed to create /dev/stderr symlink");
    } else {
        printf("[Init] Created /dev/stderr symlink\n");
    }

    printf("[Init:PID %ld] Executing final command: %s\n",
           (long)getpid(),
           command_argv[0]);
    execvp(command_argv[0], command_argv);

    perror("[Init] execvp failed");
    _exit(EXIT_FAILURE);
}

int
wait_container(libcapsule_container_state_t* state)
{
    if (!state ||
        state->host_pid <=
          0) { // Check host_pid is valid *before* assuming stack exists
        errno = EINVAL;
        return -1;
    }

    int status;
    pid_t result = waitpid(state->host_pid, &status, 0);

    int ret_val = 0;

    if (result == -1) {
        perror("[Parent] waitpid failed");
        ret_val = -1;
        state->exit_status = -1; // Indicate unknown exit status
    } else {
        if (WIFEXITED(status)) {
            state->exit_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            state->exit_status = 128 + WTERMSIG(status); // Common convention
        } else {
            state->exit_status = -1; // Unknown exit reason
        }
    }

    if (state->child_stack != NULL) {
        free(state->child_stack);
        state->child_stack = NULL;
    }

    return ret_val;
}