//
// Created by F8LEFT on 2017/2/21.
//

#include <stddef.h>
#include "BaseDef.h"

bool safe_mul(unsigned * rel, unsigned a, unsigned b) {
    if (rel != NULL) {
        *rel = a * b;
    }
    return true;
}
bool safe_add(unsigned * rel, unsigned a, unsigned b) {
    if (rel != NULL) {
        *rel = a + b;
    }
    return true;
}
