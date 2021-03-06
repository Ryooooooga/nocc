#include "nocc.h"

struct Preprocessor {
    Vec *result;
    Token **tokens;
    int index;
    Vec *include_directories;
    Vec *include_stack;
    Map *macros;
    Map *keywords;
};

typedef struct Preprocessor Preprocessor;

void pp_push_token(Preprocessor *pp, Token *t);
void pp_else(Preprocessor *pp, bool accept_else, bool skip);
void pp_endif(Preprocessor *pp, bool accept_endif);
void preprocess_lines(Preprocessor *pp, bool accept_else, bool accept_endif);

Token *pp_current_token(Preprocessor *pp) {
    assert(pp != NULL);

    return pp->tokens[pp->index];
}

Token *pp_last_token(Preprocessor *pp) {
    assert(pp != NULL);
    assert(pp->result->size > 0);

    return vec_back(pp->result);
}

Token *pp_consume_token(Preprocessor *pp) {
    Token *t;

    assert(pp != NULL);

    t = pp_current_token(pp);

    if (t->kind != '\0') {
        pp->index++;
    }

    return t;
}

Token *pp_skip_separator(Preprocessor *pp) {
    assert(pp != NULL);

    while (pp_current_token(pp)->kind == ' ') {
        pp_consume_token(pp);
    }

    return pp_current_token(pp);
}

Token *pp_expect_token(Preprocessor *pp, const char *expected_token) {
    assert(pp != NULL);
    assert(expected_token != NULL);

    if (strcmp(pp_current_token(pp)->text, expected_token) == 0) {
        return pp_consume_token(pp);
    }

    fprintf(stderr, "error at %s(%d): expected %s, but got %s\n",
            pp_current_token(pp)->filename, pp_current_token(pp)->line,
            expected_token, pp_current_token(pp)->text);
    exit(1);
}

Token *pp_expect_token_kind(Preprocessor *pp, int expected_token_kind) {
    assert(pp != NULL);

    if (pp_current_token(pp)->kind == expected_token_kind) {
        return pp_consume_token(pp);
    }

    /* TODO: better error message */
    fprintf(stderr, "error at %s(%d): expected %d, but got %s\n",
            pp_current_token(pp)->filename, pp_current_token(pp)->line,
            expected_token_kind, pp_current_token(pp)->text);
    exit(1);
}

Token *pp_expect_line_ending(Preprocessor *pp) {
    assert(pp != NULL);

    if (pp_skip_separator(pp)->kind == '\0' ||
        pp_skip_separator(pp)->kind == '\n') {
        return pp_consume_token(pp);
    }

    fprintf(stderr, "error at %s(%d): expected line ending, but got %s\n",
            pp_current_token(pp)->filename, pp_current_token(pp)->line,
            pp_current_token(pp)->text);
    exit(1);
}

const char *pp_current_file_path(Preprocessor *pp) {
    assert(pp != NULL);
    assert(pp->include_stack->size > 0);

    return vec_back(pp->include_stack);
}

bool pp_read_file(Preprocessor *pp, const char *filename, char **path,
                  char **src) {
    int i;

    assert(pp != NULL);
    assert(filename != NULL);
    assert(path != NULL);
    assert(src != NULL);

    /* search current file directory */
    *path = path_join(path_dir(pp_current_file_path(pp)), filename);
    *src = read_file(*path);

    if (*src != NULL) {
        return true;
    }

    /* search include directories */
    for (i = 0; i < pp->include_directories->size; i++) {
        *path = path_join(pp->include_directories->data[i], filename);
        *src = read_file(*path);

        if (*src != NULL) {
            return true;
        }
    }

    /* file not found */
    *path = NULL;
    *src = NULL;

    return false;
}

