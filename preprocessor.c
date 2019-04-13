#include "nocc.h"

struct Preprocessor {
    Vec *result;
    Token **tokens;
    int index;
};

typedef struct Preprocessor Preprocessor;

Token *pp_current_token(Preprocessor *pp) {
    assert(pp);

    return pp->tokens[pp->index];
}

Token *pp_consume_token(Preprocessor *pp) {
    Token *t;

    assert(pp);

    t = pp_current_token(pp);

    if (t->kind != '\0') {
        pp->index += 1;
    }

    return t;
}

void pp_concat_string(Token *t, const Token *u) {
    assert(t);
    assert(t->kind == token_string);
    assert(t->string);
    assert(u);
    assert(u->kind == token_string);
    assert(u->string);

    t->text = str_cat_n(t->text, strlen(t->text) - 1, u->text + 1,
                        strlen(t->text) - 1);
    t->string = str_cat_n(t->string, t->len_string, u->string, u->len_string);
    t->len_string += u->len_string;
}

void pp_string(Preprocessor *pp) {
    Token *token;
    Token *t;

    /* string */
    token = pp_consume_token(pp);

    if (token->kind != token_string) {
        fprintf(stderr, "expected string, but got %s\n", token->text);
        exit(1);
    }

    while (1) {
        t = pp_current_token(pp);

        switch (t->kind) {
        case ' ':
        case '\n':
            /* ignore separators */
            pp_consume_token(pp);
            break;

        case token_string:
            /* concat strings */
            pp_concat_string(token, pp_consume_token(pp));
            break;

        default:
            vec_push(pp->result, token);
            return;
        }
    }
}

void preprocess_line(Preprocessor *pp) {
    switch (pp_current_token(pp)->kind) {
    case token_string:
        pp_string(pp);
        break;

    default:
        vec_push(pp->result, pp_consume_token(pp));
        break;
    }
}

void preprocess_lines(Preprocessor *pp) {
    Token *t;

    while (pp_current_token(pp)->kind != '\0') {
        t = pp_current_token(pp);

        switch (t->kind) {
        case ' ':
        case '\n':
            /* ignore if separators */
            pp_consume_token(pp);
            break;

        default:
            preprocess_line(pp);
            break;
        }
    }
}

Vec *preprocess(const char *filename, const char *src) {
    Preprocessor pp;

    (void)filename;

    /* make preprocessor context */
    pp.result = vec_new();
    pp.tokens = (Token **)lex(src)->data;
    pp.index = 0;

    preprocess_lines(&pp);

    /* push end of file */
    vec_push(pp.result, pp_current_token(&pp));

    return pp.result;
}
