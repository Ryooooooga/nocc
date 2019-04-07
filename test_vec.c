#include "nocc.h"

void test_vec(void) {
    Vec *v;
    int i;

    v = vec_new();

    assert(v->size == 0);

    vec_push(v, (void *)(intptr_t)1);
    vec_push(v, (void *)(intptr_t)2);

    assert(v->size == 2);
    assert((intptr_t)v->data[0] == 1);
    assert((intptr_t)v->data[1] == 2);
    assert((intptr_t)vec_back(v) == 2);

    for (i = 0; i < 100; i++) {
        vec_push(v, (void *)(intptr_t)i);
    }

    assert(v->size == 102);
    assert((intptr_t)vec_back(v) == 99);

    for (i = 0; i < 100; i++) {
        vec_pop(v);
    }

    assert(v->size == 2);
    assert((intptr_t)vec_pop(v) == 2);
    assert((intptr_t)vec_pop(v) == 1);
    assert(v->size == 0);
}
