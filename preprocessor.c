#include "nocc.h"

struct Preprocessor {
    Vec *result;
    Token **tokens;
    int index;
    Vec *include_directories;
    Vec *include_stack;
    Map *macros;
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
        pp->index += 1;
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

const char *pp_current_file_path(Preprocessor *pp) {
    assert(pp != NULL);
    assert(pp->include_stack->size > 0);

    return vec_back(pp->include_stack);
}

void pp_read_file(Preprocessor *pp, const char *filename, char **path,
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
        return;
    }

    /* search include directories */
    for (i = 0; i < pp->include_directories->size; i++) {
        *path = path_join(pp->include_directories->data[i], filename);
        *src = read_file(*path);

        if (*src != NULL) {
            return;
        }
    }

    /* file not found */
    *path = NULL;
    *src = NULL;

    fprintf(stderr, "cannot include file %s in %s\n", filename,
            pp_current_file_path(pp));
    exit(1);
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
    t->len_string += str->len_string;
}

void pp_expand_macro(Preprocessor *pp, Token *t) {
    Vec *macro_tokens;
    int i;

    /* check if the identifier is a macro */
    macro_tokens = map_get(pp->macros, t->text);

    if (macro_tokens == NULL) {
        /* identifier is not a macro */
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
    const Token *filename;
    char *path;
    char *src;

    Token **saved_tokens;
    int saved_index;

    /* include */
    if (strcmp(pp_current_token(pp)->text, "include") != 0) {
        fprintf(stderr, "expected include, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* string */
    filename = pp_skip_separator(pp);

    if (filename->kind != token_string) {
        fprintf(stderr, "expected string after #include, but got %s\n",
                filename->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* new line */
    if (pp_skip_separator(pp)->kind != '\0' &&
        pp_skip_separator(pp)->kind != '\n') {
        fprintf(stderr, "expected new line after #include %s, but got %s\n",
                filename->text, pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* check the depth of include stack */
    if (pp->include_stack->size >= 256) {
        fprintf(stderr, "#include nested too deeply\n");
        exit(1);
    }

    /* open file */
    pp_read_file(pp, filename->string, &path, &src);

    /* read file */
    saved_tokens = pp->tokens;
    saved_index = pp->index;

    pp->tokens = (Token **)lex(src)->data;
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
                    "unexpected end of file, unterminated #if directives\n");
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
        fprintf(stderr, "expected ifdef or ifndef, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* identifier */
    macro_name = pp_skip_separator(pp);

    if (macro_name->kind != token_identifier) {
        fprintf(stderr, "expected identifier after #ifndef, but got %s\n",
                macro_name->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* new line */
    if (pp_skip_separator(pp)->kind != '\0' &&
        pp_skip_separator(pp)->kind != '\n') {
        fprintf(stderr, "expected new line after #ifndef %s, but got %s\n",
                macro_name->text, pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

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
    /* else */
    if (strcmp(pp_current_token(pp)->text, "else") != 0) {
        fprintf(stderr, "expected else, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* new line */
    if (pp_skip_separator(pp)->kind != '\0' &&
        pp_skip_separator(pp)->kind != '\n') {
        fprintf(stderr, "expected new line after #else, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    if (!accept_else) {
        fprintf(stderr, "#else without #if\n");
        exit(1);
    }

    if (skip) {
        pp_skip_until_else_or_endif(pp, false);
    } else {
        preprocess_lines(pp, false, true);
    }
}

void pp_endif(Preprocessor *pp, bool accept_endif) {
    /* endif */
    if (strcmp(pp_current_token(pp)->text, "endif") != 0) {
        fprintf(stderr, "expected endif, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    /* new line */
    if (pp_skip_separator(pp)->kind != '\0' &&
        pp_skip_separator(pp)->kind != '\n') {
        fprintf(stderr, "expected new line after #endif, but got %s\n",
                pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

    if (!accept_endif) {
        fprintf(stderr, "#endif without #if\n");
        exit(1);
    }
}

bool pp_directive(Preprocessor *pp, bool accept_else, bool accept_endif) {
    Token *t;

    /* # */
    if (pp_current_token(pp)->kind != '#') {
        fprintf(stderr, "expected #, but got %s", pp_current_token(pp)->text);
        exit(1);
    }
    pp_consume_token(pp);

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
        fprintf(stderr, "unknown preprocessor directive #%s\n", t->text);
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
    pp.tokens = (Token **)lex(src)->data;
    pp.index = 0;
    pp.include_directories = include_directories;
    pp.include_stack = vec_new();
    pp.macros = map_new();

    vec_push(pp.include_stack, (char *)filename);

    preprocess_lines(&pp, false, false);

    /* push end of file */
    vec_push(pp.result, pp_current_token(&pp));

    return pp.result;
}
