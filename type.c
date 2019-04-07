#include "nocc.h"

Type void_ = {type_void};
Type int32 = {type_int32};

Type *type_get_void(void) {
    return &void_;
}

Type *type_get_int32(void) {
    return &int32;
}

Type *function_type_new(Type *return_type, Type **param_types, int num_params) {
    FunctionType *t;
    int i;

    assert(return_type);
    assert(num_params >= 0);
    assert(param_types || num_params == 0);

    t = malloc(sizeof(*t));
    t->kind = type_function;
    t->return_type = return_type;
    t->param_types = malloc(sizeof(Type *) * num_params);
    t->num_params = num_params;

    for (i = 0; i < num_params; i++) {
        t->param_types[i] = param_types[i];
    }

    return (Type *)t;
}

bool is_void_type(Type *t) {
    assert(t);
    return t->kind == type_void;
}

bool is_int32_type(Type *t) {
    assert(t);
    return t->kind == type_int32;
}

bool is_function_type(Type *t) {
    assert(t);
    return t->kind == type_function;
}

Type *function_return_type(Type *t) {
    assert(t);

    if (!is_function_type(t)) {
        return NULL;
    }

    return ((FunctionType *)t)->return_type;
}
