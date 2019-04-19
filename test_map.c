#include "map.h"

void test_map(void) {
    Map *m;

    m = map_new();

    assert(map_size(m) == 0);

    assert(map_contains(m, "a") == false);
    assert(map_contains(m, "b") == false);
    assert(map_contains(m, "aa") == false);

    assert(map_get(m, "a") == NULL);
    assert(map_get(m, "b") == NULL);
    assert(map_get(m, "aa") == NULL);

    map_add(m, "a", (void *)(intptr_t)1);
    map_add(m, "b", (void *)(intptr_t)2);

    assert(map_size(m) == 2);

    assert(map_contains(m, "a") == true);
    assert(map_contains(m, "b") == true);
    assert(map_contains(m, "aa") == false);

    assert((intptr_t)map_get(m, "a") == 1);
    assert((intptr_t)map_get(m, "b") == 2);
    assert(map_get(m, "aa") == NULL);
}
