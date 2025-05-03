#ifndef LIBCAPSULE_H
#define LIBCAPSULE_H

#include <sys/types.h>

/*
clone()
 * The clone() system call is used to create a new process (or thread) in Linux.
 * It allows for fine-grained control over what resources are shared between the
parent and child processes.
 * The flags used in the clone() call determine the behavior of the new process.
 * For example, CLONE_NEWPID creates a new PID namespace, while CLONE_NEWNS
creates a new mount namespace.
 *
 * The child process can then perform operations like changing the root
filesystem and setting up its environment.
 * The parent process must wait for the child to finish its setup before
proceeding.

 * The clone() syscall does not inherit the stack, like fork() does, so one must
be assigned to the child process.
*/

#define STACK_SIZE (1024 * 1024) // 1MB
#define CONTAINER_HOSTNAME "container-1"
#define NEW_ROOT_PATH "/tmp/mycontainer_root" // Path to the prepared rootfs

typedef struct libcapsule_container_state
{
    pid_t host_pid;
    int exit_status;
    void* child_stack;
} libcapsule_container_state_t;

// Create container with raw argv (advanced)
// argv[0] = command, argv[1..N] = args, NULL-terminated
int
create_container_argv(char* const argv[], libcapsule_container_state_t* state);

// Create container from single shell command (simpler API)
int
create_container_simple(const char* command,
                        libcapsule_container_state_t* state);

// Init phase (called inside container)
int
init_container(const char* program_name, char* const command_argv[]);

// Wait for container process to exit
int
wait_container(libcapsule_container_state_t* state);

#endif // LIBCAPSULE_H