void pp_concat_string(Preprocessor *pp, const Token *str) {
    Token *t;
    int text_len1;
    int text_len2;

    t = pp_last_token(pp);

    assert(t != NULL);
    assert(t->kind == token_string);
    assert(t->string != NULL);
    assert(str != NULL);
    assert(str->kind == token_string);
    assert(str->string != NULL);

    text_len1 = strlen(t->text) - 1;   /* without last '\"' */
    text_len2 = strlen(str->text) - 1; /* without first '\"' */

    t->text = str_cat_n(t->text, text_len1, str->text + 1, text_len2);
    t->string =
        str_cat_n(t->string, t->len_string, str->string, str->len_string);
    t->len_string = t->len_string + str->len_string;
}

void pp_expand_macro(Preprocessor *pp, Token *t) {
    Vec *macro_tokens;
    int i;

    /* check if the identifier is a macro */
    macro_tokens = map_get(pp->macros, t->text);

    if (macro_tokens == NULL) {
        /* identifier is not a macro */
        /* check keywords */
        if (map_contains(pp->keywords, t->text)) {
            t->kind = (intptr_t)map_get(pp->keywords, t->text);
        }

        vec_push(pp->result, t);
        return;
    }

    /* expand the macro */
    for (i = 0; i < macro_tokens->size; i++) {
        /* TODO: expand macros recursively */
        pp_push_token(pp, macro_tokens->data[i]);
    }
}

void pp_push_token(Preprocessor *pp, Token *t) {
    assert(pp != NULL);
    assert(t != NULL);

    if (t->kind == '\0' || t->kind == '\n' || t->kind == ' ') {
        return;
    }

    if (pp->result->size > 0 && t->kind == token_string &&
        pp_last_token(pp)->kind == token_string) {
        pp_concat_string(pp, t);
        return;
    }

    if (t->kind == token_identifier) {
        pp_expand_macro(pp, t);
        return;
    }

    vec_push(pp->result, t);
}

void pp_define(Preprocessor *pp) {
    Token *identifier;
    Vec *macro_tokens;

    /* define */
    pp_expect_token(pp, "define");

    /* identifier */
    pp_skip_separator(pp);
    identifier = pp_expect_token_kind(pp, token_identifier);

    /* macro contents */
    macro_tokens = vec_new();

    while (pp_current_token(pp)->kind != '\0' &&
           pp_current_token(pp)->kind != '\n') {
        vec_push(macro_tokens, pp_consume_token(pp));
    }

    /* redefinition check */
    if (map_contains(pp->macros, identifier->text)) {
        fprintf(stderr, "error at %s(%d): macro %s has already been defined\n",
                identifier->filename, identifier->line, identifier->text);
        exit(1);
    }

    /* register macro */
    map_add(pp->macros, identifier->text, macro_tokens);
}

void pp_include(Preprocessor *pp) {
    const Token *t;
    const Token *filename;
    char *path;
    char *src;

    Token **saved_tokens;
    int saved_index;

    /* include */
    t = pp_expect_token(pp, "include");

    /* string */
    pp_skip_separator(pp);
    filename = pp_expect_token_kind(pp, token_string); /* TODO: <path> */

    /* new line */
    pp_expect_line_ending(pp);

    /* check the depth of include stack */
    if (pp->include_stack->size >= 256) {
        fprintf(stderr, "error at %s(%d): #include nested too deeply\n",
                t->filename, t->line);
        exit(1);
    }

    /* open file */
    if (!pp_read_file(pp, filename->string, &path, &src)) {
        fprintf(stderr, "error at %s(%d): cannot include file %s\n",
                t->filename, t->line, filename->string);
        exit(1);
    }

    /* read file */
    saved_tokens = pp->tokens;
    saved_index = pp->index;

    pp->tokens = (Token **)lex(path, src)->data;
    pp->index = 0;

    /* push include stack */
    vec_push(pp->include_stack, path);

    /* process the file */
    preprocess_lines(pp, false, false);

    /* pop include stack */
    vec_pop(pp->include_stack);

    pp->tokens = saved_tokens;
    pp->index = saved_index;
}

