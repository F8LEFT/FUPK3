//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// the entry point for FUpk3
//===----------------------------------------------------------------------===//

#include "MinAndroidDef.h"
#include "DexHelper/FupkImpl.h"
#include "DexHelper/Fupk.h"
#include "core_Fupk.h"

#include <jni.h>


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{

    FLOGD("try to load FUpk3");
    JNIEnv *env = nullptr;
    jint result = -1;


    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        FLOGE("This jni version is not supported");
        return JNI_VERSION_1_6;
    }

    if (!core_Fupk::registerNativeMethod(env)) {
        FLOGE("Unable to register native method: maybe the java has not loaded");
        // I don't want the process crash, just continue
        env->ExceptionClear();
    }

    if (!FupkImpl::initAll()) {
        FLOGE("Fupk interface not found are you running in the correct phone?");
    }

    FLOGD("FUpk3 load success");
    FLOGD("current JNI Version %d", JNI_VERSION_1_6);

    return JNI_VERSION_1_6;
}
