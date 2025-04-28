#include "headers.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("running: %s", argv[1]);
    for (int i = 2; i < argc; i++) {
        printf(" %s", argv[i]);
    }
    printf("\n");

    // Actual command execution logic would go here

    return 0;
}