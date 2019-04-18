#ifndef INCLUDE_map_h
#define INCLUDE_map_h

#include "vec.h"

typedef struct Map {
    Vec *keys;
    Vec *values;
} Map;

Map *map_new(void);
int map_size(Map *m);
bool map_contains(Map *m, const char *k);
void *map_get(Map *m, const char *k);
void map_add(Map *m, const char *k, void *v);

#endif
