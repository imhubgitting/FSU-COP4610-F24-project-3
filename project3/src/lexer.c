#include "lexer.h"
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024

char * get_input(void) {
    char *buffer = malloc(BUFFER_SIZE * sizeof(char));
    if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }
        return buffer;
    }
    free(buffer);
    return NULL;
}

tokenlist * get_tokens(char *input) {
    tokenlist *tokens = new_tokenlist();
    char *token = strtok(input, " ");
    while (token != NULL) {
        add_token(tokens, token);
        token = strtok(NULL, " ");
    }
    return tokens;
}

tokenlist * new_tokenlist(void) {
    tokenlist *tokens = malloc(sizeof(tokenlist));
    tokens->items = NULL;
    tokens->size = 0;
    return tokens;
}

void add_token(tokenlist *tokens, char *item) {
    tokens->size += 1;
    tokens->items = realloc(tokens->items, tokens->size * sizeof(char *));
    tokens->items[tokens->size - 1] = strdup(item);
}

void free_tokens(tokenlist *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        free(tokens->items[i]);
    }
    free(tokens->items);
    free(tokens);
}
