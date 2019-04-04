#include "nocc.h"

Vec *vec_new(void) {
    Vec *v = malloc(sizeof(*v));
    v->capacity = 8;
    v->size = 0;
    v->data = malloc(sizeof(void *) * v->capacity);

    return v;
}

void vec_reserve(Vec *v, int capacity) {
    assert(v);
    assert(capacity >= 0);

    if (capacity > v->capacity) {
        v->capacity = capacity;
        v->data = realloc(v->data, sizeof(void *) * v->capacity);
    }
}

void vec_resize(Vec *v, int size) {
    assert(v);
    assert(size >= 0);

    vec_reserve(v, size);
    v->size = size;
}

void vec_push(Vec *v, void *value) {
    assert(v);

    if (v->capacity == v->size) {
        vec_reserve(v, v->capacity * 2);
    }

    v->data[v->size++] = value;
}

void *vec_pop(Vec *v) {
    assert(v);
    assert(v->size > 0);

    return v->data[--v->size];
}
