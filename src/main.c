#include "libcapsule/libcapsule.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "init") == 0) {
        if (argc < 3) {
            fprintf(stderr, "[Init:%ld] Error: 'init' phase requires a command argument.\n", (long)getpid());
            exit(EXIT_FAILURE);
        }
        exit(init_container(argv[0], &argv[2]));
    }
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        fprintf(stderr, "Example: %s /bin/bash\n", argv[0]);
        fprintf(stderr, "Example: %s /bin/sh -c \"echo Container PID is \\$\\$; hostname\"\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    libcapsule_container_state_t state; // Holds container PID, exit status, stack ptr

    printf("[Parent] Creating container for command:");
    for (int i = 1; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
    if (create_container_argv(&argv[1], &state) != 0) {
        fprintf(stderr, "[Parent] Failed to create container.\n");
        exit(EXIT_FAILURE);
    }

    printf("[Parent] Waiting for container (Host PID %d)...\n", state.host_pid);
    if (wait_container(&state) != 0) {
        fprintf(stderr, "[Parent] Failed to wait for container.\n");
        exit(EXIT_FAILURE);
    }

    printf("[Parent] Container exited with status %d\n", state.exit_status);
    return state.exit_status;
}