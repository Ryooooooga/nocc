#include "nocc.h"

Type void_ = {type_void};
Type int32 = {type_int32};

Type *type_get_void(void) {
    return &void_;
}

Type *type_get_int32(void) {
    return &int32;
}
