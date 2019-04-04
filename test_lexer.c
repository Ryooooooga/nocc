#include "nocc.h"

typedef struct {
    int kind;
    const char *text;
} TokenTestSuite;

static void test_tokens(const char *src, const TokenTestSuite *suites) {
    Vec *tokens;
    Token *t;
    size_t i;

    tokens = lex(src);

    i = 0;
    do {
        t = tokens->data[i];

        assert(t->kind == suites[i].kind);
        assert(strcmp(t->text, suites[i].text) == 0);
        i++;
    } while (t->kind);
}

void test_lexer(void) {
    const TokenTestSuite suites[] = {{token_number, "42"},     {'+', "+"},
                                     {token_number, "5"},      {'-', "-"},
                                     {token_identifier, "a"},  {'*', "*"},
                                     {token_identifier, "abc"}};

    test_tokens("42 + 5 - a*abc", suites);
}
