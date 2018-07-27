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
 * Class loader.
 */
#ifndef DALVIK_OO_CLASS_H_
#define DALVIK_OO_CLASS_H_

#include "Object.h"

/*
 * The classpath and bootclasspath differ in that only the latter is
 * consulted when looking for classes needed by the VM.  When searching
 * for an arbitrary class definition, we start with the bootclasspath,
 * look for optional packages (a/k/a standard extensions), and then try
 * the classpath.
 *
 * In Dalvik, a class can be found in one of two ways:
 *  - in a .dex file
 *  - in a .dex file named specifically "classes.dex", which is held
 *    inside a jar file
 *
 * These two may be freely intermixed in a classpath specification.
 * Ordering is significant.
 */
enum ClassPathEntryKind {
    kCpeUnknown = 0,
    kCpeJar,
    kCpeDex,
    kCpeLastEntry       /* used as sentinel at end of array */
};

struct ClassPathEntry {
    ClassPathEntryKind kind;
    char*   fileName;
    void*   ptr;            /* JarFile* or DexFile* */
};

/* flags for dvmDumpClass / dvmDumpAllClasses */
#define kDumpClassFullDetail    1
#define kDumpClassClassLoader   (1 << 1)
#define kDumpClassInitialized   (1 << 2)


/*
 * Store a copy of the method prototype descriptor string
 * for the given method into the given DexStringCache, returning the
 * stored string for convenience.
 */
INLINE char* dvmCopyDescriptorStringFromMethod(const Method* method,
        DexStringCache *pCache);

/*
 * Compute the number of argument words (u4 units) required by the
 * given method's prototype. For example, if the method descriptor is
 * "(IJ)D", this would return 3 (one for the int, two for the long;
 * return value isn't relevant).
 */
INLINE int dvmComputeMethodArgsSize(const Method* method);

/*
 * Compare the two method prototypes. The two prototypes are compared
 * as if by strcmp() on the result of dexProtoGetMethodDescriptor().
 */
INLINE int dvmCompareMethodProtos(const Method* method1,
        const Method* method2);

/*
 * Compare the two method prototypes, considering only the parameters
 * (i.e. ignoring the return types). The two prototypes are compared
 * as if by strcmp() on the result of dexProtoGetMethodDescriptor().
 */
INLINE int dvmCompareMethodParameterProtos(const Method* method1,
        const Method* method2);

/*
 * Compare the two method names and prototypes, a la strcmp(). The
 * name is considered the "major" order and the prototype the "minor"
 * order. The prototypes are compared as if by dexProtoGetMethodDescriptor().
 */
int dvmCompareMethodNamesAndProtos(const Method* method1,
        const Method* method2);

/*
 * Compare the two method names and prototypes, a la strcmp(), ignoring
 * the return type. The name is considered the "major" order and the
 * prototype the "minor" order. The prototypes are compared as if by
 * dexProtoGetMethodDescriptor().
 */
int dvmCompareMethodNamesAndParameterProtos(const Method* method1,
        const Method* method2);

/*
 * Compare a method descriptor string with the prototype of a method,
 * as if by converting the descriptor to a DexProto and comparing it
 * with dexProtoCompare().
 */
INLINE int dvmCompareDescriptorAndMethodProto(const char* descriptor,
    const Method* method);

/*
 * Compare a (name, prototype) pair with the (name, prototype) of
 * a method, a la strcmp(). The name is considered the "major" order and
 * the prototype the "minor" order. The descriptor and prototype are
 * compared as if by dvmCompareDescriptorAndMethodProto().
 */
int dvmCompareNameProtoAndMethod(const char* name,
    const DexProto* proto, const Method* method);

/*
 * Compare a (name, method descriptor) pair with the (name, prototype) of
 * a method, a la strcmp(). The name is considered the "major" order and
 * the prototype the "minor" order. The descriptor and prototype are
 * compared as if by dvmCompareDescriptorAndMethodProto().
 */
int dvmCompareNameDescriptorAndMethod(const char* name,
    const char* descriptor, const Method* method);

#endif  // DALVIK_OO_CLASS_H_
