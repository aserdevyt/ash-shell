// prompt.c - Prompt and syntax highlighting for ash shell
// Handles prompt display and command syntax highlighting

#include "ash.h"

// Print the shell prompt string
void print_prompt(const char *distro_icon, const char *display_dir, char *prompt) {
    // Powerlevel10k-inspired prompt with Nerd Font icons and Unicode separators
    // Example icons: distro_icon (e.g., "\uf303" for Arch), folder ("\ue5fe"), user ("\uf007"), arrow ("\ue0b0")
    // Make sure your terminal uses a Nerd Font for proper display
    snprintf(prompt, ASH_PROMPT_SIZE,
        "\033[1;38;5;81m%s\033[0m " // Distro icon, cyan
        "\033[1;38;5;220m\uf007\033[0m " // User icon, yellow
        "\033[1;38;5;81m%s\033[0m " // Username
        "\033[1;38;5;39m\ue5fe\033[0m " // Folder icon, blue
        "\033[1;38;5;45m%s\033[0m " // Current directory, teal
        "\033[1;38;5;240m\ue0b0\033[0m ",
        distro_icon, getenv("USER"), display_dir);
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
