//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// see java class f8left.fupk3.core.Fupkj
//===----------------------------------------------------------------------===//


#ifndef FUPK3_CORE_FUPK_H
#define FUPK3_CORE_FUPK_H

#include <jni.h>

namespace core_Fupk {
    bool registerNativeMethod(JNIEnv *env);
    // native function for class f8left.fupk3.core.Fupk
    void unpackAll(JNIEnv* env, jobject obj, jstring folder);
}

#endif //FUPK3_CORE_FUPK_H
