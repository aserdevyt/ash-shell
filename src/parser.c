// parser.c - Implements the full parsing logic for the shell.

#include "../include/parser.h"
#include "../include/vars.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/**
 * @brief Adds a new token with the given value to the token list.
 * @param list The TokenList to add the token to.
 * @param value The string value for the new token.
 */
void add_token(TokenList *list, const char *value) {
    Token *new_token = malloc(sizeof(Token));
    if (!new_token) {
        perror("ash: malloc");
        return;
    }
    new_token->value = strdup(value);
    if (!new_token->value) {
        perror("ash: strdup");
        free(new_token);
        return;
    }
    new_token->next = NULL;

    if (list->head == NULL) {
        list->head = new_token;
        list->tail = new_token;
    } else {
        list->tail->next = new_token;
        list->tail = new_token;
    }
}

/**
 * @brief Tokenizes a command line string, respecting single quotes, double quotes, and backslash escapes.
 *
 * This function uses a state machine to correctly parse the input.
 * - STATE_NORMAL: Default state, handles whitespace, special characters, and new quotes.
 * - STATE_SINGLE_QUOTE: All characters are treated as literal until a closing single quote.
 * - STATE_DOUBLE_QUOTE: Characters are literal, except for backslashes that can escape a few specific characters and the dollar sign for variable expansion.
 *
 * This improved version correctly handles command separators like '|', '&', etc., as separate tokens, even without surrounding whitespace.
 *
 * @param input The command line string to tokenize.
 * @return A TokenList containing the parsed tokens.
 */
TokenList tokenize(const char *input) {
    TokenList list = {NULL, NULL};
    if (!input) {
        return list;
    }
    
    // Create a mutable copy of the input string to work with
    char *str = strdup(input);
    if (!str) {
        perror("ash: strdup");
        return list;
    }
    
    char *p = str;
    char buffer[1024]; // A temporary buffer for building tokens
    int buf_idx = 0;

    enum {
        STATE_NORMAL,
        STATE_SINGLE_QUOTE,
        STATE_DOUBLE_QUOTE
    } state = STATE_NORMAL;

    while (*p) {
        if (state == STATE_NORMAL) {
            // Handle whitespace as a token separator
            if (isspace(*p)) {
                if (buf_idx > 0) {
                    buffer[buf_idx] = '\0';
                    add_token(&list, buffer);
                    buf_idx = 0;
                }
            } else if (*p == '\'') {
                // Enter single quote state
                if (buf_idx > 0) {
                    buffer[buf_idx] = '\0';
                    add_token(&list, buffer);
                    buf_idx = 0;
                }
                state = STATE_SINGLE_QUOTE;
            } else if (*p == '\"') {
                // Enter double quote state
                if (buf_idx > 0) {
                    buffer[buf_idx] = '\0';
                    add_token(&list, buffer);
                    buf_idx = 0;
                }
                state = STATE_DOUBLE_QUOTE;
            } else if (*p == '\\') {
                // Handle backslash escape - append to current buffer
                p++; // Move past the backslash
                if (*p) { // If there's a character to escape, add it
                    if (buf_idx < sizeof(buffer) - 1) {
                         buffer[buf_idx++] = *p;
                    }
                }
            } else if (*p == '|' || *p == '<' || *p == '>' || *p == '&' || *p == ';') {
                // Handle special characters as separate tokens
                if (buf_idx > 0) {
                    buffer[buf_idx] = '\0';
                    add_token(&list, buffer);
                    buf_idx = 0;
                }
                
                // Handle multi-character operators like '&&' and '||'
                if ((*p == '&' && *(p+1) == '&') || (*p == '|' && *(p+1) == '|')) {
                    buffer[0] = *p;
                    buffer[1] = *(p+1);
                    buffer[2] = '\0';
                    add_token(&list, buffer);
                    p++; // Move past the second character of the operator
                } else {
                    buffer[0] = *p;
                    buffer[1] = '\0';
                    add_token(&list, buffer);
                }
            } else {
                // Append regular characters to the token buffer
                if (buf_idx < sizeof(buffer) - 1) {
                    buffer[buf_idx++] = *p;
                }
            }
        } else if (state == STATE_SINGLE_QUOTE) {
            if (*p == '\'') {
                // Exit single quote state
                state = STATE_NORMAL;
            } else {
                // Add all characters literally
                if (buf_idx < sizeof(buffer) - 1) {
                    buffer[buf_idx++] = *p;
                }
            }
        } else if (state == STATE_DOUBLE_QUOTE) {
            if (*p == '\"') {
                // Exit double quote state
                state = STATE_NORMAL;
            } else if (*p == '\\' && (*(p+1) == '\"' || *(p+1) == '$' || *(p+1) == '`' || *(p+1) == '\\')) {
                // Handle specific backslash escapes within double quotes
                p++;
                if (buf_idx < sizeof(buffer) - 1) {
                    buffer[buf_idx++] = *p;
                }
            } else {
                // Add all other characters
                if (buf_idx < sizeof(buffer) - 1) {
                    buffer[buf_idx++] = *p;
                }
            }
        }
        
        // Advance the pointer to the next character in the input string
        p++;
    }

    // Add any remaining characters in the buffer as the final token
    if (buf_idx > 0) {
        buffer[buf_idx] = '\0';
        add_token(&list, buffer);
    }
    
    free(str);
    return list;
}

