#include "../include/builtins.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int handle_cd(const char *path) {
    if (!path || path[0] == '\0') {
        // No path, go to HOME
        const char *home = getenv("HOME");
        if (home) return chdir(home);
        fprintf(stderr, "cd: HOME not set\n");
        return -1;
    }
    if (chdir(path) != 0) {
        perror("cd");
        return -1;
    }
    return 0;
}
