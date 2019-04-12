#include "nocc.h"

Token *token_new(int kind, const char *text, int length, int line) {
    Token *t;

    assert(text);
    assert(0 <= length);

    t = malloc(sizeof(*t));
    t->kind = kind;
    t->text = str_dup_n(text, length);
    t->line = line;
    t->string = NULL;
    t->len_string = 0;

    return t;
}

Token *string_token_new(Vec *chars, const char *text, int length, int line) {
    Token *t;
    int i;

    assert(chars);

    t = token_new(token_string, text, length, line);
    t->string = malloc(sizeof(char) * (chars->size + 1));
    t->len_string = chars->size;

    for (i = 0; i < t->len_string; i++) {
        t->string[i] = (intptr_t)chars->data[i];
    }

    t->string[t->len_string] = '\0';

    return t;
}

char parse_literal_char(const char *src, int *index, int *line) {
    char c;

    assert(src);
    assert(index);
    assert(line);

    switch (src[*index]) {
    case '\0':
        fprintf(stderr, "unexpected end of file in a literal\n");
        exit(1);

    case '\n':
        fprintf(stderr, "unterminated literal\n");
        exit(1);

    case '\\':
        *index += 1; /* eat \ */

        switch (src[*index]) {
        case '\'':
            *index += 1; /* eat ' */
            return '\'';

        case '\"':
            *index += 1; /* eat " */
            return '\"';

        case 'n':
            *index += 1; /* eat n */
            return '\n';

        default:
            fprintf(stderr, "unknown escape sequence '\\%c'\n", src[*index]);
            exit(1);
        }

    default:
        c = src[*index];
        *index += 1; /* eat c */
        return c;
    }
}

Token *lex_token(const char *src, int *index, int *line) {
    assert(src);
    assert(index);
    assert(line);

    while (src[*index]) {
        int kind;
        int line_start;
        int start;

        line_start = *line;
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

            return token_new(token_number, src + start, *index - start,
                             line_start);
        }

        /* string */
        if (src[*index] == '\"') {
            Vec *chars;

            *index += 1; /* eat " */

            /* string literal contents */
            chars = vec_new();

            while (src[*index] != '\"') {
                vec_push(chars, (void *)(intptr_t)parse_literal_char(src, index,
                                                                     line));
            }

            *index += 1; /* eat " */

            return string_token_new(chars, src + start, *index - start,
                                    line_start);
        }

        /* identifier */
        if (isalpha(src[*index]) || (src[*index] == '_')) {
            Token *t;

            /* [0-9A-Z_a-z]+ */
            while (isalnum(src[*index]) || (src[*index] == '_')) {
                *index += 1;
            }

            t = token_new(token_identifier, src + start, *index - start,
                          line_start);

            if (strcmp(t->text, "if") == 0) {
                t->kind = token_if;
            } else if (strcmp(t->text, "else") == 0) {
                t->kind = token_else;
            } else if (strcmp(t->text, "while") == 0) {
                t->kind = token_while;
            } else if (strcmp(t->text, "do") == 0) {
                t->kind = token_do;
            } else if (strcmp(t->text, "for") == 0) {
                t->kind = token_for;
            } else if (strcmp(t->text, "return") == 0) {
                t->kind = token_return;
            } else if (strcmp(t->text, "break") == 0) {
                t->kind = token_break;
            } else if (strcmp(t->text, "continue") == 0) {
                t->kind = token_continue;
            } else if (strcmp(t->text, "void") == 0) {
                t->kind = token_void;
            } else if (strcmp(t->text, "char") == 0) {
                t->kind = token_char;
            } else if (strcmp(t->text, "int") == 0) {
                t->kind = token_int;
            } else if (strcmp(t->text, "const") == 0) {
                t->kind = token_const;
            } else if (strcmp(t->text, "struct") == 0) {
                t->kind = token_struct;
            } else if (strcmp(t->text, "typedef") == 0) {
                t->kind = token_typedef;
            }

            return t;
        }

        if (src[*index + 0] == '<' && src[*index + 1] == '=') {
            *index += 2;
            return token_new(token_lesser_equal, src + start, 2, line_start);
        }

        if (src[*index + 0] == '>' && src[*index + 1] == '=') {
            *index += 2;
            return token_new(token_greater_equal, src + start, 2, line_start);
        }

        if (src[*index + 0] == '=' && src[*index + 1] == '=') {
            *index += 2;
            return token_new(token_equal, src + start, 2, line_start);
        }

        if (src[*index + 0] == '!' && src[*index + 1] == '=') {
            *index += 2;
            return token_new(token_not_equal, src + start, 2, line_start);
        }

        if (src[*index + 0] == '+' && src[*index + 1] == '+') {
            *index += 2;
            return token_new(token_increment, src + start, 2, line_start);
        }

        if (src[*index + 0] == '-' && src[*index + 1] == '-') {
            *index += 2;
            return token_new(token_decrement, src + start, 2, line_start);
        }

        /* single character */
        kind = src[start];
        *index += 1;

        return token_new(kind, src + start, 1, line_start);
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