void pp_skip_line(Preprocessor *pp) {
    assert(pp != NULL);

    while (pp_skip_separator(pp)->kind != '\0' &&
           pp_skip_separator(pp)->kind != '\n') {
        pp_consume_token(pp);
    }
}

void pp_skip_until_else_or_endif(Preprocessor *pp, bool accept_else) {
    Token *t;

    while (1) {
        switch (pp_skip_separator(pp)->kind) {
        case '\0':
            fprintf(stderr,
                    "error at %s(%d): "
                    "unexpected end of file, unterminated #if directives\n",
                    pp_current_token(pp)->filename, pp_current_token(pp)->line);
            exit(1);

        case '\n':
            pp_consume_token(pp);
            break;

        case '#':
            pp_consume_token(pp); /* eat # */

            /* identifier */
            t = pp_skip_separator(pp);

            if (strcmp(t->text, "if") == 0 || strcmp(t->text, "ifdef") == 0 ||
                strcmp(t->text, "ifndef") == 0) {
                pp_skip_line(pp);
                pp_skip_until_else_or_endif(pp, false);
            } else if (strcmp(t->text, "endif") == 0) {
                pp_endif(pp, true);
                return;
            } else if (strcmp(t->text, "else") == 0 && accept_else) {
                pp_else(pp, true, false);
                return;
            } else {
                pp_skip_line(pp);
            }
            break;

        default:
            pp_skip_line(pp);
            break;
        }
    }
}

