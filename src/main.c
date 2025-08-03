// main.c - Entry point for ash shell
// Implements main loop, built-ins, and shell logic with added variable support
// and a new execution engine that uses the built-in parser for pipelines and redirection.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <ctype.h>
#include <stdbool.h>

// Assuming these headers exist.
#include "../include/ash.h"
#include "../include/builtins.h"
#include "../include/vars.h"

//
// Parser.h Definitions (for a self-contained file)
// In a real project, this would be in include/parser.h
//

#define MAX_ARGS 64

// Enumeration for command separators
typedef enum {
    CMD_AND,    // &&
    CMD_OR,     // ||
    CMD_SEMI,   // ;
    CMD_PIPE,   // |
    CMD_BG,     // & (new for background jobs)
    CMD_END     // end of command list
} CmdType;

// Structure for a single token in a linked list
typedef struct Token {
    char *value;
    struct Token *next;
} Token;

// Structure for a linked list of tokens
typedef struct {
    Token *head;
    Token *tail;
} TokenList;

// Structure for a single command, including arguments and redirection.
typedef struct Command {
    char *argv[MAX_ARGS]; // Array of arguments for execvp
    char *redir_in;       // Input redirection file
    char *redir_out;      // Output redirection file
    bool redir_append;    // True if output redirection is '>>'
    CmdType type;         // The separator to the next command
    struct Command *next; // Pointer to the next command in a pipeline
} Command;

// Function prototypes from parser.h
TokenList tokenize(const char *input);
void free_tokens(TokenList *tokens);
Command *parse_command(TokenList *tokens);
void free_command(Command *cmd);
char* expand_variables(const char* input);

//
// End of Parser.h Definitions
//

// Global variable definition for the shell name.
char *shell_name;

// Global variables for job control.
#define MAX_JOBS 64
typedef struct {
    pid_t pid;
    char *command_line;
    int job_id;
    int status; // 0 for running, 1 for stopped, 2 for done
} Job;

Job jobs[MAX_JOBS];
int job_count = 0;
int next_job_id = 1;

// Function to add a new job to the jobs list
void add_job(pid_t pid, const char *command_line) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "ash: too many background jobs\n");
        return;
    }
    jobs[job_count].pid = pid;
    jobs[job_count].command_line = strdup(command_line);
    jobs[job_count].job_id = next_job_id++;
    jobs[job_count].status = 0; // 0 for running
    job_count++;
}

// Function to remove a job from the jobs list
void remove_job(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            free(jobs[i].command_line);
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j+1];
            }
            job_count--;
            break;
        }
    }
}

// Function to update the status of jobs.
// This is called by the SIGCHLD handler.
void update_jobs_status() {
    pid_t pid;
    int status;
    int i = 0;
    while (i < job_count) {
        pid = waitpid(jobs[i].pid, &status, WNOHANG);
        if (pid == jobs[i].pid) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                printf("\n[%d] Done %s\n", jobs[i].job_id, jobs[i].command_line);
                remove_job(jobs[i].job_id);
            } else if (WIFSTOPPED(status)) {
                // Not implemented in this version, but a placeholder
                printf("\n[%d] Stopped %s\n", jobs[i].job_id, jobs[i].command_line);
                jobs[i].status = 1;
                i++;
            }
        } else {
            i++;
        }
    }
}

// Signal handler for SIGCHLD to prevent zombie processes.
void handle_sigchld(int sig) {
    update_jobs_status();
}

// Built-in function prototypes (forward declarations)
int builtin_cd(char *path);
void builtin_exit();
void builtin_history();
void builtin_help();
void builtin_clear();
void builtin_version();
void builtin_status(int last_status, double last_time);
void builtin_jobs();
void builtin_fg(int job_id);
void builtin_bg(int job_id);
bool is_builtin(const char *cmd);
int execute_builtin(Command *cmd, int input_fd, int output_fd, int last_status, double last_time);

// Checks if a command is a built-in.
bool is_builtin(const char *cmd) {
    return (strcmp(cmd, "cd") == 0 ||
            strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "history") == 0 ||
            strcmp(cmd, "help") == 0 ||
            strcmp(cmd, "clear") == 0 ||
            strcmp(cmd, "version") == 0 ||
            strcmp(cmd, "status") == 0 ||
            strcmp(cmd, "jobs") == 0 ||
            strcmp(cmd, "fg") == 0 ||
            strcmp(cmd, "bg") == 0);
}

