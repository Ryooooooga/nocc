#include "nocc.h"

int main(int argc, char **argv) {
    const char *filename;
    char *src;
    TranslationUnitNode *node;
    LLVMModuleRef module;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    filename = argv[1];
    src = read_file(filename);

    if (src == NULL) {
        fprintf(stderr, "cannot open file %s\n", filename);
        exit(1);
    }

    node = parse(filename, src, vec_new());
    module = generate(node);

    LLVMDumpModule(module);

    LLVMDisposeModule(module);

    return 0;
}