/**
 * @brief Frees the memory allocated for a TokenList.
 * @param tokens The TokenList to free.
 */
void free_tokens(TokenList *tokens) {
    Token *current = tokens->head;
    while (current != NULL) {
        Token *next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
    tokens->head = NULL;
    tokens->tail = NULL;
}

// Global variable to hold the shell's name (e.g., from argv[0] in main)
extern char *shell_name;

/**
 * @brief Expands shell variables in a token string.
 *
 * This function handles simple variable expansion in the form of `$VARIABLE`.
 * It now also handles special shell parameters like `$0`.
 * It allocates a new string to hold the expanded value.
 *
 * @param token_value The token string to expand.
 * @return A newly allocated string with variables expanded.
 */
char* expand_variables(const char* token_value) {
    // If the token is NULL or doesn't contain a '$', just return a copy.
    if (!token_value || !strchr(token_value, '$')) {
        return strdup(token_value);
    }

    char expanded_buffer[4096] = {0}; // Large buffer for expanded string
    char *write_ptr = expanded_buffer;
    const char *read_ptr = token_value;
    
    while (*read_ptr) {
        if (*read_ptr == '$') {
            read_ptr++; // Move past the '$'
            
            // Check for special parameters like $0
            if (*read_ptr == '0') {
                read_ptr++;
                if (shell_name) {
                    strcpy(write_ptr, shell_name);
                    write_ptr += strlen(shell_name);
                }
            } else {
                // Find the end of the variable name
                const char *var_start = read_ptr;
                while (*read_ptr && (isalnum(*read_ptr) || *read_ptr == '_')) {
                    read_ptr++;
                }
                
                size_t var_name_len = read_ptr - var_start;
                if (var_name_len > 0) {
                    char var_name[var_name_len + 1];
                    strncpy(var_name, var_start, var_name_len);
                    var_name[var_name_len] = '\0';
                    
                    // Get the value from the environment
                    char *var_value = getenv(var_name);
                    
                    if (var_value) {
                        strcpy(write_ptr, var_value);
                        write_ptr += strlen(var_value);
                    }
                } else {
                    // If there's a '$' but no variable name, just copy the '$'
                    *write_ptr++ = '$';
                }
            }
        } else {
            *write_ptr++ = *read_ptr++;
        }
    }
    
    *write_ptr = '\0';
    return strdup(expanded_buffer);
}


/**
 * @brief Parses a token list into a command structure, with variable expansion.
 *
 * This function iterates through tokens, expands any shell variables, and
 * populates the command's argv array. It correctly handles pipelines, background
 * processes, and command separators.
 *
 * @param tokens The TokenList to parse.
 * @return A pointer to the parsed Command structure.
 */
Command *parse_command(TokenList *tokens) {
    if (tokens->head == NULL) {
        return NULL;
    }

    Command *head_cmd = (Command *)malloc(sizeof(Command));
    if (head_cmd == NULL) {
        perror("ash: memory allocation failed");
        exit(1);
    }
    memset(head_cmd, 0, sizeof(Command));

    Command *current_cmd = head_cmd;
    Token *current_token = tokens->head;
    int arg_index = 0;

    while (current_token != NULL) {
        if (strcmp(current_token->value, ";") == 0) {
            current_cmd->type = CMD_SEMI;
            current_cmd->next = (Command *)malloc(sizeof(Command));
            if (current_cmd->next == NULL) {
                perror("ash: memory allocation failed");
                exit(1);
            }
            memset(current_cmd->next, 0, sizeof(Command));
            current_cmd = current_cmd->next;
            arg_index = 0;
        } else if (strcmp(current_token->value, "|") == 0) {
            current_cmd->type = CMD_PIPE;
            current_cmd->next = (Command *)malloc(sizeof(Command));
            if (current_cmd->next == NULL) {
                perror("ash: memory allocation failed");
                exit(1);
            }
            memset(current_cmd->next, 0, sizeof(Command));
            current_cmd = current_cmd->next;
            arg_index = 0;
        } else if (strcmp(current_token->value, "&") == 0) {
            current_cmd->type = CMD_BACKGROUND;
            current_cmd->next = (Command *)malloc(sizeof(Command));
            if (current_cmd->next == NULL) {
                perror("ash: memory allocation failed");
                exit(1);
            }
            memset(current_cmd->next, 0, sizeof(Command));
            current_cmd = current_cmd->next;
            arg_index = 0;
        } else if (strcmp(current_token->value, "&&") == 0) {
            current_cmd->type = CMD_AND;
            current_cmd->next = (Command *)malloc(sizeof(Command));
            if (current_cmd->next == NULL) {
                perror("ash: memory allocation failed");
                exit(1);
            }
            memset(current_cmd->next, 0, sizeof(Command));
            current_cmd = current_cmd->next;
            arg_index = 0;
        } else if (strcmp(current_token->value, "||") == 0) {
            current_cmd->type = CMD_OR;
            current_cmd->next = (Command *)malloc(sizeof(Command));
            if (current_cmd->next == NULL) {
                perror("ash: memory allocation failed");
                exit(1);
            }
            memset(current_cmd->next, 0, sizeof(Command));
            current_cmd = current_cmd->next;
            arg_index = 0;
        } else if (strcmp(current_token->value, "<") == 0) {
            if (current_token->next != NULL) {
                current_cmd->redir_in = expand_variables(current_token->next->value);
                current_token = current_token->next;
            }
        } else if (strcmp(current_token->value, ">") == 0) {
            if (current_token->next != NULL) {
                current_cmd->redir_out = expand_variables(current_token->next->value);
                current_token = current_token->next;
            }
        } else {
            if (arg_index < MAX_ARGS - 1) {
                current_cmd->argv[arg_index++] = expand_variables(current_token->value);
            }
        }
        current_token = current_token->next;
    }
    current_cmd->type = CMD_END;
    return head_cmd;
}

/**
 * @brief Frees the memory allocated for a Command structure.
 * @param cmd The Command structure to free.
 */
void free_command(Command *cmd) {
    Command *current = cmd;
    while (current != NULL) {
        Command *temp = current;
        current = current->next;
        for (int i = 0; temp->argv[i] != NULL; i++) {
            free(temp->argv[i]);
        }
        if (temp->redir_in != NULL) {
            free(temp->redir_in);
        }
        if (temp->redir_out != NULL) {
            free(temp->redir_out);
        }
        free(temp);
    }
}
