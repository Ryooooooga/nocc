#include "nocc.h"

char *str_dup(const char *s) {
    assert(s);
    return str_dup_n(s, strlen(s));
}

char *str_dup_n(const char *s, int length) {
    char *p;

    assert(s);
    assert(length >= 0);

    p = malloc(sizeof(char) * (length + 1));
    strncpy(p, s, length);
    p[length] = '\0';

    return p;
}
