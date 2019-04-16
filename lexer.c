#include "nocc.h"

typedef struct LexerContext {
    char *filename;
    char *src;
    int index;
    int line;
} LexerContext;

Token *token_new_text(LexerContext *ctx, int kind, const char *text, int length,
                      int line) {
    Token *t;

    assert(ctx != NULL);
    assert(text != NULL);
    assert(length >= 0);

    t = malloc(sizeof(*t));
    t->kind = kind;
    t->text = str_dup_n(text, length);
    t->filename = ctx->filename;
    t->line = line;
    t->string = NULL;
    t->len_string = 0;

    return t;
}

Token *token_new(LexerContext *ctx, int kind, int start, int line) {
    return token_new_text(ctx, kind, ctx->src + start, ctx->index - start,
                          line);
}

Token *character_token_new(LexerContext *ctx, char c, int start, int line) {
    Token *t;

    t = token_new(ctx, token_character, start, line);
    t->string = str_dup_n(&c, 1);
    t->len_string = 1;

    return t;
}

Token *string_token_new(LexerContext *ctx, Vec *chars, int start, int line) {
    Token *t;
    int i;

    assert(chars != NULL);

    t = token_new(ctx, token_string, start, line);
    t->string = malloc(sizeof(char) * (chars->size + 1));
    t->len_string = chars->size;

    for (i = 0; i < t->len_string; i++) {
        t->string[i] = (intptr_t)chars->data[i];
    }

    t->string[t->len_string] = '\0';

    return t;
}

char current_char(LexerContext *ctx) {
    assert(ctx != NULL);

    return ctx->src[ctx->index];
}

char consume_char(LexerContext *ctx) {
    assert(ctx != NULL);

    switch (current_char(ctx)) {
    case '\0':
        return '\0';

    case '\n':
        ctx->line++;
        break;

    default:
        break;
    }

    return ctx->src[ctx->index++];
}

char parse_literal_char(LexerContext *ctx) {
    assert(ctx != NULL);

    switch (current_char(ctx)) {
    case '\0':
        fprintf(stderr,
                "error at %s(%d): unexpected end of file in a literal\n",
                ctx->filename, ctx->line);
        exit(1);

    case '\n':
        fprintf(stderr, "error at %s(%d): unterminated literal\n",
                ctx->filename, ctx->line);
        exit(1);

    case '\\':
        consume_char(ctx);

        switch (current_char(ctx)) {
        case '0':
            consume_char(ctx);
            return '\0';

        case '\'':
            consume_char(ctx);
            return '\'';

        case '\"':
            consume_char(ctx);
            return '\"';

        case 'n':
            consume_char(ctx);
            return '\n';

        case '\\':
            consume_char(ctx);
            return '\\';

        default:
            fprintf(stderr, "error at %s(%d): unknown escape sequence '\\%c'\n",
                    ctx->filename, ctx->line, current_char(ctx));
            exit(1);
        }

    default:
        return consume_char(ctx);
    }
}

Token *lex_token(LexerContext *ctx) {
    assert(ctx != NULL);

    int line_start;
    int start;
    char c;

    line_start = ctx->line;
    start = ctx->index;
    c = consume_char(ctx);

    /* new line */
    if (c == '\n') {
        return token_new(ctx, '\n', start, line_start);
    }

    /* separator */
    if (isspace(c)) {
        while (isspace(current_char(ctx)) && current_char(ctx) != '\n') {
            consume_char(ctx);
        }

        return token_new(ctx, ' ', start, line_start);
    }

    /* comment */
    if (c == '/' && current_char(ctx) == '*') {
        consume_char(ctx); /* eat '*' */

        while (true) {
            c = consume_char(ctx);

            if (c == '*' && current_char(ctx) == '/') {
                break;
            }

            if (c == '\0') {
                fprintf(stderr,
                        "error at %s(%d): unterminated /* ... */ comment\n",
                        ctx->filename, line_start);
                exit(1);
            }
        }

        consume_char(ctx); /* eat '/' */
        return token_new_text(ctx, ' ', " ", 1, line_start);
    }

    /* number */
    if (isdigit(c)) {
        /* [0-9]+ */
        while (isdigit(current_char(ctx))) {
            consume_char(ctx);
        }

        return token_new(ctx, token_number, start, line_start);
    }

    /* character */
    if (c == '\'') {
        /* character literal contents */
        c = parse_literal_char(ctx);

        /* ' */
        if (consume_char(ctx) != '\'') {
            fprintf(stderr, "error at %s(%d): unterminated character literal\n",
                    ctx->filename, line_start);
            exit(1);
        }

        return character_token_new(ctx, c, start, line_start);
    }

    /* string */
    if (c == '\"') {
        Vec *chars;

        /* string literal contents */
        chars = vec_new();

        while (current_char(ctx) != '\"') {
            vec_push(chars, (void *)(intptr_t)parse_literal_char(ctx));
        }

        consume_char(ctx); /* eat " */
        return string_token_new(ctx, chars, start, line_start);
    }

    /* identifier */
    if (isalpha(c) || (c == '_')) {
        /* [0-9A-Z_a-z]+ */
        while (isalnum(current_char(ctx)) || (current_char(ctx) == '_')) {
            consume_char(ctx);
        }

        return token_new(ctx, token_identifier, start, line_start);
    }

    if (c == '<' && current_char(ctx) == '=') {
        consume_char(ctx);
        return token_new(ctx, token_lesser_equal, start, line_start);
    }

    if (c == '>' && current_char(ctx) == '=') {
        consume_char(ctx);
        return token_new(ctx, token_greater_equal, start, line_start);
    }

    if (c == '=' && current_char(ctx) == '=') {
        consume_char(ctx);
        return token_new(ctx, token_equal, start, line_start);
    }

    if (c == '!' && current_char(ctx) == '=') {
        consume_char(ctx);
        return token_new(ctx, token_not_equal, start, line_start);
    }

    if (c == '+' && current_char(ctx) == '+') {
        consume_char(ctx);
        return token_new(ctx, token_increment, start, line_start);
    }

    if (c == '-' && current_char(ctx) == '-') {
        consume_char(ctx);
        return token_new(ctx, token_decrement, start, line_start);
    }

    if (c == '&' && current_char(ctx) == '&') {
        consume_char(ctx);
        return token_new(ctx, token_and, start, line_start);
    }

    if (c == '|' && current_char(ctx) == '|') {
        consume_char(ctx);
        return token_new(ctx, token_or, start, line_start);
    }

    if (c == '-' && current_char(ctx) == '>') {
        consume_char(ctx);
        return token_new(ctx, token_arrow, start, line_start);
    }

    if (c == '.' && current_char(ctx) == '.') {
        consume_char(ctx);

        if (current_char(ctx) == '.') {
            /* ... */
            consume_char(ctx);
            return token_new(ctx, token_var_args, start, line_start);
        }

        /* .. */
        fprintf(stderr, "error at %s(%d): invalid token '..'\n", ctx->filename,
                line_start);
    }

    /* single character */
    return token_new(ctx, c, start, line_start);
}

Vec *lex(const char *filename, const char *src) {
    LexerContext ctx;
    Token *t;
    Vec *tokens;

    assert(filename != NULL);
    assert(src != NULL);

    ctx.filename = str_dup(filename);
    ctx.src = str_dup(src);
    ctx.index = 0;
    ctx.line = 1;

    tokens = vec_new();

    do {
        t = lex_token(&ctx);
        vec_push(tokens, t);
    } while (t->kind != '\0');

    return tokens;
}
