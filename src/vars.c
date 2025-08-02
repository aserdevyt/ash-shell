#include "vars.h"
#include <string.h>

#define MAX_VARS 128
#define VAR_NAME_LEN 64
#define VAR_VALUE_LEN 256

static char names[MAX_VARS][VAR_NAME_LEN];
static char values[MAX_VARS][VAR_VALUE_LEN];
static int var_count = 0;

void set_variable(const char *name, const char *value) {
    for (int i = 0; i < var_count; ++i) {
        if (strcmp(names[i], name) == 0) {
            strncpy(values[i], value, VAR_VALUE_LEN - 1);
            values[i][VAR_VALUE_LEN - 1] = '\0';
            return;
        }
    }
    if (var_count < MAX_VARS) {
        strncpy(names[var_count], name, VAR_NAME_LEN - 1);
        names[var_count][VAR_NAME_LEN - 1] = '\0';
        strncpy(values[var_count], value, VAR_VALUE_LEN - 1);
        values[var_count][VAR_VALUE_LEN - 1] = '\0';
        var_count++;
    }
}

const char *get_variable(const char *name) {
    for (int i = 0; i < var_count; ++i) {
        if (strcmp(names[i], name) == 0) {
            return values[i];
        }
    }
    return getenv(name); // Fallback to environment variables
}

void free_variables(void) {
    // No dynamic allocation in this simple example
    var_count = 0;
}