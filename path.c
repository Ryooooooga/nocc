#include "nocc.h"

char *path_join(const char *dir, const char *filename) {
    char *path;
    int dir_len;
    int name_len;

    assert(dir);
    assert(filename);

    dir_len = strlen(dir);
    name_len = strlen(filename);

    if (dir_len == 0) {
        return str_dup(filename);
    }

    if (dir_len > 0 && dir[dir_len - 1] == '/') {
        dir_len -= 1;
    }

    path = malloc(sizeof(char) * (dir_len + 1 + name_len + 1));
    strncpy(path, dir, dir_len);
    path[dir_len] = '/';
    strncpy(path + dir_len + 1, filename, name_len);
    path[dir_len + 1 + name_len] = '\0';

    return path;
}

char *path_dir(const char *path) {
    int len;

    assert(path);

    len = strlen(path);

    while (len > 0 && path[len - 1] != '/') {
        len -= 1;
    }

    return str_dup_n(path, len);
}
