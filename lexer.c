#include "nocc.h"

Token *token_new(int kind, const char *text, int length, int line) {
    Token *t;

    assert(text != NULL);
    assert(0 <= length);

    t = malloc(sizeof(*t));
    t->kind = kind;
    t->text = str_dup_n(text, length);
    t->line = line;
    t->string = NULL;
    t->len_string = 0;

    return t;
}

Token *character_token_new(char c, const char *text, int length, int line) {
    Token *t;

    t = token_new(token_character, text, length, line);
    t->string = str_dup_n(&c, 1);
    t->len_string = 1;

    return t;
}

Token *string_token_new(Vec *chars, const char *text, int length, int line) {
    Token *t;
    int i;

    assert(chars != NULL);

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

    assert(src != NULL);
    assert(index != NULL);
    assert(line != NULL);

    switch (src[*index]) {
    case '\0':
        fprintf(stderr, "unexpected end of file in a literal\n");
        exit(1);

    case '\n':
        fprintf(stderr, "unterminated literal\n");
        exit(1);

    case '\\':
        (*index)++; /* eat \ */

        switch (src[*index]) {
        case '0':
            (*index)++; /* eat 0 */
            return '\0';

        case '\'':
            (*index)++; /* eat ' */
            return '\'';

        case '\"':
            (*index)++; /* eat " */
            return '\"';

        case 'n':
            (*index)++; /* eat n */
            return '\n';

        case '\\':
            (*index)++; /* eat \ */
            return '\\';

        default:
            fprintf(stderr, "unknown escape sequence '\\%c'\n", src[*index]);
            exit(1);
        }

    default:
        c = src[*index];
        (*index)++; /* eat c */
        return c;
    }
}

Token *lex_token(const char *src, int *index, int *line) {
    assert(src != NULL);
    assert(index != NULL);
    assert(line != NULL);

    while (src[*index]) {
        int kind;
        int line_start;
        int start;

        line_start = *line;
        start = *index;

        /* new line */
        if (src[*index] == '\n') {
            (*line)++;
            (*index)++;

            return token_new('\n', src + start, *index - start, line_start);
        }

        /* separator */
        if (isspace(src[*index])) {
            while (isspace(src[*index]) && src[*index] != '\n') {
                (*index)++;
            }

            return token_new(' ', src + start, *index - start, line_start);
        }

        /* comment */
        if (src[*index + 0] == '/' && src[*index + 1] == '*') {
            *index = *index + 2; /* eat '/' '*' */

            while (!(src[*index + 0] == '*' && src[*index + 1] == '/')) {
                if (src[*index] == '\n') {
                    (*line)++;
                }

                (*index)++;
            }

            *index = *index + 2; /* eat '*' '/' */

            return token_new(' ', " ", 1, line_start);
        }

        /* number */
        if (isdigit(src[*index])) {
            /* [0-9]+ */
            while (isdigit(src[*index])) {
                (*index)++;
            }

            return token_new(token_number, src + start, *index - start,
                             line_start);
        }

        /* character */
        if (src[*index] == '\'') {
            char c;

            (*index)++; /* eat ' */

            /* character literal contents */
            c = parse_literal_char(src, index, line);

            /* ' */
            if (src[*index] != '\'') {
                fprintf(stderr, "unterminated character literal\n");
                exit(1);
            }

            (*index)++; /* eat ' */

            return character_token_new(c, src + start, *index - start,
                                       line_start);
        }

        /* string */
        if (src[*index] == '\"') {
            Vec *chars;

            (*index)++; /* eat " */

            /* string literal contents */
            chars = vec_new();

            while (src[*index] != '\"') {
                vec_push(chars, (void *)(intptr_t)parse_literal_char(src, index,
                                                                     line));
            }

            (*index)++; /* eat " */

            return string_token_new(chars, src + start, *index - start,
                                    line_start);
        }

        /* identifier */
        if (isalpha(src[*index]) || (src[*index] == '_')) {
            /* [0-9A-Z_a-z]+ */
            while (isalnum(src[*index]) || (src[*index] == '_')) {
                (*index)++;
            }

            return token_new(token_identifier, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '<' && src[*index + 1] == '=') {
            *index = *index + 2;
            return token_new(token_lesser_equal, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '>' && src[*index + 1] == '=') {
            *index = *index + 2;
            return token_new(token_greater_equal, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '=' && src[*index + 1] == '=') {
            *index = *index + 2;
            return token_new(token_equal, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '!' && src[*index + 1] == '=') {
            *index = *index + 2;
            return token_new(token_not_equal, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '+' && src[*index + 1] == '+') {
            *index = *index + 2;
            return token_new(token_increment, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '-' && src[*index + 1] == '-') {
            *index = *index + 2;
            return token_new(token_decrement, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '&' && src[*index + 1] == '&') {
            *index = *index + 2;
            return token_new(token_and, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '|' && src[*index + 1] == '|') {
            *index = *index + 2;
            return token_new(token_or, src + start, *index - start, line_start);
        }

        if (src[*index + 0] == '-' && src[*index + 1] == '>') {
            *index = *index + 2;
            return token_new(token_arrow, src + start, *index - start,
                             line_start);
        }

        if (src[*index + 0] == '.' && src[*index + 1] == '.' &&
            src[*index + 2] == '.') {
            *index = *index + 3;
            return token_new(token_var_args, src + start, *index - start,
                             line_start);
        }

        /* single character */
        kind = src[start];
        (*index)++;

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

    assert(src != NULL);

    index = 0;
    line = 1;

    tokens = vec_new();

    do {
        t = lex_token(src, &index, &line);
        vec_push(tokens, t);
    } while (t->kind != '\0');

    return tokens;
}
