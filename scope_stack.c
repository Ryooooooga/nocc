#include "nocc.h"

ScopeStack *scope_stack_new(void) {
    ScopeStack *s;

    s = malloc(sizeof(*s));
    s->scopes = vec_new();

    scope_stack_push(s);

    return s;
}

void scope_stack_push(ScopeStack *s) {
    assert(s);

    vec_push(s->scopes, map_new());
}

void scope_stack_pop(ScopeStack *s) {
    assert(s);
    assert(s->scopes->size > 1);

    vec_pop(s->scopes);
}

DeclNode *scope_stack_find(ScopeStack *s, const char *name, bool recursive) {
    DeclNode *decl;
    int i;

    assert(s);
    assert(s->scopes->size > 0);
    assert(name);

    i = s->scopes->size - 1;

    do {
        decl = map_get(s->scopes->data[i], name);

        if (decl != NULL) {
            return decl;
        }
    } while (i-- >= 0 && recursive);

    return NULL;
}

void scope_stack_register(ScopeStack *s, DeclNode *decl) {
    assert(s);
    assert(s->scopes->size > 0);
    assert(decl);

    map_add(s->scopes->data[s->scopes->size - 1], decl->identifier, decl);
}
