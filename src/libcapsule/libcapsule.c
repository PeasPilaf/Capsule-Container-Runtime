#define _GNU_SOURCE 

#include "libcapsule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h> 
#include <sched.h>
#include <errno.h>

#ifndef CLONE_FLAGS
#define CLONE_FLAGS (CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD)
#endif

static int child_shim_func(void *arg) {
    char **original_argv = (char **)arg;
    int original_argc = 0;
    while (original_argv[original_argc] != NULL) {
        original_argc++;
    }

    char *new_argv[original_argc + 3];
    new_argv[0] = "/proc/self/exe";
    new_argv[1] = "init";
    for (int i = 0; i < original_argc; i++) {
        new_argv[i + 2] = original_argv[i];
    }
    new_argv[original_argc + 2] = NULL;

    extern char **environ;
    execve("/proc/self/exe", new_argv, environ);

    perror("[Shim] execve failed");
    _exit(EXIT_FAILURE);
}

int create_container_argv(char *const argv[], libcapsule_container_state_t *state) {
    if (!argv || !argv[0] || !state) {
        errno = EINVAL;
        return -1;
    }

    state->host_pid = -1;
    state->exit_status = -1;
    state->child_stack = NULL; // Important initialization
    

    void *child_stack = malloc(STACK_SIZE);
    if (!child_stack) {
        perror("[Parent] Failed to allocate stack");

        return -1;
    }

    state->child_stack = child_stack; 
    
    void *stack_top = child_stack + STACK_SIZE;
    int clone_flags = CLONE_FLAGS;

    pid_t child_pid = clone(child_shim_func, stack_top, clone_flags, (void*) argv);

    if (child_pid == -1) {
        perror("[Parent] clone failed");
        free(state->child_stack);
        state->child_stack = NULL;
        return -1; 
    }

    printf("[Parent] Created container process with Host PID: %ld\n", (long)child_pid);

    state->host_pid = child_pid;
   


    return 0; // Return success
}

int create_container_simple(const char *command, libcapsule_container_state_t *state) {
    if (!command || !state) {
        errno = EINVAL;
        return -1;
    }
    char *argv[] = { "/bin/sh", "-c", (char *)command, NULL };
    return create_container_argv(argv, state);
}


int init_container(const char *program_name, char *const command_argv[]) {
    printf("[Init:PID %ld] Running inside container namespace\n", (long)getpid());

    if (sethostname(CONTAINER_HOSTNAME, strlen(CONTAINER_HOSTNAME)) == -1) {
        perror("[Init] sethostname failed");
        return EXIT_FAILURE;
    }

    printf("[Init:PID %ld] Executing final command: %s\n", (long)getpid(), command_argv[0]);
    execvp(command_argv[0], command_argv);

    perror("[Init] execvp failed");
    return EXIT_FAILURE;
}


int wait_container(libcapsule_container_state_t *state) {
    if (!state || state->host_pid <= 0) { // Check host_pid is valid *before* assuming stack exists
        errno = EINVAL;
        return -1;
    }

    int status;
    pid_t result = waitpid(state->host_pid, &status, 0);

    int ret_val = 0; // Assume success initially

    if (result == -1) {
        perror("[Parent] waitpid failed");



        ret_val = -1; // waitpid failed
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


    return ret_val; // Return 0 if waitpid succeeded, -1 if it failed
}