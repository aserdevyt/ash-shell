// ash.h - Main shell header
// Contains global declarations and utility prototypes for ash shell

#ifndef ASH_H
#define ASH_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

#define ASH_MAX_ALIASES 128
#define ASH_MAX_COMMANDS 4096
#define ASH_MAX_PATH 1024
#define ASH_PROMPT_SIZE 2048

extern char **commands;
extern size_t commands_count;
extern char *aliases[ASH_MAX_ALIASES];
extern char *alias_cmds[ASH_MAX_ALIASES];
extern int alias_count;

void build_command_list(const char *homedir);
void load_aliases(const char *homedir);
void free_aliases(void);
void free_commands(void);
void ensure_ashrc(const char *homedir);
void run_ashrc(const char *homedir);
void print_prompt(const char *distro_icon, const char *display_dir, char *prompt);
void syntax_highlight(const char *input);
void handle_cd(const char *path);

#endif // ASH_H
