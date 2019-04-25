#include "nocc.h"

VariableSymbol *variable_symbol_new(const char *filename, int line,
                                    const char *identifier, Type *type) {
    VariableSymbol *p;

    assert(filename != NULL);
    assert(identifier != NULL);
    assert(type != NULL);

    p = malloc(sizeof(*p));
    p->kind = symbol_variable;
    p->filename = str_dup(filename);
    p->line = line;
    p->identifier = str_dup(identifier);
    p->type = type;
    p->generated_location = NULL;

    return p;
}

Symbol *type_symbol_new(const char *filename, int line, const char *identifier,
                        Type *type) {
    Symbol *p;

    assert(filename != NULL);
    assert(identifier != NULL);
    assert(type != NULL);

    p = malloc(sizeof(*p));
    p->kind = symbol_type;
    p->filename = str_dup(filename);
    p->line = line;
    p->identifier = str_dup(identifier);
    p->type = type;

    return p;
}
