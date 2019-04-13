#include "nocc.h"

char *read_file(const char *filename) {
    FILE *fp;
    int size;
    char *buffer;

    assert(filename);

    fp = fopen(filename, "r");

    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(size);
    fread(buffer, 1, size, fp);

    fclose(fp);

    return buffer;
}
