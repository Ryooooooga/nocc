#include "nocc.h"

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/TargetMachine.h>

void test_engine_run_function(const char *filename, const char *src,
                              const char *func, int param, int result) {
    assert(filename);
    assert(src);
    assert(func);

    fprintf(stderr, "test_engine:%s --- ", filename);

    TranslationUnitNode *node = parse(filename, src);
    LLVMModuleRef module = generate(node);

    LLVMExecutionEngineRef engine = NULL;
    char *error = NULL;

    if (LLVMCreateExecutionEngineForModule(&engine, module, &error)) {
        fprintf(stderr, "%s: error %s\n", filename, error);
        exit(1);
    }

    int (*f)(int) = (int (*)(int))LLVMGetFunctionAddress(engine, func);

    int actual = f(param);

    if (actual != result) {
        fprintf(stderr, "%s: %s(%d) expected result %d, but actual %d\n",
                filename, func, param, result, actual);

        LLVMDisposeMessage(error);
        LLVMDisposeExecutionEngine(engine);
        exit(1);
    }

    LLVMDisposeMessage(error);
    LLVMDisposeExecutionEngine(engine);

    fprintf(stderr, "done!!\n");
}

void test_engine(void) {
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMLinkInMCJIT();

    test_engine_run_function("add3",
                             "int add3(int a) {\n"
                             "  return a+3;\n"
                             "}\n",
                             "add3", 5, 8);

    test_engine_run_function("factorial",
                             "int factorial(int n) {\n"
                             "  if (n <= 0) return 1;\n"
                             "  return n*factorial(n-1);\n"
                             "}\n",
                             "factorial", 5, 120);

    test_engine_run_function("variable",
                             "int variable(int n) {\n"
                             "  int a;\n"
                             "  a = n;\n"
                             "  return a;\n"
                             "}\n",
                             "variable", 42, 42);

    test_engine_run_function("variables",
                             "int variables(int n) {\n"
                             "  int a;\n"
                             "  int b;\n"
                             "  a = b = n;\n"
                             "  a = a + 1;"
                             "  return a * b;\n"
                             "}\n",
                             "variables", 4, 20);

    test_engine_run_function("global",
                             "int a;\n"
                             "int global(int n) {\n"
                             "  return a;\n"
                             "}\n",
                             "global", 4, 0);

    test_engine_run_function("global2",
                             "int a;\n"
                             "int global2(int n) {\n"
                             "  a = n;\n"
                             "  return a;\n"
                             "}\n",
                             "global2", 4, 4);

    test_engine_run_function("global2",
                             "int a;\n"
                             "int global2(int n) {\n"
                             "  a = n;\n"
                             "  return a;\n"
                             "}\n",
                             "global2", 4, 4);

    test_engine_run_function("global3",
                             "int a;\n"
                             "int b;\n"
                             "int global3(int n) {\n"
                             "  a = n;\n"
                             "  b = 3;\n"
                             "  return a * b;\n"
                             "}\n",
                             "global3", 4, 12);

    test_engine_run_function("global4",
                             "int *a;\n"
                             "int b;\n"
                             "int global4(int n) {\n"
                             "  a = &n;\n"
                             "  b = 3;\n"
                             "  return *a * b;\n"
                             "}\n",
                             "global4", 4, 12);

    test_engine_run_function("global5",
                             "int *a;\n"
                             "int global5(int n) {\n"
                             "  return a == (int *)0;\n"
                             "}\n",
                             "global5", 0, 1);

    test_engine_run_function("sum",
                             "int sum(int n) {\n"
                             "  int sum;\n"
                             "  int i;\n"
                             "  sum = 0;\n"
                             "  i = 1;\n"
                             "  while (i <= n) {\n"
                             "    sum = sum + i;\n"
                             "    i = i + 1;\n"
                             "  }\n"
                             "  return sum;\n"
                             "}\n",
                             "sum", 100, 5050);

    test_engine_run_function("sum2",
                             "int sum2(int n) {\n"
                             "  int sum;\n"
                             "  int i;\n"
                             "  sum = 0;\n"
                             "  for (i = 1; i <= n; i = i + 1) {\n"
                             "    sum = sum + i;\n"
                             "  }\n"
                             "  return sum;\n"
                             "}\n",
                             "sum2", 100, 5050);

    test_engine_run_function("do_while",
                             "int do_while(int n) {\n"
                             "  do {\n"
                             "    n = n + 1;\n"
                             "  } while (n < 0);\n"
                             "  return n;\n"
                             "}\n",
                             "do_while", 100, 101);

    test_engine_run_function("do_while2",
                             "int do_while2(int n) {\n"
                             "  do {\n"
                             "    n = n + 1;\n"
                             "  } while (n < 0);\n"
                             "  return n;\n"
                             "}\n",
                             "do_while2", -10, 0);

    test_engine_run_function("break",
                             "int break_(int n) {\n"
                             "  for (;;) break;\n"
                             "  return n;\n"
                             "}\n",
                             "break_", 42, 42);

    test_engine_run_function("continue",
                             "int continue_(int n) {\n"
                             "  int a; int i;\n"
                             "  a = 0;\n"
                             "  for (i = 0; i < n; i = i + 1) {\n"
                             "    if (i < 5) continue;\n"
                             "    a = a + i;\n"
                             "  }\n"
                             "  return a;\n"
                             "}\n",
                             "continue_", 10, 35);

    test_engine_run_function("pointer",
                             "int pointer(int n) {\n"
                             "  int a;\n"
                             "  *&a = n;\n"
                             "  return a;\n"
                             "}\n",
                             "pointer", 10, 10);

    test_engine_run_function("pointer2",
                             "int pointer2(int n) {\n"
                             "  int a;\n"
                             "  int *p;\n"
                             "  p = &a;\n"
                             "  *p = n + 2;\n"
                             "  return a;\n"
                             "}\n",
                             "pointer2", 10, 12);

    test_engine_run_function("pointer3",
                             "int pointer3(int n) {\n"
                             "  int a;\n"
                             "  int *p;\n"
                             "  int **pp;\n"
                             "  p = &a;\n"
                             "  pp = &p;\n"
                             "  **pp = n + 2;\n"
                             "  return a;\n"
                             "}\n",
                             "pointer3", 10, 12);

    test_engine_run_function("pointer4",
                             "int *f(int *p, int a) {\n"
                             "  *p = a;\n"
                             "  return p;\n"
                             "}\n"
                             "int pointer4(int n) {\n"
                             "  int a;\n"
                             "  return *f(&a, n);\n"
                             "}\n",
                             "pointer4", 42, 42);

    test_engine_run_function("struct",
                             "int struct_(int n) {\n"
                             "  struct tag {\n"
                             "    int x;\n"
                             "    int y;\n"
                             "  } a;\n"
                             "  struct tag b;\n"
                             "  a.x = 10;\n"
                             "  a.y = n;\n"
                             "  b = a;\n"
                             "  return b.x * b.y;\n"
                             "}\n",
                             "struct_", 42, 420);

    test_engine_run_function("struct2",
                             "struct tag {\n"
                             "  int x;\n"
                             "  int y;\n"
                             "} f(int x, int y) {\n"
                             "  struct tag a;\n"
                             "  a.x = x;\n"
                             "  a.y = y;\n"
                             "  return a;\n"
                             "}\n"
                             "int struct2(int n) {\n"
                             "  return f(n, 2 * n).y;\n"
                             "}\n",
                             "struct2", 42, 84);

    test_engine_run_function("struct3",
                             "struct a {\n"
                             "  int x;\n"
                             "  int y;\n"
                             "};\n"
                             "int struct3(int n) {\n"
                             "  struct a a;\n"
                             "  a.x = n;\n"
                             "  a.y = 3;\n"
                             "  return a.x + a.y;\n"
                             "}\n",
                             "struct3", 42, 45);

    test_engine_run_function("struct4",
                             "int struct4(int n) {\n"
                             "  struct a { int x; };\n"
                             "  struct a a;\n"
                             "  a.x = n;\n"
                             "  return a.x;\n"
                             "}\n",
                             "struct4", 42, 42);

    test_engine_run_function("typedef",
                             "int typedef_(int n) {\n"
                             "  typedef struct a {int x;} a;\n"
                             "  a b;\n"
                             "  b.x = n;\n"
                             "  return b.x;\n"
                             "}\n",
                             "typedef_", 42, 42);

    test_engine_run_function("typedef2",
                             "typedef int a;\n"
                             "int typedef2(a n) {\n"
                             "  return n;\n"
                             "}\n",
                             "typedef2", 42, 42);

    test_engine_run_function("cast",
                             "int cast(int n) {\n"
                             "  int *p;\n"
                             "  p = (void*)0;\n"
                             "  (void)n;\n"
                             "  return 9;\n"
                             "}\n",
                             "cast", 42, 9);

    test_engine_run_function("char1",
                             "char c;\n"
                             "int char1(int n) {\n"
                             "  return c;\n"
                             "}\n",
                             "char1", 0, 0);

    test_engine_run_function("char2",
                             "int char2(int n) {\n"
                             "  char c;\n"
                             "  c = n;\n"
                             "  return c;\n"
                             "}\n",
                             "char2", 5, 5);

    test_engine_run_function("char3",
                             "int char3(int n) {\n"
                             "  char c;\n"
                             "  c = n;\n"
                             "  n = c;\n"
                             "  return n + (int)c;\n"
                             "}\n",
                             "char3", 5, 10);

    test_engine_run_function("string",
                             "int strlen(const char *s);\n"
                             "int string(int n) {\n"
                             "  return strlen(\"Hello, world!\\n\");\n"
                             "}\n",
                             "string", 0, 14);

    test_engine_run_function("positive",
                             "int positive(int n) {\n"
                             "  return +n;\n"
                             "}\n",
                             "positive", 8, 8);

    test_engine_run_function("negative",
                             "int negative(int n) {\n"
                             "  return -n;\n"
                             "}\n",
                             "negative", 8, -8);

    test_engine_run_function("preinc",
                             "int preinc(int n) {\n"
                             "  int a;\n"
                             "  int b;\n"
                             "  a = n;\n"
                             "  b = ++a;\n"
                             "  return (a == n + 1) * (b == n + 1);\n"
                             "}\n",
                             "preinc", 8, 1);

    test_engine_run_function("predec",
                             "int predec(int n) {\n"
                             "  int a;\n"
                             "  int b;\n"
                             "  a = n;\n"
                             "  b = --a;\n"
                             "  return (a == n - 1) * (b == n - 1);\n"
                             "}\n",
                             "predec", 8, 1);

    test_engine_run_function("postinc",
                             "int postinc(int n) {\n"
                             "  int a;\n"
                             "  int b;\n"
                             "  a = n;\n"
                             "  b = a++;\n"
                             "  return (a == n + 1) * (b == n);\n"
                             "}\n",
                             "postinc", 8, 1);

    test_engine_run_function("postdec",
                             "int postdec(int n) {\n"
                             "  int a;\n"
                             "  int b;\n"
                             "  a = n;\n"
                             "  b = a--;\n"
                             "  return (a == n - 1) * (b == n);\n"
                             "}\n",
                             "postdec", 8, 1);

    test_engine_run_function("var_args",
                             "void *malloc(int s);\n"
                             "void free(void *p);\n"
                             "int strcmp(const char *a, const char* b);\n"
                             "int sprintf(char *p, const char *f, ...);\n"
                             "int var_args(int n) {\n"
                             "  char *p;\n"
                             "  int res;\n"
                             "  p = malloc(100);\n"
                             "  sprintf(p, \"%d\", n);\n"
                             "  res = strcmp(p, \"50\");\n"
                             "  free(0);\n"
                             "  return res;\n"
                             "return n;"
                             "}\n",
                             "var_args", 50, 0);

    test_engine_run_function("forward",
                             "int f(void);\n"
                             "int forward(int n) {\n"
                             "  return f();"
                             "}\n"
                             "int f(void) {\n"
                             "  return 42;"
                             "}\n",
                             "forward", 0, 42);

    test_engine_run_function("ptrref",
                             "int ptrref(int n) {\n"
                             "  return *\"test\";\n"
                             "}\n",
                             "ptrref", 0, *"test");

    test_engine_run_function("ptradd",
                             "int ptradd(int n) {\n"
                             "  return *(\"test\" + n);\n"
                             "}\n",
                             "ptradd", 2, *("test" + 2));

    test_engine_run_function("ptrref2",
                             "int ptrref2(int n) {\n"
                             "  return \"test\"[n];\n"
                             "}\n",
                             "ptrref2", 3, "test"[3]);
}
