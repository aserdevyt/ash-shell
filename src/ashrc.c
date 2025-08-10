// ashrc.c - ~/.ashrc management for ash shell
// Ensures ~/.ashrc exists and runs its commands at startup

#include "ash.h"

// Create ~/.ashrc if it does not exist
void ensure_ashrc(const char *homedir) {
    char ashrc_path[ASH_MAX_PATH];
    snprintf(ashrc_path, sizeof(ashrc_path), "%s/.ashrc", homedir ? homedir : ".");
    FILE *ashrc = fopen(ashrc_path, "r");
    if (!ashrc) {
        ashrc = fopen(ashrc_path, "w");
        if (!ashrc) {
            fprintf(stderr, "ash: could not create %s\n", ashrc_path);
            return;
        }
        fprintf(ashrc, "# Default ashrc\n# Add your aliases and PATH extensions here\n");
        fclose(ashrc);
        printf("Created default %s\n", ashrc_path);
    } else {
        fclose(ashrc);
    }
}

// Run commands from ~/.ashrc
void run_ashrc(const char *homedir) {
    char ashrc_path[ASH_MAX_PATH];
    snprintf(ashrc_path, sizeof(ashrc_path), "%s/.ashrc", homedir ? homedir : ".");
    FILE *ashrc = fopen(ashrc_path, "r");
    if (!ashrc) {
        fprintf(stderr, "ash: could not open %s\n", ashrc_path);
        return;
    }
    char line[1024];
    while (fgets(line, sizeof(line), ashrc)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "ash: fork failed\n");
                fclose(ashrc);
                exit(1);
            }
            if (pid == 0) {
                execl("/bin/sh", "sh", "-c", line, (char *)NULL);
                fprintf(stderr, "ash: exec failed\n");
                exit(1);
            } else if (pid > 0) {
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
    fclose(ashrc);
}