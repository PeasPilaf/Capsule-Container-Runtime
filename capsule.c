#include "headers.h" // Make sure sys/wait.h and unistd.h are included via headers.h

int main(int argc, char *argv[]) {
    pid_t child_pid;
    int status;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("[Parent:%ld] Forking process to run: ", (long)getpid());
    for (int i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");

    child_pid = fork();

    if (child_pid == -1) {
        // Fork failed
        perror("[Parent] fork failed");
        exit(EXIT_FAILURE);
    } else if (child_pid == 0) {
        // --- Child Process ---
        printf("[Child:%ld] Executing command: %s\n", (long)getpid(), argv[1]);

        // Prepare arguments for execvp: argv + 1 points to the command itself
        char **child_argv = &argv[1];

        execvp(child_argv[0], child_argv);

        // If execvp returns, an error occurred
        perror("[Child] execvp failed");
        // Exit the child process with failure status
        // _exit() is often preferred over exit() after fork without exec
        // to avoid running stdio cleanup routines twice.
        _exit(EXIT_FAILURE);

    } else {
        // --- Parent Process ---
        printf("[Parent:%ld] Waiting for child process (PID: %ld) to finish...\n", (long)getpid(), (long)child_pid);

        // Wait for the specific child process to change state
        if (waitpid(child_pid, &status, 0) == -1) {
            perror("[Parent] waitpid failed");
            exit(EXIT_FAILURE);
        }

        // Check how the child terminated
        if (WIFEXITED(status)) {
            printf("[Parent:%ld] Child process (PID: %ld) exited normally with status: %d\n",
                   (long)getpid(), (long)child_pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Parent:%ld] Child process (PID: %ld) killed by signal: %d\n",
                   (long)getpid(), (long)child_pid, WTERMSIG(status));
        } else {
            printf("[Parent:%ld] Child process (PID: %ld) exited with unknown status.\n",
                   (long)getpid(), (long)child_pid);
        }

        printf("[Parent:%ld] Exiting.\n", (long)getpid());
        exit(EXIT_SUCCESS); // Exit parent successfully
    }

    // This line should not be reached
    return 0;
}