void pp_ifdef(Preprocessor *pp, bool defined) {
    Token *macro_name;

    /* ifndef */
    if (strcmp(pp_current_token(pp)->text, "ifdef") != 0 &&
        strcmp(pp_current_token(pp)->text, "ifndef") != 0) {
        fprintf(stderr,
                "error at %s(%d): expected ifdef or ifndef, but got %s\n",
                pp_current_token(pp)->filename, pp_current_token(pp)->line,
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* identifier */
    pp_skip_separator(pp);
    macro_name = pp_expect_token_kind(pp, token_identifier);

    /* new line */
    pp_expect_line_ending(pp);

    /* check if the macro has been defined */
    if (map_contains(pp->macros, macro_name->text) == defined) {
        /* process until #else or #endif */
        preprocess_lines(pp, true, true);
    } else {
        /* skip until #else or #endif */
        pp_skip_until_else_or_endif(pp, true);
    }
}

void pp_else(Preprocessor *pp, bool accept_else, bool skip) {
    const Token *t;

    /* else */
    t = pp_expect_token(pp, "else");

    /* new line */
    pp_expect_line_ending(pp);

    if (!accept_else) {
        fprintf(stderr, "error at %s(%d): #else without #if\n", t->filename,
                t->line);
        exit(1);
    }

    if (skip) {
        pp_skip_until_else_or_endif(pp, false);
    } else {
        preprocess_lines(pp, false, true);
    }
}

void pp_endif(Preprocessor *pp, bool accept_endif) {
    const Token *t;

    /* endif */
    t = pp_expect_token(pp, "endif");

    /* new line */
    pp_expect_line_ending(pp);

    if (!accept_endif) {
        fprintf(stderr, "error at %s(%d): #endif without #if\n", t->filename,
                t->line);
        exit(1);
    }
}

bool pp_directive(Preprocessor *pp, bool accept_else, bool accept_endif) {
    Token *t;

    /* # */
    pp_expect_token_kind(pp, '#');

    /* identifier */
    t = pp_skip_separator(pp);

    if (strcmp(t->text, "define") == 0) {
        pp_define(pp);
        return true;
    } else if (strcmp(t->text, "include") == 0) {
        pp_include(pp);
        return true;
    } else if (strcmp(t->text, "ifdef") == 0) {
        pp_ifdef(pp, true);
        return true;
    } else if (strcmp(t->text, "ifndef") == 0) {
        pp_ifdef(pp, false);
        return true;
    } else if (strcmp(t->text, "else") == 0) {
        pp_else(pp, accept_else, true);
        return false;
    } else if (strcmp(t->text, "endif") == 0) {
        pp_endif(pp, accept_endif);
        return false;
    } else {
        fprintf(stderr, "error at %s(%d): unknown preprocessor directive #%s\n",
                t->filename, t->line, t->text);
        exit(1);
    }
}

bool preprocess_line(Preprocessor *pp, bool accept_else, bool accept_endif) {
    switch (pp_skip_separator(pp)->kind) {
    case '\0':
        return false;

    case '\n':
        pp_consume_token(pp);
        return true;

    case '#':
        return pp_directive(pp, accept_else, accept_endif);

    default:
        while (pp_skip_separator(pp)->kind != '\0' &&
               pp_skip_separator(pp)->kind != '\n') {
            pp_push_token(pp, pp_consume_token(pp));
        }
        return true;
    }
}

void preprocess_lines(Preprocessor *pp, bool accept_else, bool accept_endif) {
    while (preprocess_line(pp, accept_else, accept_endif)) {
    }
}

Vec *preprocess(const char *filename, const char *src,
                Vec *include_directories) {
    Preprocessor pp;

    assert(filename != NULL);
    assert(src != NULL);
    assert(include_directories != NULL);

    /* make preprocessor context */
    pp.result = vec_new();
    pp.tokens = (Token **)lex(filename, src)->data;
    pp.index = 0;
    pp.include_directories = include_directories;
    pp.include_stack = vec_new();
    pp.macros = map_new();
    pp.keywords = map_new();

    /* keywords */
    map_add(pp.keywords, "if", (void *)(intptr_t)token_if);
    map_add(pp.keywords, "else", (void *)(intptr_t)token_else);
    map_add(pp.keywords, "switch", (void *)(intptr_t)token_switch);
    map_add(pp.keywords, "case", (void *)(intptr_t)token_case);
    map_add(pp.keywords, "default", (void *)(intptr_t)token_default);
    map_add(pp.keywords, "while", (void *)(intptr_t)token_while);
    map_add(pp.keywords, "do", (void *)(intptr_t)token_do);
    map_add(pp.keywords, "for", (void *)(intptr_t)token_for);
    map_add(pp.keywords, "return", (void *)(intptr_t)token_return);
    map_add(pp.keywords, "break", (void *)(intptr_t)token_break);
    map_add(pp.keywords, "continue", (void *)(intptr_t)token_continue);
    map_add(pp.keywords, "void", (void *)(intptr_t)token_void);
    map_add(pp.keywords, "char", (void *)(intptr_t)token_char);
    map_add(pp.keywords, "int", (void *)(intptr_t)token_int);
    map_add(pp.keywords, "long", (void *)(intptr_t)token_long);
    map_add(pp.keywords, "unsigned", (void *)(intptr_t)token_unsigned);
    map_add(pp.keywords, "const", (void *)(intptr_t)token_const);
    map_add(pp.keywords, "struct", (void *)(intptr_t)token_struct);
    map_add(pp.keywords, "typedef", (void *)(intptr_t)token_typedef);
    map_add(pp.keywords, "extern", (void *)(intptr_t)token_extern);
    map_add(pp.keywords, "sizeof", (void *)(intptr_t)token_sizeof);

    /* predefined macro */
#ifdef __APPLE__
    map_add(pp.macros, "__APPLE__", vec_new());
#endif

#ifdef __MINGW64__
    map_add(pp.macros, "__MINGW64__", vec_new());
#endif

    vec_push(pp.include_stack, (char *)filename);

    preprocess_lines(&pp, false, false);

    /* push end of file */
    vec_push(pp.result, pp_current_token(&pp));

    return pp.result;
}
