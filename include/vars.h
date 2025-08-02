#ifndef VARS_H
#define VARS_H

#include <stdlib.h>

void set_variable(const char *name, const char *value);
const char *get_variable(const char *name);
void free_variables(void);

#endif // VARS_H