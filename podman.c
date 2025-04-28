#include "headers.h" 
#define STACK_SIZE (1024 * 1024) // 1MB stack
#define CONTAINER_HOSTNAME "container-1"

static int container_init(char *program_name, char *const command_argv[]);
static int child_shim_func(void *arg);

static int child_shim_func(void *arg) {
    char **original_argv = (char **)arg;
    int original_argc = 0;
    while (original_argv[original_argc] != NULL) {
        original_argc++;
    }

    /*  We need to construct a new argv for the self-exec:
        ["/proc/self/exe", "init", original_argv[1], original_argv[2], ..., NULL]
        Size needed: 1 (exe) + 1 (init) + (original_argc - 1) (command+args) + 1 (NULL) = original_argc + 2
    */
    
    char *new_argv[original_argc + 2];

    new_argv[0] = "/proc/self/exe";
    new_argv[1] = "init";

    // Copy the original command and its arguments (starting from original_argv[1])
    for (int i = 1; i < original_argc; i++) {
        new_argv[i + 1] = original_argv[i];
    }
    new_argv[original_argc + 1] = NULL; // Null-terminate the new argv array

    // --- Shim Process Function (runs immediately after clone) ---
    printf("[Shim:%ld] Parent PID: %ld. Executing 'init' phase via /proc/self/exe...\n", (long)getpid(), (long)getppid());
    printf("[Shim:%ld] New argv for execve: ", (long)getpid());
    for (int i = 0; new_argv[i] != NULL; i++) {
        printf("'%s' ", new_argv[i]);
    }
    printf("\n");

    // Ensure PATH might be inherited or set default if needed
    extern char **environ; // Use the environment from the parent initially

    execve("/proc/self/exe", new_argv, environ);

    // If execve returns, it's an error
    perror("[Shim] execve failed");
    _exit(EXIT_FAILURE);
}

// --- Container Init Function (runs after self-exec, called from main) ---
// program_name is argv[0] ("/proc/self/exe"), command_argv is &argv[2] from main
static int container_init(char *program_name, char *const command_argv[]) {
    // Because of CLONE_NEWPID, this process should have PID 1 within its namespace
    printf("[Init:PID %ld] Running container_init (My Parent PID according to namespace: %ld)\n", (long)getpid(), (long)getppid());

    // --- UTS Namespace Setup ---
    printf("[Init:PID %ld] Setting hostname to '%s'...\n", (long)getpid(), CONTAINER_HOSTNAME);
    if (sethostname(CONTAINER_HOSTNAME, strlen(CONTAINER_HOSTNAME)) == -1) {
        perror("[Init] sethostname failed");
        return EXIT_FAILURE;
    }
    printf("[Init:PID %ld] Hostname set.\n", (long)getpid());

    // --- Demonstrate PID Namespace ---
    printf("[Init:PID %ld] My PID inside the namespace should be 1. Let's check with getpid(): %ld\n", (long)getpid(), (long)getpid());
    printf("[Init:PID %ld] My Parent PID inside the namespace should be 0 (no parent in this namespace). Let's check getppid(): %ld\n", (long)getpid(), (long)getppid());
    printf("[Init:PID %ld] Now executing the final command: %s\n", (long)getpid(), command_argv[0]);
    printf("[Init:PID %ld] Note: If you run 'ps' or similar, you might still see host processes because /proc is not isolated yet!\n", (long)getpid());

    // --- Execute the final user process ---
    // command_argv[0] is the command, command_argv is the full argv array for it.
    execvp(command_argv[0], command_argv);

    // If execvp returns, it failed
    perror("[Init] final execvp failed");
    return EXIT_FAILURE;
}


// --- Main Function ---
int main(int argc, char *argv[]) {
    /*
    Now introduces PID namespace isolation.
    - The init process inside the container will report PID 1.
    - Running 'ps' inside the container might still show host processes.
      This is because 'ps' reads from /proc, and we haven't isolated
      the filesystem view (Mount Namespace) or mounted a new /proc yet.

    To build and run:
    gcc podman.c -o podman -Wall -Werror -std=gnu99
    sudo ./podman.o /bin/sh -c "echo Shell PID: \$\$; ps aux | head -n 5; sleep 10"
    Or to get an interactive shell:
    sudo ./podman.o /bin/bash
    (Inside bash, run `echo $$` and `ps aux`)

    Observe:
    1. The "[Init:PID 1]" messages.
    2. The PID reported by `echo $$` inside the container's shell (should be 1).
    3. The output of `ps aux`, showing many processes from the host. Can you guess,
    why ps does not show the container processes ids?
    */

    // --- Check if we are in the "init" phase ---
    if (argc > 1 && strcmp(argv[1], "init") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[Init:%ld] Error: 'init' requires a command to run.\n", (long)getpid());
            exit(EXIT_FAILURE);
        }
        exit(container_init(argv[0], &argv[2]));
    }

    // --- Otherwise, we are in the "create" phase (parent) ---
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        fprintf(stderr, "Example: %s /bin/sh -c \"echo Shell PID: \\$\\$; ps aux | head -n 5\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    void *child_stack = NULL;
    void *stack_top = NULL;
    pid_t child_pid;
    int status;

    printf("[Parent:%ld] Starting container setup (create phase)...\n", (long)getpid());
    printf("[Parent:%ld] Command to run in container: ", (long)getpid());
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    // Allocate stack for the child process
    child_stack = malloc(STACK_SIZE);
    if (child_stack == NULL) {
        perror("[Parent] Failed to allocate stack");
        exit(EXIT_FAILURE);
    }
    stack_top = child_stack + STACK_SIZE; // Point to the top of the stack
    printf("[Parent:%ld] Allocated %d bytes stack for child.\n", (long)getpid(), STACK_SIZE);

    // Flags for clone: New UTS + New PID namespace + Send SIGCHLD to parent on termination
    // <<< CHANGE: Added CLONE_NEWPID >>>
    int clone_flags = CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD;
    // <<< CHANGE: Updated log message >>>
    printf("[Parent:%ld] Calling clone() with flags: CLONE_NEWUTS | CLONE_NEWPID | SIGCHLD\n", (long)getpid());

    // Pass the original argv to the shim function via the 'arg' parameter
    child_pid = clone(child_shim_func, stack_top, clone_flags, argv);

    if (child_pid == -1) {
        perror("[Parent] clone failed");
        free(child_stack); // Clean up stack on failure
        exit(EXIT_FAILURE);
    }
    printf("[Parent:%ld] Cloned shim process with Host PID: %ld\n", (long)getpid(), (long)child_pid);

    // --- Parent Waits ---
    printf("[Parent:%ld] Waiting for container process (Host PID %ld) to exit...\n", (long)getpid(), (long)child_pid);
    if (waitpid(child_pid, &status, 0) == -1) {
        perror("[Parent] waitpid failed");
        // Still try to free stack
    } else {
        if (WIFEXITED(status)) {
            printf("[Parent:%ld] Container process exited normally with status: %d\n", (long)getpid(), WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Parent:%ld] Container process killed by signal: %d\n", (long)getpid(), WTERMSIG(status));
        } else {
            printf("[Parent:%ld] Container process exited with unknown status.\n", (long)getpid());
        }
    }

    // Clean up
    free(child_stack);
    printf("[Parent:%ld] Exiting.\n", (long)getpid());
    return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE; // Return child's exit status or failure
}