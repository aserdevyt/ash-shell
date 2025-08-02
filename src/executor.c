// executor.c - Implements the logic for executing a parsed command list.

#include "../include/executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/**
 * @brief Executes a single command.
 * @param cmd The Command structure to execute.
 */
void execute_single_command(Command *cmd) {
    // Check for built-in commands like 'exit'
    if (strcmp(cmd->argv[0], "exit") == 0) {
        // Handle exit built-in
        exit(0);
    }
    
    // Check for I/O redirection and set up file descriptors
    if (cmd->redir_in) {
        int fd_in = open(cmd->redir_in, O_RDONLY);
        if (fd_in == -1) {
            perror("ash: open input file");
            exit(1);
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    
    if (cmd->redir_out) {
        int fd_out = open(cmd->redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out == -1) {
            perror("ash: open output file");
            exit(1);
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    
    // Execute the command using execvp
    execvp(cmd->argv[0], cmd->argv);
    
    // If execvp returns, an error occurred
    perror("ash: command not found");
    exit(1);
}

/**
 * @brief Executes a list of commands, handling pipes and separators.
 * @param cmd_list The head of the Command linked list.
 * @return The exit status of the last executed command.
 */
int execute_commands(Command *cmd_list) {
    if (!cmd_list || !cmd_list->argv[0]) {
        return 0; // No commands to execute
    }
    
    Command *current_cmd = cmd_list;
    pid_t pid;
    int status = 0;
    int pipe_fd[2];
    int prev_pipe_read_fd = STDIN_FILENO;
    
    while (current_cmd != NULL) {
        bool is_pipe = (current_cmd->type == CMD_PIPE);
        bool is_background = (current_cmd->type == CMD_BACKGROUND);

        if (is_pipe) {
            if (pipe(pipe_fd) == -1) {
                perror("ash: pipe failed");
                return -1;
            }
        }
        
        pid = fork();
        if (pid == -1) {
            perror("ash: fork failed");
            return -1;
        } else if (pid == 0) {
            // Child process
            
            // Set up signal handling in child
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            
            // Handle piping from previous command
            if (prev_pipe_read_fd != STDIN_FILENO) {
                dup2(prev_pipe_read_fd, STDIN_FILENO);
                close(prev_pipe_read_fd);
            }
            
            // Handle piping to next command
            if (is_pipe) {
                close(pipe_fd[0]); // Close read end of the pipe
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
            }
            
            // Execute the single command
            execute_single_command(current_cmd);
        } else {
            // Parent process
            
            // Close the previous read end of the pipe
            if (prev_pipe_read_fd != STDIN_FILENO) {
                close(prev_pipe_read_fd);
            }
            
            if (is_pipe) {
                close(pipe_fd[1]); // Close write end of the pipe
                prev_pipe_read_fd = pipe_fd[0];
            } else {
                // If not a pipe, close any remaining pipe
                prev_pipe_read_fd = STDIN_FILENO;
            }
            
            if (!is_background) {
                waitpid(pid, &status, 0);
            }
        }
        
        current_cmd = current_cmd->next;
    }
    
    // Clean up any remaining pipes
    if (prev_pipe_read_fd != STDIN_FILENO) {
        close(prev_pipe_read_fd);
    }
    
    return status;
}
