// commands.c - Command list management for ash shell
// Handles building and freeing the dynamic command list for tab completion

#include "ash.h"

char **commands = NULL;
size_t commands_count = 0;

// Build the command list from $PATH and ~/.ashrc PATH+=
void build_command_list(const char *homedir) {
    char search_path[4096] = "/bin:/usr/bin:";
    if (homedir) {
        strncat(search_path, homedir, sizeof(search_path) - strlen(search_path) - 1);
        strncat(search_path, "/.local/bin", sizeof(search_path) - strlen(search_path) - 1);
    }
    char ashrc_path[ASH_MAX_PATH];
    snprintf(ashrc_path, sizeof(ashrc_path), "%s/.ashrc", homedir ? homedir : ".");
    FILE *ashrc = fopen(ashrc_path, "r");
    if (ashrc) {
        char line[1024];
        while (fgets(line, sizeof(line), ashrc)) {
            if (strncmp(line, "PATH+=:", 7) == 0) {
                char *extra = line + 7;
                extra[strcspn(extra, "\n")] = 0;
                strncat(search_path, ":", sizeof(search_path) - strlen(search_path) - 1);
                strncat(search_path, extra, sizeof(search_path) - strlen(search_path) - 1);
            }
        }
        fclose(ashrc);
    }
    size_t cap = 256;
    commands = malloc(cap * sizeof(char *));
    if (!commands) {
        fprintf(stderr, "ash: memory allocation failed\n");
        exit(1);
    }
    commands_count = 0;
    char *path = strdup(search_path);
    if (!path) {
        fprintf(stderr, "ash: memory allocation failed\n");
        exit(1);
    }
    char *saveptr = NULL;
    char *dir = strtok_r(path, ":", &saveptr);
    while (dir) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *entry;
            while ((entry = readdir(d))) {
                if (entry->d_type == DT_REG || entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN) {
                    int exists = 0;
                    for (size_t i = 0; i < commands_count; ++i) {
                        if (strcmp(commands[i], entry->d_name) == 0) { exists = 1; break; }
                    }
                    if (!exists) {
                        if (commands_count + 1 >= cap) {
                            cap *= 2;
                            char **tmp = realloc(commands, cap * sizeof(char *));
                            if (!tmp) {
                                fprintf(stderr, "ash: memory allocation failed\n");
                                closedir(d);
                                free(path);
                                exit(1);
                            }
                            commands = tmp;
                        }
                        commands[commands_count] = strdup(entry->d_name);
                        if (!commands[commands_count]) {
                            fprintf(stderr, "ash: memory allocation failed\n");
                            closedir(d);
                            free(path);
                            exit(1);
                        }
                        commands_count++;
                    }
                }
            }
            closedir(d);
        }
        dir = strtok_r(NULL, ":", &saveptr);
    }
    free(path);
    if (commands_count + 1 >= cap) {
        cap++;
        char **tmp = realloc(commands, cap * sizeof(char *));
        if (!tmp) {
            fprintf(stderr, "ash: memory allocation failed\n");
            exit(1);
        }
        commands = tmp;
    }
    commands[commands_count] = NULL;
}

// Free the command list
void free_commands(void) {
    for (size_t i = 0; i < commands_count; ++i) {
        free(commands[i]);
    }
    free(commands);
    commands = NULL;
    commands_count = 0;
}