// Executes a built-in command with optional I/O redirection.
int execute_builtin(Command *cmd, int input_fd, int output_fd, int last_status, double last_time) {
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    if (input_fd != STDIN_FILENO) {
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }
    if (output_fd != STDOUT_FILENO) {
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
    
    // Execute the built-in based on the command name
    if (strcmp(cmd->argv[0], "cd") == 0) {
        handle_cd(cmd->argv[1]);
    } else if (strcmp(cmd->argv[0], "exit") == 0) {
        // Exit from the shell
        free_command(cmd);
        exit(0);
    } else if (strcmp(cmd->argv[0], "history") == 0) {
        HIST_ENTRY **hist = history_list();
        if (hist) {
            for (int i = 0; hist[i]; ++i) {
                printf("%d  %s\n", i + history_base, hist[i]->line);
            }
        }
    } else if (strcmp(cmd->argv[0], "help") == 0) {
        builtin_help();
    } else if (strcmp(cmd->argv[0], "clear") == 0) {
        printf("\033[2J\033[H");
    } else if (strcmp(cmd->argv[0], "version") == 0) {
        printf("ash shell version 1.0\n");
    } else if (strcmp(cmd->argv[0], "status") == 0) {
        builtin_status(last_status, last_time);
    } else if (strcmp(cmd->argv[0], "jobs") == 0) {
        builtin_jobs();
    } else if (strcmp(cmd->argv[0], "fg") == 0) {
        if (cmd->argv[1]) {
            builtin_fg(atoi(cmd->argv[1]));
        } else {
            fprintf(stderr, "fg: usage: fg <job_id>\n");
        }
    } else if (strcmp(cmd->argv[0], "bg") == 0) {
        if (cmd->argv[1]) {
            builtin_bg(atoi(cmd->argv[1]));
        } else {
            fprintf(stderr, "bg: usage: bg <job_id>\n");
        }
    }

    // Restore original file descriptors
    dup2(saved_stdin, STDIN_FILENO);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdin);
    close(saved_stdout);
    
    return 0;
}

