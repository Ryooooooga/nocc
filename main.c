#include "nocc.h"

#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>

int main(int argc, char *argv[]) {
    int value;
    LLVMBuilderRef builder;
    LLVMModuleRef module;
    LLVMTypeRef functionType;
    LLVMValueRef function;
    LLVMBasicBlockRef entryBasicBlock;
    LLVMValueRef val;
    char *error;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <src>\n", argv[0]);
        return 1;
    }

    value = atoi(argv[1]);

    builder = LLVMCreateBuilder();
    module = LLVMModuleCreateWithName("main");
    functionType = LLVMFunctionType(LLVMInt32Type(), NULL, 0, 0);
    function = LLVMAddFunction(module, "main", functionType);
    entryBasicBlock = LLVMAppendBasicBlock(function, "entry");

    LLVMPositionBuilderAtEnd(builder, entryBasicBlock);
    val = LLVMConstInt(LLVMInt32Type(), value, 1);
    LLVMBuildRet(builder, val);

    if (LLVMVerifyModule(module, LLVMPrintMessageAction, &error)) {
        fprintf(stderr, "error: %s\n", error);
    }
    LLVMDisposeMessage(error);

    LLVMDumpModule(module);

    LLVMDisposeModule(module);
    LLVMDisposeBuilder(builder);

    return 0;
}
