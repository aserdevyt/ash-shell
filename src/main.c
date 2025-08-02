// main.c - Entry point for ash shell
// Implements main loop, built-ins, and shell logic

#define _POSIX_C_SOURCE 200809L
#include "ash.h"
#include <signal.h>
#include <time.h>

int main() {
    // Get home directory
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw ? pw->pw_dir : NULL;
    // Ensure ~/.ashrc exists
    ensure_ashrc(homedir);
    // Ensure ~/.config/ash.conf exists and handle first time logic
    ash_create_config(homedir);
    if (!ash_get_config_bool(homedir, "first_time", false)) {
        printf("\033[1;36mWelcome to ash!\033[0m\n\n");
        printf("This is your first time running ash.\n");
        printf("this project is in early development.\n");
        printf("please report any issues you encounter.\n");
        printf("make sure to update this shell from the aur or github.\n");
        printf("fell free to contribute in https://github.com/aserdevyt/ash-shell.\n");
        printf("Your shell is now configurable via ~/.config/ash.conf\n");
        printf("Edit this file to change options like hiding the OS icon.\n\n");
        // Set first_time to true for future runs
        char conf_path[ASH_MAX_PATH];
        snprintf(conf_path, sizeof(conf_path), "%s/.config/ash.conf", homedir);
        FILE *f = fopen(conf_path, "r+");
        if (f) {
            char buf[1024];
            size_t len = fread(buf, 1, sizeof(buf)-1, f);
            buf[len] = 0;
            rewind(f);
            char *ft = strstr(buf, "first_time=false");
            if (ft) {
                strcpy(ft, "first_time=true");
                fwrite(buf, 1, strlen(buf), f);
            }
            fclose(f);
        }
    }
    // Build command list for tab completion
    build_command_list(homedir);
    // Ignore SIGINT in shell
    signal(SIGINT, SIG_IGN);
    // Detect Linux distro for prompt icon
    char distro_icon[16] = "󰻀"; // Default icon
    FILE *os_release = fopen("/etc/os-release", "r");
    if (!os_release) {
        fprintf(stderr, "ash: could not open /etc/os-release\n");
    } else {
        char line[256];
        while (fgets(line, sizeof(line), os_release)) {
            if (strstr(line, "ID=")) {
                char *id = strchr(line, '=');
                if (id) {
                    id++;
                    char *nl = strchr(id, '\n'); if (nl) *nl = 0;
                    if (*id == '"') id++;
                    char *endq = strchr(id, '"'); if (endq) *endq = 0;
                    if (strcmp(id, "arch") == 0) strcpy(distro_icon, "󰣇");
                    else if (strcmp(id, "debian") == 0) strcpy(distro_icon, "");
                    else if (strcmp(id, "ubuntu") == 0) strcpy(distro_icon, "");
                    else if (strcmp(id, "linuxmint") == 0) strcpy(distro_icon, "󰣭");
                    else if (strcmp(id, "gentoo") == 0) strcpy(distro_icon, "");
                    break;
                }
            }
        }
        fclose(os_release);
    }
    // Set up tab completion
    rl_attempted_completion_function = NULL; // TODO: add completion function
    // Run commands from ~/.ashrc
    run_ashrc(homedir);
    // Load aliases
    load_aliases(homedir);
    // History file path
    char hist_path[ASH_MAX_PATH];
    snprintf(hist_path, sizeof(hist_path), "%s/.ashhistory", homedir ? homedir : ".");
    read_history(hist_path);
    char cwd[ASH_MAX_PATH];
    char *username = pw ? pw->pw_name : "unknown";
    int is_root = (geteuid() == 0);
    while (1) {
        static int last_status = 0;
        static double last_time = 0.0;
        if (!getcwd(cwd, sizeof(cwd))) {
            fprintf(stderr, "ash: getcwd failed\n");
            break;
        }
        char display_dir[ASH_MAX_PATH];
        if (homedir && strstr(cwd, homedir) == cwd) {
            char *subdir = cwd + strlen(homedir);
            if (subdir[0] == '/') subdir++;
            if (strlen(subdir) > 0)
                snprintf(display_dir, sizeof(display_dir), "~/%s", subdir);
            else
                snprintf(display_dir, sizeof(display_dir), "~");
        } else {
            snprintf(display_dir, sizeof(display_dir), "%s", cwd);
        }
        char prompt[ASH_PROMPT_SIZE];
        // Check config for hide_icon
        bool hide_icon = ash_get_config_bool(homedir, "hide_icon", false);
        char icon[16];
        strcpy(icon, hide_icon ? "" : distro_icon);
        print_prompt(icon, display_dir, prompt);
        char *input = readline(prompt);
        // Built-in clear
        if (input && strcmp(input, "clear") == 0) {
            printf("\033[2J\033[H");
            free(input);
            continue;
        }
        // Built-in history
        if (input && strcmp(input, "history") == 0) {
            HIST_ENTRY **hist = history_list();
            if (hist) {
                for (int i = 0; hist[i]; ++i) {
                    printf("%d  %s\n", i + history_base, hist[i]->line);
                }
            }
            free(input);
            continue;
        }
        // Built-in version
        if (input && strcmp(input, "version") == 0) {
            printf("ash shell version 1.0\n");
            free(input);
            continue;
        }
        // Show last exit status
        if (input && strcmp(input, "status") == 0) {
            printf("Last exit status: %d\n", last_status);
            printf("Last command time: %.3f seconds\n", last_time);
            free(input);
            continue;
        }
        // Environment variable export
        if (input && strncmp(input, "export ", 7) == 0) {
            char *eq = strchr(input + 7, '=');
            if (eq) {
                *eq = 0;
                setenv(input + 7, eq + 1, 1);
            }
            free(input);
            continue;
        }
        // Aliases
        for (int i = 0; input && i < alias_count; ++i) {
            if (strncmp(input, aliases[i], strlen(aliases[i])) == 0 && (input[strlen(aliases[i])] == ' ' || input[strlen(aliases[i])] == '\0')) {
                size_t newlen = strlen(alias_cmds[i]) + strlen(input) + 2;
                char *newcmd = malloc(newlen);
                if (!newcmd) {
                    fprintf(stderr, "ash: memory allocation failed\n");
                    free(input);
                    input = NULL;
                    break;
                }
                snprintf(newcmd, newlen, "%s%s", alias_cmds[i], input + strlen(aliases[i]));
                free(input);
                input = newcmd;
                break;
            }
        }
        // Syntax highlighting
        if (input && strlen(input) > 0) {
            syntax_highlight(input);
        }
        if (!input) break;
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        add_history(input);
        append_history(1, hist_path);
        if (strcmp(input, "exit") == 0) {
            free(input);
            break;
        }
        // Built-in help
        if (strcmp(input, "help") == 0) {
            printf("\n\033[1;36mWelcome to ash!\033[0m\n\n");
            printf("Features:\n");
            printf("- Customizable prompt with Linux distro icon and current directory\n");
            printf("- Tab completion for all executables in /bin, /usr/bin, ~/.local/bin, and custom paths via PATH+= in ~/.ashrc\n");
            printf("- Command history saved to ~/.ashhistory\n");
            printf("- Built-in cd command\n");
            printf("- Syntax highlighting for recognized commands\n");
            printf("- Ctrl+C only terminates running commands, not the shell\n");
            printf("- Runs commands from ~/.ashrc at startup\n");
            printf("- More features coming soon!\n");
            printf("\nType 'exit' to quit.\n\n");
            free(input);
            continue;
        }
        // Built-in cd
        if (strncmp(input, "cd", 2) == 0 && (input[2] == ' ' || input[2] == '\0')) {
            char *path = input + 2;
            while (*path == ' ') path++;
            handle_cd(path);
            free(input);
            continue;
        }
        // Background job support
        int run_bg = 0;
        size_t inlen = strlen(input);
        if (inlen > 0 && input[inlen - 1] == '&') {
            run_bg = 1;
            input[inlen - 1] = 0;
            while (inlen > 1 && input[inlen - 2] == ' ') { input[inlen - 2] = 0; inlen--; }
        }
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "ash: fork failed\n");
            free(input);
            continue;
        }
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            execl("/bin/sh", "sh", "-c", input, (char *)NULL);
            fprintf(stderr, "ash: exec failed\n");
            exit(1);
        } else if (pid > 0) {
            int status = 0;
            if (!run_bg) {
                if (waitpid(pid, &status, 0) < 0) {
                    fprintf(stderr, "ash: waitpid failed\n");
                }
                last_status = WIFEXITED(status) ? WEXITSTATUS(status) : status;
                clock_gettime(CLOCK_MONOTONIC, &t1);
                last_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
                if (last_status != 0) {
                    printf("\033[1;31m[error] Command exited with status %d\033[0m\n", last_status);
                }
            } else {
                printf("[background job started: pid %d]\n", pid);
            }
        }
        free(input);
    }
    // Save history on exit
    write_history(hist_path);
    free_aliases();
    free_commands();
    return 0;
}
