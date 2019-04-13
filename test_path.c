#include "nocc.h"

void test_path_join(const char *dir, const char *name, const char *expected) {
    char *path;

    assert(dir != NULL);
    assert(name != NULL);
    assert(expected != NULL);

    path = path_join(dir, name);

    if (strcmp(path, expected) != 0) {
        fprintf(stderr, "path is expected '%s', but got '%s'\n", expected,
                path);
        exit(1);
    }
}

void test_path_dir(const char *path, const char *expected) {
    char *dir;

    assert(path != NULL);
    assert(expected != NULL);

    dir = path_dir(path);

    if (strcmp(dir, expected) != 0) {
        fprintf(stderr, "dir is expected '%s', but got %s\n", expected, dir);
        exit(1);
    }
}

void test_path(void) {
    test_path_join("", "file", "file");
    test_path_join(".", "file", "./file");
    test_path_join("..", "file", "../file");
    test_path_join("/", "file", "/file");
    test_path_join("dir", "file", "dir/file");
    test_path_join("dir/", "file", "dir/file");
    test_path_join("dir/", "./file", "dir/./file");
    test_path_join("dir", "./file", "dir/./file");
    test_path_join("dir/", "./file", "dir/./file");
    test_path_join("dir", "../file", "dir/../file");
    test_path_join("dir/", "../file", "dir/../file");

    test_path_dir("", "");
    test_path_dir(".", "");
    test_path_dir("a.c", "");
    test_path_dir("/a.c", "/");
    test_path_dir("./a.c", "./");
    test_path_dir("../a.c", "../");
    test_path_dir("dir/a.c", "dir/");
    test_path_dir("dir/dir2/a.c", "dir/dir2/");
    test_path_dir("./dir/dir2/a.c", "./dir/dir2/");
    test_path_dir("./dir/dir2/./a.c", "./dir/dir2/./");
    test_path_dir("/", "/");
    test_path_dir("./", "./");
    test_path_dir("../", "../");
    test_path_dir("dir/", "dir/");
    test_path_dir("dir/dir2/", "dir/dir2/");
}
