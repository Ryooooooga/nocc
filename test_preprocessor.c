#include "nocc.h"

typedef struct TestSuite {
    int kind;
    const char *text;
    const char *string;
} TestSuite;

void test_pp(const char *filename, const char *src, const TestSuite *suites) {
    const Token **toks = (const Token **)preprocess(filename, src)->data;
    int i = 0;

    do {
        if (toks[i]->kind != suites[i].kind) {
            fprintf(stderr, "%s: toks[%d]->kind is expected %d, but got %d\n",
                    filename, i, suites[i].kind, toks[i]->kind);
            exit(1);
        }

        if (strcmp(toks[i]->text, suites[i].text) != 0) {
            fprintf(stderr, "%s: toks[%d]->text is expected %s, but got %s\n",
                    filename, i, suites[i].text, toks[i]->text);
            exit(1);
        }

        if (suites[i].string == NULL) {
            if (toks[i]->string != NULL) {
                fprintf(stderr,
                        "%s: toks[%d]->string is expected null, but got %s\n",
                        filename, i, toks[i]->string);
                exit(1);
            }
        } else {
            if (toks[i]->string == NULL) {
                fprintf(stderr,
                        "%s: toks[%d]->string is expected %s, but got null\n",
                        filename, i, suites[i].string);
                exit(1);
            } else if (strcmp(toks[i]->string, suites[i].string) != 0) {
                fprintf(stderr,
                        "%s: toks[%d]->string is expected %s, but got %s\n",
                        filename, i, suites[i].string, toks[i]->string);
                exit(1);
            }
        }
    } while (toks[i++]->kind != '\0');
}

void test_preprocessor(void) {
    test_pp("define",
            "# define N 0\n"
            "N\n",
            (TestSuite[]){
                {.kind = token_number, .text = "0", .string = NULL},
                {.kind = '\0', .text = "", .string = NULL},
            });

    test_pp("define2",
            "# define M a b c\n"
            "M\n",
            (TestSuite[]){
                {.kind = token_identifier, .text = "a", .string = NULL},
                {.kind = token_identifier, .text = "b", .string = NULL},
                {.kind = token_identifier, .text = "c", .string = NULL},
                {.kind = '\0', .text = "", .string = NULL},
            });
}