// Executes a single pipeline segment (one or more commands connected by '|')
int execute_segment(Command *head, const char *original_input) {
    Command *cmd = head;
    pid_t pid;
    int input_fd = STDIN_FILENO;
    int last_status = 0;
    
    while (cmd) {
        if (!cmd->argv[0]) {
            // If the command is empty, just move to the next one
            cmd = cmd->next;
            continue;
        }

        int pipe_fd[2];
        
        // If there's a next command AND it's a pipe, set up a new pipe
        if (cmd->next && cmd->type == CMD_PIPE) {
            if (pipe(pipe_fd) == -1) {
                perror("ash: pipe");
                return -1;
            }
        }
        
        // Check if the command is a built-in. If so, execute it in the main process.
        if (is_builtin(cmd->argv[0])) {
            last_status = execute_builtin(cmd, input_fd, STDOUT_FILENO, 0, 0.0);
            if (input_fd != STDIN_FILENO) {
                close(input_fd);
            }
            cmd = cmd->next;
            continue;
        }

        // It's not a built-in, so we must fork
        pid = fork();
        if (pid < 0) {
            perror("ash: fork failed");
            return -1;
        }
        
        if (pid == 0) {
            // Child process
            signal(SIGINT, SIG_DFL);
            
            // Redirect input if necessary
            if (input_fd != STDIN_FILENO) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
            
            // Redirect output if this is not the last command in the pipeline
            if (cmd->next && cmd->type == CMD_PIPE) {
                close(pipe_fd[0]);
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
            }
            
            // Handle I/O redirection from the command struct
            if (cmd->redir_in) {
                int fd = open(cmd->redir_in, O_RDONLY);
                if (fd == -1) {
                    perror("ash: open input file");
                    exit(1);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
            if (cmd->redir_out) {
                int flags = O_WRONLY | O_CREAT;
                flags |= (cmd->redir_append) ? O_APPEND : O_TRUNC;
                int fd = open(cmd->redir_out, flags, 0644);
                if (fd == -1) {
                    perror("ash: open output file");
                    exit(1);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            // Executing the command
            execvp(cmd->argv[0], cmd->argv);
            perror("ash");
            exit(1);
        } else {
            // Parent process
            int status;
            if (input_fd != STDIN_FILENO) {
                close(input_fd);
            }
            if (cmd->next && cmd->type == CMD_PIPE) {
                close(pipe_fd[1]);
                input_fd = pipe_fd[0];
            }
            
            // Wait for the child only if it's not a background job.
            if (cmd->type != CMD_BG) {
                waitpid(pid, &status, 0);
                last_status = WIFEXITED(status) ? WEXITSTATUS(status) : status;
            } else {
                add_job(pid, original_input);
                printf("[%d] %d\n", next_job_id-1, pid);
            }
        }
        
        if (cmd->next && cmd->type == CMD_PIPE) {
            cmd = cmd->next;
        } else {
            cmd = NULL; // End of this segment
        }
    }
    
    return last_status;
}

// The main execution function that handles command separators.
int execute_commands(Command *head, const char *original_input) {
    int last_status = 0;
    Command *current = head;

    // Check for variable assignment which is a special case.
    // Ensure it's not a command with a leading variable, like `echo $VAR=val`
    char *eq = strchr(original_input, '=');
    if (eq && (eq > original_input) && isalpha(*(eq - 1)) && (strchr(original_input, ' ') == NULL)) {
        char *key = strndup(original_input, eq - original_input);
        char *expanded_value = expand_variables(eq + 1);
        if(expanded_value) {
            set_variable(key, expanded_value);
            setenv(key, expanded_value, 1);
            free(expanded_value); // Free the dynamically allocated string
        }
        free(key);
        return 0; // Success
    }
    
    // Handle export command before parsing
    if (strncmp(original_input, "export ", 7) == 0) {
        char *eq_pos = strchr(original_input + 7, '=');
        if (eq_pos) {
            char *key = strndup(original_input + 7, eq_pos - (original_input + 7));
            char *expanded_value = expand_variables(eq_pos + 1);
            if(expanded_value) {
                set_variable(key, expanded_value);
                setenv(key, expanded_value, 1);
                free(expanded_value);
            }
            free(key);
        }
        return 0;
    }

    while (current) {
        // Before execution, sanitize the argument list if it's a background job.
        if (current->type == CMD_BG) {
            for (int i = 0; current->argv[i] != NULL; i++) {
                if (current->argv[i+1] == NULL && strcmp(current->argv[i], "&") == 0) {
                    current->argv[i] = NULL;
                    break;
                }
            }
        }
        
        last_status = execute_segment(current, original_input);
        
        // Move past the current pipeline segment
        while(current && current->next && current->type == CMD_PIPE) {
            current = current->next;
        }

        // Check logical operators
        if (current && current->type == CMD_AND) {
            if (last_status != 0) {
                // Previous command failed, skip the next one
                current = current->next;
            }
        } else if (current && current->type == CMD_OR) {
            if (last_status == 0) {
                // Previous command succeeded, skip the next one
                current = current->next;
            }
        }
        
        if (current) {
            current = current->next;
        }
    }
    
    return last_status;
}

// Built-in functions
void builtin_help() {
    printf("\n\033[1;36mWelcome to ash!\033[0m\n\n");
    printf("Features:\n");
    printf("- Customizable prompt with Linux distro icon and current directory\n");
    printf("- Tab completion for all executables in /bin, /usr/bin, ~/.local/bin, and custom paths via PATH+= in ~/.ashrc\n");
    printf("- Command history saved to ~/.ashhistory\n");
    printf("- Built-in cd command\n");
    printf("- Command separators ('&&', '||', ';', '&')\n");
    printf("- Ctrl+C only terminates running commands, not the shell\n");
    printf("- Runs commands from ~/.ashrc at startup\n");
    printf("- Basic variable assignment and substitution\n");
    printf("- Pipeline ('|') and I/O redirection ('<', '>', '>>') support\n");
    printf("- Job control with `jobs`, `fg`, and `bg`\n");
    printf("More features coming soon!\n");
    printf("\nType 'exit' to quit.\n\n");
}

void builtin_jobs() {
    for (int i = 0; i < job_count; i++) {
        const char *status_str = (jobs[i].status == 0) ? "Running" : "Stopped";
        printf("[%d] %s %s\n", jobs[i].job_id, status_str, jobs[i].command_line);
    }
}

void builtin_fg(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            if (kill(jobs[i].pid, SIGCONT) < 0) {
                perror("ash: fg");
                return;
            }
            waitpid(jobs[i].pid, NULL, WUNTRACED);
            remove_job(job_id);
            return;
        }
    }
    fprintf(stderr, "ash: fg: job not found\n");
}

void builtin_bg(int job_id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].job_id == job_id) {
            if (kill(jobs[i].pid, SIGCONT) < 0) {
                perror("ash: bg");
                return;
            }
            jobs[i].status = 0; // Mark as running
            return;
        }
    }
    fprintf(stderr, "ash: bg: job not found\n");
}

void builtin_status(int last_status, double last_time) {
    printf("Last exit status: %d\n", last_status);
    printf("Last command time: %.3f seconds\n", last_time);
}

