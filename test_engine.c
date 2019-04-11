#include "nocc.h"

#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/TargetMachine.h>

void test_engine_run_function(const char *filename, const char *src,
                              const char *func, int param, int result) {
    assert(filename);
    assert(src);
    assert(func);

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
}
