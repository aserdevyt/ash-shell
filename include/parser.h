#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_ARGS 64

// Enumeration for command separators
typedef enum {
    CMD_AND,        // &&
    CMD_OR,         // ||
    CMD_SEMI,       // ;
    CMD_PIPE,       // |
    CMD_BACKGROUND, // &
    CMD_END         // end of command list
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
    CmdType type;         // The separator to the next command
    struct Command *next; // Pointer to the next command in a pipeline
} Command;

// Function prototypes
void add_token(TokenList *list, const char *value);
TokenList tokenize(const char *input);
void free_tokens(TokenList *tokens);
char* expand_variables(const char* token_value);
Command *parse_command(TokenList *tokens);
void free_command(Command *cmd);

#endif // PARSER_H
