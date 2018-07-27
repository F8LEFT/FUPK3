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
 * Resolve "constant pool" references into pointers to VM structs.
 */
#ifndef DALVIK_OO_RESOLVE_H_
#define DALVIK_OO_RESOLVE_H_

#include "Object.h"

/*
 * "Direct" and "virtual" methods are stored independently.  The type of call
 * used to invoke the method determines which list we search, and whether
 * we travel up into superclasses.
 *
 * (<clinit>, <init>, and methods declared "private" or "static" are stored
 * in the "direct" list.  All others are stored in the "virtual" list.)
 */
enum MethodType {
    METHOD_UNKNOWN  = 0,
    METHOD_DIRECT,      // <init>, private
    METHOD_STATIC,      // static
    METHOD_VIRTUAL,     // virtual, super
    METHOD_INTERFACE    // interface
};

/*
 * Return debug string constant for enum.
 */
const char* dvmMethodTypeStr(MethodType methodType);

#endif  // DALVIK_OO_RESOLVE_H_
