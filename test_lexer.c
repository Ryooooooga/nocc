#include "nocc.h"

typedef struct {
    int kind;
    const char *text;
    int line;
    const char *string;
} TokenTestSuite;

static void test_tokens(const char *src, const TokenTestSuite *suites) {
    const Token **toks = (const Token **)lex(src)->data;
    int i = 0;

    do {
        if (toks[i]->kind != suites[i].kind) {
            fprintf(stderr, "%s\n: toks[%d]->kind is expected %d, but got %d\n",
                    src, i, suites[i].kind, toks[i]->kind);
            exit(1);
        }

        if (strcmp(toks[i]->text, suites[i].text) != 0) {
            fprintf(stderr, "%s\n: toks[%d]->text is expected %s, but got %s\n",
                    src, i, suites[i].text, toks[i]->text);
            exit(1);
        }

        if (toks[i]->line != suites[i].line) {
            fprintf(stderr, "%s\n: toks[%d]->line is expected %d, but got %d\n",
                    src, i, suites[i].line, toks[i]->line);
            exit(1);
        }

        if (suites[i].string == NULL) {
            if (toks[i]->string != NULL) {
                fprintf(stderr,
                        "%s\n: toks[%d]->string is expected null, but got %s\n",
                        src, i, toks[i]->string);
                exit(1);
            }
        } else {
            if (toks[i]->string == NULL) {
                fprintf(stderr,
                        "%s\n: toks[%d]->string is expected %s, but got null\n",
                        src, i, suites[i].string);
                exit(1);
            } else if (strcmp(toks[i]->string, suites[i].string) != 0) {
                fprintf(stderr,
                        "%s\n: toks[%d]->string is expected %s, but got %s\n",
                        src, i, suites[i].string, toks[i]->string);
                exit(1);
            }
        }
    } while (toks[i++]->kind != '\0');
}

void test_lexer(void) {
    test_tokens("42  + 5 - \n a*abc", (TokenTestSuite[]){
                                          {token_number, "42", 1, NULL},
                                          {' ', "  ", 1, NULL},
                                          {'+', "+", 1, NULL},
                                          {' ', " ", 1, NULL},
                                          {token_number, "5", 1, NULL},
                                          {' ', " ", 1, NULL},
                                          {'-', "-", 1, NULL},
                                          {' ', " ", 1, NULL},
                                          {'\n', "\n", 1, NULL},
                                          {' ', " ", 2, NULL},
                                          {token_identifier, "a", 2, NULL},
                                          {'*', "*", 2, NULL},
                                          {token_identifier, "abc", 2, NULL},
                                          {'\0', "", 2, NULL},
                                      });

    test_tokens("if (x<=y==true) {\n}", (TokenTestSuite[]){
                                            {token_identifier, "if", 1, NULL},
                                            {' ', " ", 1, NULL},
                                            {'(', "(", 1, NULL},
                                            {token_identifier, "x", 1, NULL},
                                            {token_lesser_equal, "<=", 1, NULL},
                                            {token_identifier, "y", 1, NULL},
                                            {token_equal, "==", 1, NULL},
                                            {token_identifier, "true", 1, NULL},
                                            {')', ")", 1, NULL},
                                            {' ', " ", 1, NULL},
                                            {'{', "{", 1, NULL},
                                            {'\n', "\n", 1, NULL},
                                            {'}', "}", 2, NULL},
                                            {'\0', "", 2, NULL},
                                        });

    test_tokens("\"\" \"hello\"\n\"wor\\nld\" \"he\\\"lp\"",
                (TokenTestSuite[]){
                    {token_string, "\"\"", 1, ""},
                    {' ', " ", 1, NULL},
                    {token_string, "\"hello\"", 1, "hello"},
                    {'\n', "\n", 1, NULL},
                    {token_string, "\"wor\\nld\"", 2, "wor\nld"},
                    {' ', " ", 2, NULL},
                    {token_string, "\"he\\\"lp\"", 2, "he\"lp"},
                    {'\0', "", 2, NULL},
                });

    test_tokens("/* comment */ /* comment\n */ code */",
                (TokenTestSuite[]){
                    {' ', " ", 1, NULL}, /* comment */
                    {' ', " ", 1, NULL}, /* */
                    {' ', " ", 1, NULL}, /* comment\n */
                    {' ', " ", 2, NULL}, /* */
                    {token_identifier, "code", 2, NULL},
                    {' ', " ", 2, NULL},
                    {'*', "*", 2, NULL},
                    {'/', "/", 2, NULL},
                    {'\0', "", 2, NULL},
                });

    test_tokens("'a''b''\\\\''\\'''\\n'",
                (TokenTestSuite[]){
                    {token_character, "'a'", 1, "a"},
                    {token_character, "'b'", 1, "b"},
                    {token_character, "'\\\\'", 1, "\\"},
                    {token_character, "'\\''", 1, "\'"},
                    {token_character, "'\\n'", 1, "\n"},
                    {'\0', "", 1, NULL},
                });
}