void run_script_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("ash");
        exit(1);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        
        if (strlen(line) == 0) {
            continue;
        }
        
        // Expand variables before tokenizing
        char *expanded_line = expand_variables(line);
        if (expanded_line == NULL || strlen(expanded_line) == 0) {
            if (expanded_line) free(expanded_line);
            continue;
        }

        TokenList tokens = tokenize(expanded_line);
        Command *cmd_list = parse_command(&tokens);
        
        if (cmd_list) {
            execute_commands(cmd_list, expanded_line);
            free_command(cmd_list);
        }
        free_tokens(&tokens);
        
        if (expanded_line) free(expanded_line);
    }
    
    free(line);
    fclose(file);
    exit(0);
}

int main(int argc, char *argv[]) {
    // Check if a script file is provided as an argument
    if (argc > 1) {
        run_script_file(argv[1]);
        return 0; // The run_script_file function will handle exit
    }

    // Check if the program name is available and create a copy.
    if (argc > 0) {
        shell_name = strdup(argv[0]);
    } else {
        shell_name = strdup("ash"); // Fallback name
    }

    // Set up signal handler for background jobs.
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigchld;
    sigaction(SIGCHLD, &sa, NULL);

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
        printf("This project is in early development.\n");
        printf("Please report any issues you encounter.\n");
        printf("Make sure to update this shell from the aur or github.\n");
        printf("Feel free to contribute in https://github.com/aserdevyt/ash-shell.\n");
        printf("Your shell is now configurable via ~/.config/ash.conf\n");
        printf("Edit this file to change options like hiding the OS icon.\n\n");
        
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
    rl_attempted_completion_function = NULL;
    
    // Run commands from ~/.ashrc
    run_ashrc(homedir);
    
    // Load aliases
    load_aliases(homedir);
    
    // History file path
    char hist_path[ASH_MAX_PATH];
    snprintf(hist_path, sizeof(hist_path), "%s/.ashhistory", homedir ? homedir : ".");
    read_history(hist_path);
    
    char cwd[ASH_MAX_PATH];
    
    while (1) {
        static int last_status = 0;
        static double last_time = 0.0;
        
        // Check for terminated background jobs
        update_jobs_status();

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
        bool hide_icon = ash_get_config_bool(homedir, "hide_icon", false);
        char icon[16];
        strcpy(icon, hide_icon ? "" : distro_icon);
        print_prompt(icon, display_dir, prompt);
        
        char *input = readline(prompt);
        
        if (!input) {
            break;
        }

        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        // Add to history and save
        add_history(input);
        append_history(1, hist_path);
        
        // Expand variables and aliases before parsing
        char *expanded_input = expand_variables(input);
        if (expanded_input == NULL || strlen(expanded_input) == 0) {
            if (expanded_input) free(expanded_input);
            free(input);
            continue;
        }
        
        // Alias substitution
        char *processed_input = strdup(expanded_input);
        for (int i = 0; processed_input && i < alias_count; ++i) {
            if (strncmp(processed_input, aliases[i], strlen(aliases[i])) == 0 && (processed_input[strlen(aliases[i])] == ' ' || processed_input[strlen(aliases[i])] == '\0')) {
                size_t newlen = strlen(alias_cmds[i]) + strlen(processed_input) + 2;
                char *newcmd = malloc(newlen);
                if (!newcmd) {
                    fprintf(stderr, "ash: memory allocation failed\n");
                    free(processed_input);
                    processed_input = NULL;
                    break;
                }
                snprintf(newcmd, newlen, "%s%s", alias_cmds[i], processed_input + strlen(aliases[i]));
                free(processed_input);
                processed_input = newcmd;
                break;
            }
        }
        
        // If the alias resulted in a null command, skip
        if (!processed_input) {
            free(input);
            free(expanded_input);
            continue;
        }
        
        // --- New Execution Logic using built-in parser ---
        TokenList tokens = tokenize(processed_input);
        Command *cmd_list = parse_command(&tokens);
        
        if (cmd_list) {
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            
            last_status = execute_commands(cmd_list, processed_input);
            
            clock_gettime(CLOCK_MONOTONIC, &t1);
            last_time = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            
            if (last_status != 0) {
                printf("\033[1;31m[error] Command exited with status %d\033[0m\n", last_status);
            }
            
            free_command(cmd_list);
        }
        free_tokens(&tokens);
        
        if (processed_input) free(processed_input);
        if (expanded_input) free(expanded_input);
        free(input);
    }
    
    // Save history on exit
    write_history(hist_path);
    free_aliases();
    free_commands();
    free_variables();
    
    if (shell_name) { // Free the allocated memory for shell_name
        free(shell_name);
    }
    
    return 0;
}
