#include "nocc.h"

Type *type_get_void(void) {
    Type *t;

    t = malloc(sizeof(*t));
    t->kind = type_void;

    return t;
}

Type *type_get_int8(void) {
    Type *t;

    t = malloc(sizeof(*t));
    t->kind = type_int8;

    return t;
}

Type *type_get_int32(void) {
    Type *t;

    t = malloc(sizeof(*t));
    t->kind = type_int32;

    return t;
}

Type *pointer_type_new(Type *element_type) {
    PointerType *t;

    assert(element_type != NULL);

    t = malloc(sizeof(*t));
    t->kind = type_pointer;
    t->element_type = element_type;

    return (Type *)t;
}

Type *array_type_new(Type *element_type, int length) {
    ArrayType *t;

    assert(element_type != NULL);
    assert(length >= 1);

    t = malloc(sizeof(*t));
    t->kind = type_array;
    t->element_type = element_type;
    t->length = length;

    return (Type *)t;
}

Type *function_type_new(Type *return_type, Type **param_types, int num_params,
                        bool var_args) {
    FunctionType *t;
    int i;

    assert(return_type != NULL);
    assert(num_params >= 0);
    assert(param_types != NULL || num_params == 0);

    t = malloc(sizeof(*t));
    t->kind = type_function;
    t->return_type = return_type;
    t->param_types = malloc(sizeof(Type *) * num_params);
    t->num_params = num_params;
    t->var_args = var_args;

    for (i = 0; i < num_params; i++) {
        t->param_types[i] = param_types[i];
    }

    return (Type *)t;
}

bool type_equals(Type *a, Type *b) {
    int i;

    assert(a != NULL);
    assert(b != NULL);

    if (a == b) {
        return true;
    }

    if (a->kind != b->kind) {
        return false;
    }

    switch (a->kind) {
    case type_void:
    case type_int8:
    case type_int32:
        return true;

    case type_pointer:
        return type_equals(pointer_element_type(a), pointer_element_type(b));

    case type_array:
        return array_type_count_elements(a) == array_type_count_elements(b) &&
               type_equals(array_element_type(a), array_element_type(b));

    case type_function:
        if (!type_equals(function_return_type(a), function_return_type(b))) {
            return false;
        }

        if (function_count_param_types(a) != function_count_param_types(b)) {
            return false;
        }

        for (i = 0; i < function_count_param_types(a); i++) {
            if (!type_equals(function_param_type(a, i),
                             function_param_type(b, i))) {
                return false;
            }
        }

        return true;

    case type_struct:
        return false;

    default:
        fprintf(stderr, "unknown type %d\n", a->kind);
        exit(1);
    }
}

bool is_void_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_void;
}

bool is_int8_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_int8;
}

bool is_int32_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_int32;
}

bool is_pointer_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_pointer;
}

bool is_array_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_array;
}

bool is_function_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_function;
}

bool is_struct_type(Type *t) {
    assert(t != NULL);
    return t->kind == type_struct;
}

bool is_incomplete_type(Type *t) {
    assert(t != NULL);

    switch (t->kind) {
    case type_void:
    case type_function:
        return true;

    case type_array:
        return is_incomplete_type(array_element_type(t));

    case type_struct:
        return ((StructType *)t)->is_incomplete;

    default:
        return false;
    }
}

bool is_void_pointer_type(Type *t) {
    assert(t != NULL);
    return is_pointer_type(t) && is_void_type(pointer_element_type(t));
}

bool is_function_pointer_type(Type *t) {
    assert(t != NULL);
    return is_pointer_type(t) && is_function_type(pointer_element_type(t));
}

bool is_incomplete_pointer_type(Type *t) {
    assert(t != NULL);
    return is_pointer_type(t) && is_incomplete_type(pointer_element_type(t));
}

bool is_integer_type(Type *t) {
    assert(t != NULL);
    return is_int8_type(t) || is_int32_type(t);
}

bool is_scalar_type(Type *t) {
    assert(t != NULL);
    return is_integer_type(t) || is_pointer_type(t);
}

Type *pointer_element_type(Type *t) {
    assert(t != NULL);

    if (!is_pointer_type(t)) {
        return NULL;
    }

    return ((PointerType *)t)->element_type;
}

Type *array_element_type(Type *t) {
    assert(t != NULL);

    if (!is_array_type(t)) {
        return NULL;
    }

    return ((ArrayType *)t)->element_type;
}

int array_type_count_elements(Type *t) {
    assert(t != NULL);

    if (!is_array_type(t)) {
        return -1;
    }

    return ((ArrayType *)t)->length;
}

Type *function_return_type(Type *t) {
    assert(t != NULL);

    if (!is_function_type(t)) {
        return NULL;
    }

    return ((FunctionType *)t)->return_type;
}

int function_count_param_types(Type *t) {
    assert(t != NULL);

    if (!is_function_type(t)) {
        return -1;
    }

    return ((FunctionType *)t)->num_params;
}

Type *function_param_type(Type *t, int index) {
    assert(t != NULL);
    assert(index >= 0);
    assert(index < function_count_param_types(t));

    return ((FunctionType *)t)->param_types[index];
}

bool function_type_is_var_args(Type *t) {
    assert(t != NULL);

    if (!is_function_type(t)) {
        return false;
    }

    return ((FunctionType *)t)->var_args;
}

int struct_type_count_members(Type *t) {
    assert(t != NULL);

    if (!is_struct_type(t)) {
        return -1;
    }

    return ((StructType *)t)->num_members;
}

struct MemberNode *struct_type_member(Type *t, int index) {
    assert(t != NULL);
    assert(index >= 0);
    assert(index < struct_type_count_members(t));

    return ((StructType *)t)->members[index];
}

struct MemberNode *struct_type_find_member(Type *t, const char *member_name,
                                           int *index) {
    MemberNode *member;

    assert(t != NULL);
    assert(member_name != NULL);
    assert(index != NULL);

    if (!is_struct_type(t)) {
        return NULL;
    }

    for (*index = 0; struct_type_count_members(t); (*index)++) {
        member = struct_type_member(t, *index);

        if (strcmp(member->symbol->identifier, member_name) == 0) {
            return member;
        }
    }

    *index = -1;
    return NULL;
}
