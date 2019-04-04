#ifndef INCLUDE_nocc_h
#define INCLUDE_nocc_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Vec {
    int capacity;
    int size;
    void **data;
};

typedef struct Vec Vec;

Vec *vec_new(void);
void vec_reserve(Vec *v, int capacity);
void vec_resize(Vec *v, int size);
void vec_push(Vec *v, void *value);
void *vec_pop(Vec *v);

#endif
