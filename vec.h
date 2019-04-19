#ifndef INCLUDE_vec_h
#define INCLUDE_vec_h

typedef struct Vec {
    int capacity;
    int size;
    void **data;
} Vec;

Vec *vec_new(void);
void vec_reserve(Vec *v, int capacity);
void vec_resize(Vec *v, int size);
void *vec_back(Vec *v);
void vec_push(Vec *v, void *value);
void *vec_pop(Vec *v);

#endif
