#include "nocc.h"

struct Preprocessor {
    Vec *result;
    Token **tokens;
    int index;
    Map *macros;
};

typedef struct Preprocessor Preprocessor;

Token *pp_current_token(Preprocessor *pp) {
    assert(pp);

    return pp->tokens[pp->index];
}

Token *pp_last_token(Preprocessor *pp) {
    assert(pp);
    assert(pp->result->size > 0);

    return vec_back(pp->result);
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

Token *pp_skip_separator(Preprocessor *pp) {
    assert(pp);

    while (pp_current_token(pp)->kind == ' ') {
        pp_consume_token(pp);
    }

    return pp_current_token(pp);
}

void pp_concat_string(Preprocessor *pp, const Token *str) {
    Token *t;
    int text_len1;
    int text_len2;

    t = pp_last_token(pp);

    assert(t);
    assert(t->kind == token_string);
    assert(t->string);
    assert(str);
    assert(str->kind == token_string);
    assert(str->string);

    text_len1 = strlen(t->text) - 1;   /* without last '\"' */
    text_len2 = strlen(str->text) - 1; /* without first '\"' */

    t->text = str_cat_n(t->text, text_len1, str->text + 1, text_len2);
    t->string =
        str_cat_n(t->string, t->len_string, str->string, str->len_string);
    t->len_string += str->len_string;
}

void pp_define(Preprocessor *pp) {
    Token *identifier;
    Vec *macro_tokens;

    /* define */
    if (strcmp(pp_current_token(pp)->text, "define") != 0) {
        fprintf(stderr, "expected define, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* identifier */
    identifier = pp_skip_separator(pp);

    if (identifier->kind != token_identifier) {
        fprintf(stderr, "expected identifier after #define, but got %s\n",
                identifier->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* macro contents */
    macro_tokens = vec_new();

    while (pp_skip_separator(pp)->kind != '\0' &&
           pp_skip_separator(pp)->kind != '\n') {
        vec_push(macro_tokens, pp_consume_token(pp));
    }

    /* redefinition check */
    if (map_contains(pp->macros, identifier->text)) {
        fprintf(stderr, "macro %s has already been defined\n",
                identifier->text);
        exit(1);
    }

    /* register macro */
    map_add(pp->macros, identifier->text, macro_tokens);
}

void pp_include(Preprocessor *pp) {
    /* include */
    if (strcmp(pp_current_token(pp)->text, "include") != 0) {
        fprintf(stderr, "expected include, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    fprintf(stderr, "#include not implemented\n");
    exit(1);
}

void pp_directive(Preprocessor *pp) {
    Token *t;

    /* # */
    if (pp_current_token(pp)->kind != '#') {
        fprintf(stderr, "expected #, but got %s", pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* identifier */
    t = pp_skip_separator(pp);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier after #, but got %s\n", t->text);
        exit(1);
    }

    if (strcmp(t->text, "define") == 0) {
        pp_define(pp);
    } else if (strcmp(t->text, "include") == 0) {
        pp_include(pp);
    } else {
        fprintf(stderr, "unknown preprocessor directive %s\n", t->text);
        exit(1);
    }
}

void pp_string(Preprocessor *pp) {
    Token *t;

    /* string */
    t = pp_consume_token(pp);

    if (t->kind != token_string) {
        fprintf(stderr, "expected string, but got %s\n", t->text);
        exit(1);
    }

    if (pp->result->size > 0 && pp_last_token(pp)->kind == token_string) {
        /* concat literals if the last token is string */
        pp_concat_string(pp, t);
    } else {
        vec_push(pp->result, t);
    }
}

void preprocess_line(Preprocessor *pp) {
    switch (pp_skip_separator(pp)->kind) {
    case '\0':
        break;

    case '\n':
        pp_consume_token(pp);
        break;

    case '#':
        pp_directive(pp);
        break;

    case token_string:
        pp_string(pp);
        break;

    default:
        vec_push(pp->result, pp_consume_token(pp));
        break;
    }
}

void preprocess_lines(Preprocessor *pp) {
    while (pp_skip_separator(pp)->kind != '\0') {
        preprocess_line(pp);
    }
}

Vec *preprocess(const char *filename, const char *src) {
    Preprocessor pp;

    (void)filename;

    /* make preprocessor context */
    pp.result = vec_new();
    pp.tokens = (Token **)lex(src)->data;
    pp.index = 0;
    pp.macros = map_new();

    preprocess_lines(&pp);

    /* push end of file */
    vec_push(pp.result, pp_current_token(&pp));

    return pp.result;
}
