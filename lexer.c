#include "nocc.h"

Token *token_new(int kind, const char *text, int length, int line) {
    Token *t;

    assert(text);
    assert(0 <= length);

    t = malloc(sizeof(*t));
    t->kind = kind;
    t->text = malloc(length + 1);
    t->line = line;

    strncpy(t->text, text, length);
    t->text[length] = '\0';

    return t;
}

Token *lex_token(const char *src, int *index, int *line) {
    assert(src);
    assert(index);
    assert(line);

    while (src[*index]) {
        int kind;
        int start;

        start = *index;

        /* separator */
        if (isspace(src[*index])) {
            if (src[*index] == '\n') {
                *line += 1;
            }

            *index += 1;
            continue;
        }

        /* number */
        if (isdigit(src[*index])) {
            /* [0-9]+ */
            while (isdigit(src[*index])) {
                *index += 1;
            }

            return token_new(token_number, src + start, *index - start, *line);
        }

        /* identifier */
        if (isalpha(src[*index]) || (src[*index] == '_')) {
            Token *t;

            /* [0-9A-Z_a-z]+ */
            while (isalnum(src[*index]) || (src[*index] == '_')) {
                *index += 1;
            }

            t = token_new(token_identifier, src + start, *index - start, *line);

            if (strcmp(t->text, "if") == 0) {
                t->kind = token_if;
            } else if (strcmp(t->text, "else") == 0) {
                t->kind = token_else;
            } else if (strcmp(t->text, "return") == 0) {
                t->kind = token_return;
            } else if (strcmp(t->text, "void") == 0) {
                t->kind = token_void;
            } else if (strcmp(t->text, "int") == 0) {
                t->kind = token_int;
            }

            return t;
        }

        /* single character */
        kind = src[start];
        *index += 1;

        return token_new(kind, src + start, 1, *line);
    }

    /* end of file */
    return token_new('\0', src + *index, 0, *line);
}

Vec *lex(const char *src) {
    int index;
    int line;
    Token *t;
    Vec *tokens;

    assert(src);

    index = 0;
    line = 1;

    tokens = vec_new();

    do {
        t = lex_token(src, &index, &line);
        vec_push(tokens, t);
    } while (t->kind != '\0');

    return tokens;
}
