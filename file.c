#include "nocc.h"

char *read_file(const char *filename) {
    FILE *fp;
    int size;
    char *buffer;

    assert(filename != NULL);

    fp = fopen(filename, "r");

    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buffer = malloc(size + 1);
    fread(buffer, 1, size, fp);
    buffer[size] = '\0';

    fclose(fp);

    return buffer;
}
