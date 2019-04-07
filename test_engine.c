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
}
