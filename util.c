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

char *str_cat_n(const char *s1, int len1, const char *s2, int len2) {
    char *p;

    assert(s1);
    assert(len1 >= 0);
    assert(s2);
    assert(len2 >= 0);

    p = malloc(sizeof(char) * (len1 + len2 + 1));
    strncpy(p, s1, len1);
    strncpy(p + len1, s2, len2);
    p[len1 + len2] = '\0';

    return p;
}
