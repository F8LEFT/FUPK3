/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Resolve classes, methods, fields, and strings.
 *
 * According to the VM spec (v2 5.5), classes may be initialized by use
 * of the "new", "getstatic", "putstatic", or "invokestatic" instructions.
 * If we are resolving a static method or static field, we make the
 * initialization check here.
 *
 * (NOTE: the verifier has its own resolve functions, which can be invoked
 * if a class isn't pre-verified.  Those functions must not update the
 * "resolved stuff" tables for static fields and methods, because they do
 * not perform initialization.)
 */
#include "Resolve.h"
#include <stdlib.h>
/*
 * For debugging: return a string representing the methodType.
 */
const char* dvmMethodTypeStr(MethodType methodType)
{
    switch (methodType) {
    case METHOD_DIRECT:         return "direct";
    case METHOD_STATIC:         return "static";
    case METHOD_VIRTUAL:        return "virtual";
    case METHOD_INTERFACE:      return "interface";
    case METHOD_UNKNOWN:        return "UNKNOWN";
    }
    assert(false);
    return "BOGUS";
}
