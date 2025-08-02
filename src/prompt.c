// prompt.c - Prompt and syntax highlighting for ash shell
// Handles prompt display and command syntax highlighting

#include "ash.h"

// Print the shell prompt string
void print_prompt(const char *distro_icon, const char *display_dir, char *prompt) {
    snprintf(prompt, ASH_PROMPT_SIZE,
        "\033[1;37m%s\033[0m \033[1;34m%s\033[0m \033[1;32m›\033[0m ", distro_icon, display_dir);
}

// Syntax highlight recognized commands
void syntax_highlight(const char *input) {
    char cmd[64] = {0};
    int i = 0;
    while (input[i] && !isspace(input[i]) && i < 63) {
        cmd[i] = input[i];
        i++;
    }
    cmd[i] = 0;
    for (size_t j = 0; commands[j]; ++j) {
        if (strcmp(cmd, commands[j]) == 0) {
            printf("\033[1;36m%s\033[0m%s\n", cmd, input + i);
            break;
        }
    }
}
