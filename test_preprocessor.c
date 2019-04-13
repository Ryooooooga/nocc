#include "nocc.h"

typedef struct TestSuite {
    int kind;
    const char *text;
    const char *string;
} TestSuite;

void test_pp(const char *filename, const char *src, Vec *include_directories,
             const TestSuite *suites) {
    const Token **toks =
        (const Token **)preprocess(filename, src, include_directories)->data;
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

void test_preprocessor(Vec *include_directories) {
    test_pp("separator", "pp removes spaces \n and new line\n", vec_new(),
            (TestSuite[]){
                {token_identifier, "pp", NULL},
                {token_identifier, "removes", NULL},
                {token_identifier, "spaces", NULL},
                {token_identifier, "and", NULL},
                {token_identifier, "new", NULL},
                {token_identifier, "line", NULL},
                {'\0', "", NULL},
            });

    test_pp("string", "\"hell\" \"o, \"\n\"world\"\n", vec_new(),
            (TestSuite[]){
                {token_string, "\"hello, world\"", "hello, world"},
                {'\0', "", NULL},
            });

    test_pp("define",
            "# define N 0\n"
            "N\n",
            vec_new(),
            (TestSuite[]){
                {token_number, "0", NULL},
                {'\0', "", NULL},
            });

    test_pp("define2",
            "# define M a b c\n"
            "M\n",
            vec_new(),
            (TestSuite[]){
                {token_identifier, "a", NULL},
                {token_identifier, "b", NULL},
                {token_identifier, "c", NULL},
                {'\0', "", NULL},
            });

    test_pp("define3",
            "# define M \"a\" \"b\" c\n"
            "M\n",
            vec_new(),
            (TestSuite[]){
                {token_string, "\"ab\"", "ab"},
                {token_identifier, "c", NULL},
                {'\0', "", NULL},
            });

    test_pp("define3",
            "# define M \"a\" \"b\" c \"d\"\n"
            "\"x\" M \"y\"\n",
            vec_new(),
            (TestSuite[]){
                {token_string, "\"xab\"", "xab"},
                {token_identifier, "c", NULL},
                {token_string, "\"dy\"", "dy"},
                {'\0', "", NULL},
            });

    test_pp("include", "# include \"test/test_include.h\"\n",
            include_directories,
            (TestSuite[]){
                {token_int, "int", NULL},
                {token_identifier, "f", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {'\0', "", NULL},
            });

    test_pp("include2",
            "# define f F\n"
            "# include \"test/test_include.h\"\n",
            include_directories,
            (TestSuite[]){
                {token_int, "int", NULL},
                {token_identifier, "F", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {'\0', "", NULL},
            });

    test_pp("include3", "# include \"test/test_include2.h\"\n",
            include_directories,
            (TestSuite[]){
                {token_int, "int", NULL},
                {token_identifier, "F", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {'\0', "", NULL},
            });

    test_pp("include4",
            "# include \"test/test_include.h\"\n"
            "# include \"test/test_include2.h\"\n",
            include_directories,
            (TestSuite[]){
                {token_int, "int", NULL},
                {token_identifier, "f", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {token_int, "int", NULL},
                {token_identifier, "F", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {'\0', "", NULL},
            });

    test_pp("include5",
            "# include \"test/test_include.h\"\n"
            "# include \"test/test_include2.h\"\n",
            vec_new(),
            (TestSuite[]){
                {token_int, "int", NULL},
                {token_identifier, "f", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {token_int, "int", NULL},
                {token_identifier, "F", NULL},
                {'(', "(", NULL},
                {token_void, "void", NULL},
                {')', ")", NULL},
                {';', ";", NULL},
                {'\0', "", NULL},
            });
}
