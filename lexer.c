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
        const char *text;

        text = src + *index;

        /* single character */
        kind = text[0];
        *index += 1;

        return token_new(kind, text, 1, *line);
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
