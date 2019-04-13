void test_vec(void);
void test_map(void);
void test_lexer(void);
void test_preprocessor(void);
void test_parser(void);
void test_generator(void);
void test_engine(void);

int main(void) {
    test_vec();
    test_map();
    test_lexer();
    test_preprocessor();
    test_parser();
    test_generator();
    test_engine();

    return 0;
}
