#ifndef USE_STANDARD_HEADERS
#define USE_STANDARD_HEADERS
#endif

#include "nocc.h"

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/TargetMachine.h>

void test_engine_run_function(const char *filename, const char *src,
                              const char *func, int param, int result) {
    assert(filename != NULL);
    assert(src != NULL);
    assert(func != NULL);

    fprintf(stderr, "test_engine:%s --- ", filename);

    TranslationUnitNode *node = parse(filename, src, vec_new());
    LLVMModuleRef module = generate(node);

    if (LLVMGetNamedFunction(module, func) == NULL) {
        fprintf(stderr, "%s: function %s not found\n", filename, func);
        exit(1);
    }

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

    test_engine_run_function("ptradd2",
                             "int ptradd2(int n) {\n"
                             "  return *(n + \"test\");\n"
                             "}\n",
                             "ptradd2", 2, *(2 + "test"));

    test_engine_run_function("ptradd3",
                             "int ptradd3(int n) {\n"
                             "  int *a;\n"
                             "  a = 0;\n"
                             "  return a + n;\n"
                             "}\n",
                             "ptradd3", 2, 2 * 4);

    test_engine_run_function("ptrsub",
                             "int ptrsub(int n) {\n"
                             "  return *((\"test\" + n) - 2);\n"
                             "}\n",
                             "ptrsub", 4, *("test" + 4 - 2));

    test_engine_run_function("ptrdiff",
                             "int ptrdiff(int n) {\n"
                             "  const char* p;\n"
                             "  const char* q;\n"
                             "  p = \"test\";\n"
                             "  q = p + n;"
                             "  return q - p;\n"
                             "}\n",
                             "ptrdiff", 3, 3);

    test_engine_run_function("ptrdiff2",
                             "int ptrdiff2(int n) {\n"
                             "  int* p;\n"
                             "  int* q;\n"
                             "  p = (int *)0;\n"
                             "  q = (int *)8;"
                             "  return q - p;\n"
                             "}\n",
                             "ptrdiff2", 0, 2);

    test_engine_run_function("index",
                             "int index(int n) {\n"
                             "  return \"test\"[n];\n"
                             "}\n",
                             "index", 3, "test"[3]);

    test_engine_run_function("index2",
                             "int index2(int n) {\n"
                             "  int *a;\n"
                             "  a = &n;\n"
                             "  a[0] = n + 1;\n"
                             "  return n == a[0];\n"
                             "}\n",
                             "index2", 5, 1);

    test_engine_run_function("array",
                             "int array(int n) {\n"
                             "  int a[3];\n"
                             "\"a\"[0] = 0;"
                             "  a[0] = 2;\n"
                             "  a[1] = 3;\n"
                             "  a[2] = 5;\n"
                             "  return a[0] * a[1] * a[2];\n"
                             "}\n",
                             "array", 0, 30);

    test_engine_run_function("param_array",
                             "int param_array(int n[1]) {\n"
                             "  return (int)n;\n"
                             "}\n",
                             "param_array", 30, 30);

    test_engine_run_function("global_array",
                             "int a[3];\n"
                             "int global_array(int n) {\n"
                             "  a[0] = 2;\n"
                             "  a[1] = 3;\n"
                             "  a[2] = 5;\n"
                             "  return a[0] * a[1] * a[2];\n"
                             "}\n",
                             "global_array", 0, 30);

    test_engine_run_function("multi_array",
                             "int multi_array(int n) {\n"
                             "  int a[2][2];\n"
                             "  a[0][0] = 2;\n"
                             "  a[0][1] = 3;\n"
                             "  a[1][0] = 5;\n"
                             "  a[1][1] = 7;\n"
                             "  return a[0][0] * a[0][1] * a[1][0] * a[1][1];\n"
                             "}\n",
                             "multi_array", 0, 210);

    test_engine_run_function("struct_array",
                             "int struct_array(int n) {\n"
                             "  struct t {int x[2];} a[2];\n"
                             "  a[0].x[0] = 2;\n"
                             "  a[0].x[1] = 3;\n"
                             "  a[1].x[0] = 5;\n"
                             "  a[1].x[1] = 7;\n"
                             "  return a[0].x[0] * a[0].x[1]\n"
                             "       * a[1].x[0] * a[1].x[1];\n"
                             "}\n",
                             "struct_array", 0, 210);

    test_engine_run_function("typedef_array",
                             "int typedef_array(int n) {\n"
                             "  typedef int t[2];\n"
                             "  t a;\n"
                             "  a[0] = 3;\n"
                             "  a[1] = 5;\n"
                             "  return a[0] * a[1];\n"
                             "}\n",
                             "typedef_array", 0, 15);

    test_engine_run_function("arrow",
                             "int arrow(int n) {\n"
                             "  struct t {int x;} a[1];\n"
                             "  a->x = n;\n"
                             "  return a->x;\n"
                             "}\n",
                             "arrow", 42, 42);

    extern int test_extern;
    test_extern = 42;
    test_engine_run_function("extern",
                             "extern int test_extern;\n"
                             "int extern_(int n) {\n"
                             "  if (test_extern != 42) return 0;\n"
                             "  test_extern = 33;\n"
                             "  return test_extern;\n"
                             "}\n",
                             "extern_", 0, 33);
    assert(test_extern == 33);

    test_extern = 42;
    test_engine_run_function("extern2",
                             "extern int test_extern;\n"
                             "extern int test_extern;\n"
                             "int extern2(int n) {\n"
                             "  if (test_extern != 42) return 0;\n"
                             "  test_extern = 33;\n"
                             "  return test_extern;\n"
                             "}\n",
                             "extern2", 0, 33);
    assert(test_extern == 33);

    test_engine_run_function("extern3",
                             "extern int test_extern3;\n"
                             "extern int test_extern3;\n"
                             "int extern3(int n) {\n"
                             "  test_extern3 = 33;\n"
                             "  return test_extern3;\n"
                             "}\n"
                             "int test_extern3;\n",
                             "extern3", 0, 33);

    test_engine_run_function("extern4",
                             "extern int test_extern4;\n"
                             "int test_extern4;\n"
                             "extern int test_extern4;\n"
                             "int extern4(int n) {\n"
                             "  test_extern4 = 33;\n"
                             "  return test_extern4;\n"
                             "}\n",
                             "extern4", 0, 33);

    test_engine_run_function("sizeof1",
                             "int sizeof1(int n) {\n"
                             "  return sizeof((char)0);\n"
                             "}\n",
                             "sizeof1", 0, 1);

    test_engine_run_function("sizeof2",
                             "int sizeof2(int n) {\n"
                             "  return sizeof((int)0);\n"
                             "}\n",
                             "sizeof2", 0, 4);

    test_engine_run_function("sizeof3",
                             "int sizeof3(int n) {\n"
                             "  return sizeof((int *)0);\n"
                             "}\n",
                             "sizeof3", 0, 8);

    test_engine_run_function("sizeof4",
                             "int sizeof4(int n) {\n"
                             "  return sizeof((void *)0);\n"
                             "}\n",
                             "sizeof4", 0, 8);

    test_engine_run_function("sizeof5",
                             "int sizeof5(int n) {\n"
                             "  struct t {int a; char b; int *c;} a;\n"
                             "  return sizeof(a);\n"
                             "}\n",
                             "sizeof5", 0, 16);

    test_engine_run_function("character",
                             "int character(int n) {\n"
                             "  return 'a';\n"
                             "}\n",
                             "character", 0, 'a');

    test_engine_run_function("character",
                             "int character(int n) {\n"
                             "  return '\\n';\n"
                             "}\n",
                             "character", 0, '\n');

    test_engine_run_function("logical_and",
                             "int logical_and(int n) {\n"
                             "  return ((0 && 0) == 0) * ((1 && 0) == 0) *"
                             "         ((0 && 1) == 0) * ((1 && 1) == 1);\n"
                             "}\n",
                             "logical_and", 0, 1);

    test_engine_run_function("logical_or",
                             "int logical_or(int n) {\n"
                             "  return ((0 && 0) == 0) * ((1 && 0) == 1) *"
                             "         ((0 && 1) == 1) * ((1 && 1) == 1);\n"
                             "}\n",
                             "logical_or", 0, 1);

    test_engine_run_function("shortcircuit1",
                             "int a;\n"
                             "int f(void) {return ++a;}\n"
                             "int shortcircuit1(int n) {\n"
                             "  0 && f();\n"
                             "  return a == 0;\n"
                             "}\n",
                             "shortcircuit1", 0, 1);

    test_engine_run_function("shortcircuit2",
                             "int a;\n"
                             "int f(void) {return ++a;}\n"
                             "int shortcircuit2(int n) {\n"
                             "  1 && f();\n"
                             "  return a == 1;\n"
                             "}\n",
                             "shortcircuit2", 0, 1);

    test_engine_run_function("shortcircuit3",
                             "int a;\n"
                             "int f(void) {return ++a;}\n"
                             "int shortcircuit3(int n) {\n"
                             "  0 || f();\n"
                             "  return a == 1;\n"
                             "}\n",
                             "shortcircuit3", 0, 1);

    test_engine_run_function("shortcircuit4",
                             "int a;\n"
                             "int f(void) {return ++a;}\n"
                             "int shortcircuit4(int n) {\n"
                             "  1 || f();\n"
                             "  return a == 0;\n"
                             "}\n",
                             "shortcircuit4", 0, 1);
}

int test_extern = 24;
