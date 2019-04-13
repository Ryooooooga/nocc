#include "nocc.h"

int main(int argc, char *argv[]) {
    const char *filename;
    char *src;
    TranslationUnitNode *node;
    LLVMModuleRef module;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    src = read_file(filename);

    node = parse(filename, src);
    module = generate(node);

    LLVMDumpModule(module);

    LLVMDisposeModule(module);

    return 0;
}
