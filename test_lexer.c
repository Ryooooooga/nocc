#include "nocc.h"

typedef struct {
    int kind;
    const char *text;
    int line;
    const char *string;
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

        if (suites[i].string == NULL) {
            if (t->string != NULL) {
                fprintf(stderr,
                        "%s\n%d: t->string is expected null, but got %s\n", src,
                        (int)i, t->string);
                exit(1);
            }
        } else {
            if (t->string == NULL) {
                fprintf(stderr,
                        "%s\n%d: t->string is expected %s, but got null\n", src,
                        (int)i, suites[i].string);
                exit(1);
            } else if (strcmp(t->string, suites[i].string) != 0) {
                fprintf(stderr,
                        "%s\n%d: t->string is expected %s, but got %s\n", src,
                        (int)i, suites[i].string, t->string);
                exit(1);
            }
        }

        i++;
    } while (t->kind);
}

void test_lexer(void) {
    const TokenTestSuite suites1[] = {
        {token_number, "42", 1, NULL},      {'+', "+", 1, NULL},
        {token_number, "5", 1, NULL},       {'-', "-", 1, NULL},
        {token_identifier, "a", 2, NULL},   {'*', "*", 2, NULL},
        {token_identifier, "abc", 2, NULL}, {'\0', "", 2, NULL},
    };

    const TokenTestSuite suites2[] = {
        {token_if, "if", 1, NULL},
        {'(', "(", 1, NULL},
        {token_identifier, "x", 1, NULL},
        {token_lesser_equal, "<=", 1, NULL},
        {token_identifier, "y", 1, NULL},
        {token_equal, "==", 1, NULL},
        {token_identifier, "true", 1, NULL},
        {')', ")", 1, NULL},
        {'{', "{", 1, NULL},
        {'}', "}", 2, NULL},
        {'\0', "", 2, NULL},
    };

    const TokenTestSuite suites3[] = {
        {token_string, "\"\"", 1, ""},
        {token_string, "\"hello\"", 1, "hello"},
        {token_string, "\"wor\\nld\"", 2, "wor\nld"},
        {token_string, "\"he\\\"lp\"", 2, "he\"lp"},
        {'\0', "", 2, NULL},
    };

    test_tokens("42 + 5 - \n a*abc", suites1);
    test_tokens("if (x<=y==true) {\n}", suites2);
    test_tokens("\"\" \"hello\"\n\"wor\\nld\" \"he\\\"lp\"", suites3);
}
