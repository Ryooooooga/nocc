#include "nocc.h"

Vec *preprocess(const char *filename, const char *src) {
    (void)filename;
    return lex(src);
}
