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

void pp_push_token(Preprocessor *pp, Token *t) {
    assert(pp);
    assert(t);

    if (t->kind == '\0' || t->kind == '\n' || t->kind == ' ') {
        return;
    }

    if (pp->result->size > 0 && t->kind == token_string &&
        pp_last_token(pp)->kind == token_string) {
        pp_concat_string(pp, t);
        return;
    }

    vec_push(pp->result, t);
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

    while (pp_current_token(pp)->kind != '\0' &&
           pp_current_token(pp)->kind != '\n') {
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

void pp_identifier(Preprocessor *pp) {
    Token *t;
    Vec *macro_tokens;
    int i;

    /* identifier */
    t = pp_consume_token(pp);

    if (t->kind != token_identifier) {
        fprintf(stderr, "expected identifier, but got %s", t->text);
        exit(1);
    }

    /* check if the identifier is a macro */
    macro_tokens = map_get(pp->macros, t->text);

    if (macro_tokens == NULL) {
        /* identifier is not a macro */
        pp_push_token(pp, t);
        return;
    }

    /* expand the macro */
    for (i = 0; i < macro_tokens->size; i++) {
        /* TODO: expand macros recursively */
        pp_push_token(pp, macro_tokens->data[i]);
    }
}

void preprocess_line(Preprocessor *pp) {
    switch (pp_skip_separator(pp)->kind) {
    case '#':
        pp_directive(pp);
        break;

    case token_identifier:
        pp_identifier(pp);
        break;

    default:
        pp_push_token(pp, pp_consume_token(pp));
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
