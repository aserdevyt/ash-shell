
#include "../include/git.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>

/**
 * @brief Checks if the current directory is a git repository.
 * @param path The current working directory.
 * @return True if a git repository is found, false otherwise.
 */
bool is_git_repo(const char *path) {
    char git_path[ASH_MAX_PATH];
    snprintf(git_path, sizeof(git_path), "%s/.git", path);
    struct stat st;
    return stat(git_path, &st) == 0;
}

/**
 * @brief Gets the current git branch name using 'git rev-parse'.
 *
 * This function forks and executes `git rev-parse --abbrev-ref HEAD` and
 * reads the output from a pipe. It returns a dynamically allocated string
 * with the branch name or an empty string if no branch is found.
 *
 * @param path The current working directory.
 * @param buffer The buffer to store the branch name.
 * @param buffer_size The size of the buffer.
 */
void get_git_branch(const char *path, char *buffer, size_t buffer_size) {
    // Check if the current directory is a Git repository
    if (!is_git_repo(path)) {
        buffer[0] = '\0';
        return;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("ash: pipe failed");
        buffer[0] = '\0';
        return;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("ash: fork failed");
        buffer[0] = '\0';
        return;
    }

    if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        // Change directory to the repository path
        chdir(path);
        
        // Execute the git command
        execlp("git", "git", "rev-parse", "--abbrev-ref", "HEAD", (char *)NULL);
        // If execlp returns, an error occurred
        perror("ash: execlp failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        close(pipefd[1]); // Close write end
        
        char git_output[128] = "";
        ssize_t bytes_read = read(pipefd[0], git_output, sizeof(git_output) - 1);
        if (bytes_read > 0) {
            git_output[bytes_read] = '\0';
            // Trim newline characters
            git_output[strcspn(git_output, "\n")] = '\0';
            snprintf(buffer, buffer_size, " %s", git_output); // Add a git icon
        } else {
            buffer[0] = '\0';
        }
        
        close(pipefd[0]);
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }
}