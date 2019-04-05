#include "nocc.h"

typedef struct {
    int kind;
    const char *text;
    int line;
} TokenTestSuite;

static void test_tokens(const char *src, const TokenTestSuite *suites) {
    Vec *tokens;
    Token *t;
    size_t i;

    tokens = lex(src);

    i = 0;
    do {
        t = tokens->data[i];

        if (t->kind != suites[i].kind) {
            fprintf(stderr, "%s\n%d: t->kind is expected %d, but got %d\n", src,
                    (int)i, suites[i].kind, t->kind);
            exit(1);
        }

        if (strcmp(t->text, suites[i].text) != 0) {
            fprintf(stderr, "%s\n%d: t->text is expected %s, but got %s\n", src,
                    (int)i, suites[i].text, t->text);
            exit(1);
        }

        i++;
    } while (t->kind);
}

void test_lexer(void) {
    const TokenTestSuite suites1[] = {
        {token_number, "42", 1},      {'+', "+", 1},
        {token_number, "5", 1},       {'-', "-", 1},
        {token_identifier, "a", 2},   {'*', "*", 2},
        {token_identifier, "abc", 2}, {'\0', "", 2}};

    const TokenTestSuite suites2[] = {
        {token_if, "if", 1}, {'(', "(", 1}, {token_identifier, "x", 1},
        {')', ")", 1},       {'{', "{", 1}, {'}', "}", 2},
        {'\0', "", 2}};

    test_tokens("42 + 5 - \n a*abc", suites1);
    test_tokens("if (x) {\n}", suites2);
}
