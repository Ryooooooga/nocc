#include "nocc.h"

void test_vec(void);
void test_map(void);
void test_lexer(void);
void test_preprocessor(Vec *include_directories);
void test_parser(void);
void test_generator(void);
void test_engine(void);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s <test directory>\n", argv[0]);
        exit(1);
    }

    Vec *include_directories = vec_new();
    vec_push(include_directories, argv[1]);

    test_vec();
    test_map();
    test_lexer();
    test_preprocessor(include_directories);
    test_parser();
    test_generator();
    test_engine();

    return 0;
}
