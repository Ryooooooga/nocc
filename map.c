#include "nocc.h"

Map *map_new(void) {
    Map *m;

    m = malloc(sizeof(*m));
    m->keys = vec_new();
    m->values = vec_new();

    return m;
}

int map_size(Map *m) {
    assert(m);
    assert(m->keys->size == m->values->size);

    return m->keys->size;
}

void map_shrink(Map *m, int size) {
    assert(m);
    assert(size >= 0);
    assert(size <= map_size(m));

    vec_resize(m->keys, size);
    vec_resize(m->values, size);
}

bool map_contains(Map *m, const char *k) {
    int i;

    assert(m);
    assert(k);

    for (i = map_size(m) - 1; i >= 0; i--) {
        if (strcmp(m->keys->data[i], k) == 0) {
            return true;
        }
    }

    return false;
}

void *map_get(Map *m, const char *k) {
    int i;

    assert(m);
    assert(k);

    for (i = map_size(m) - 1; i >= 0; i--) {
        if (strcmp(m->keys->data[i], k) == 0) {
            return m->values->data[i];
        }
    }

    return NULL;
}

void map_add(Map *m, const char *k, void *v) {
    assert(m);
    assert(k);
    assert(m->keys->size == m->values->size);

    vec_push(m->keys, strdup(k));
    vec_push(m->values, v);
}
