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
}
