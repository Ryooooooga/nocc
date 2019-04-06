#include "nocc.h"

int main(int argc, char *argv[]) {
    TranslationUnitNode *node;
    LLVMModuleRef module;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <src>\n", argv[0]);
        return 1;
    }

    node = parse("<stdin>", argv[1]);
    module = generate(node);

    LLVMDumpModule(module);

    LLVMDisposeModule(module);

    return 0;
}
