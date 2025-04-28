#include "headers.h"

int main(int argc, char *argv[]) {

    /*
     * Attempting to mimic docker exec: directly executing the provided command.
     * WARNING: This does NOT provide any container isolation. Running arbitrary
     * commands without proper sandboxing or namespaces is dangerous.
     */
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("running: %s", argv[1]);
    for (int i = 2; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");
    
    execvp(argv[1], argv + 1);
    perror("execvp failed");
    exit(EXIT_FAILURE);

    return 0;
}