// aliases.c - Alias management for ash shell
// Handles loading, freeing, and expanding aliases from ~/.ashrc

#include "ash.h"

char *aliases[ASH_MAX_ALIASES];
char *alias_cmds[ASH_MAX_ALIASES];
int alias_count = 0;

// Load aliases from ~/.ashrc
void load_aliases(const char *homedir) {
    alias_count = 0;
    char ashrc_path[ASH_MAX_PATH];
    snprintf(ashrc_path, sizeof(ashrc_path), "%s/.ashrc", homedir ? homedir : ".");
    FILE *ashrc = fopen(ashrc_path, "r");
    if (!ashrc) return;
    char line[1024];
    while (fgets(line, sizeof(line), ashrc)) {
        if (strncmp(line, "alias ", 6) == 0) {
            char *eq = strchr(line, '=');
            if (eq) {
                *eq = 0;
                char *name = line + 6;
                char *cmd = eq + 1;
                name[strcspn(name, " \t\n")] = 0;
                cmd[strcspn(cmd, "\n")] = 0;
                aliases[alias_count] = strdup(name);
                alias_cmds[alias_count] = strdup(cmd);
                if (!aliases[alias_count] || !alias_cmds[alias_count]) {
                    fprintf(stderr, "ash: memory allocation failed\n");
                    fclose(ashrc);
                    exit(1);
                }
                alias_count++;
            }
        }
    }
    fclose(ashrc);
}

// Free aliases
void free_aliases(void) {
    for (int i = 0; i < alias_count; ++i) {
        free(aliases[i]);
        free(alias_cmds[i]);
    }
    alias_count = 0;
}
