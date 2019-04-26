#include "nocc.h"

ScopeStack *scope_stack_new(void) {
    ScopeStack *s;

    s = malloc(sizeof(*s));
    s->scopes = vec_new();

    scope_stack_push(s);

    return s;
}

int scope_stack_depth(ScopeStack *s) {
    assert(s != NULL);

    return s->scopes->size;
}

void scope_stack_push(ScopeStack *s) {
    assert(s != NULL);

    vec_push(s->scopes, map_new());
}

void scope_stack_pop(ScopeStack *s) {
    assert(s != NULL);
    assert(s->scopes->size > 1);

    vec_pop(s->scopes);
}

Symbol *scope_stack_find(ScopeStack *s, const char *name, bool recursive) {
    Symbol *value;
    int i;

    assert(s != NULL);
    assert(s->scopes->size > 0);
    assert(name != NULL);

    i = s->scopes->size - 1;

    do {
        value = map_get(s->scopes->data[i], name);

        if (value != NULL) {
            return value;
        }
    } while (--i >= 0 && recursive);

    return NULL;
}

void scope_stack_register(ScopeStack *s, Symbol *symbol) {
    assert(s != NULL);
    assert(s->scopes->size > 0);
    assert(symbol != NULL);

    map_add(vec_back(s->scopes), symbol->identifier, symbol);
}
