// config.c - Configuration file handling for ash shell
#include "ash.h"
#include <sys/stat.h>
#include <stdbool.h>

#define ASH_CONFIG_PATH "/.config/ash.conf"

// Reads config value by key, returns true/false for boolean keys
bool ash_get_config_bool(const char *homedir, const char *key, bool default_value) {
    char path[ASH_MAX_PATH];
    snprintf(path, sizeof(path), "%s%s", homedir, ASH_CONFIG_PATH);
    FILE *f = fopen(path, "r");
    if (!f) return default_value;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = 0;
        if (strcmp(line, key) == 0) {
            char *val = eq + 1;
            if (strncmp(val, "true", 4) == 0) { fclose(f); return true; }
            if (strncmp(val, "false", 5) == 0) { fclose(f); return false; }
        }
    }
    fclose(f);
    return default_value;
}

// Creates config file with first_time=false
void ash_create_config(const char *homedir) {
    char path[ASH_MAX_PATH];
    snprintf(path, sizeof(path), "%s%s", homedir, ASH_CONFIG_PATH);
    struct stat st;
    if (stat(path, &st) == 0) return; // Already exists
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "first_time=false\nhide_icon=false\n");
    fclose(f);
}