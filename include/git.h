#ifndef GIT_H
#define GIT_H

#include <stdbool.h>
#include "ash.h"

bool is_git_repo(const char *path);
void get_git_branch(const char *path, char *buffer, size_t buffer_size);

#endif // GIT_H
