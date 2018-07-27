//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "core_Fupk.h"
#include <DexHelper/FupkImpl.h>

#include <DexHelper/Fupk.h>
#include <AndroidDef/AndroidDef.h>
#include <sys/stat.h>
#include <DexHelper/DexDumper.h>

//================= register function==================
bool core_Fupk::registerNativeMethod(JNIEnv *env) {
    bool useSystem = true, useHookEntry = false;
    if (useSystem) {
        // for system entry
        auto clazz = env->FindClass("android/app/fupk3/Fupk");
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            JNINativeMethod natives[] = {
                    {"unpackAll", "(Ljava/lang/String;)V", (void*)core_Fupk::unpackAll}
            };
            if (env->RegisterNatives(clazz, natives,
                                     sizeof(natives)/sizeof(JNINativeMethod)) != JNI_OK) {
                env->ExceptionClear();
            }
        }

    }
    if (useHookEntry) {
        // for hook entry
        auto clazz = env->FindClass("f8left/fupk3/core/Fupk");
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        } else {
            JNINativeMethod natives[] = {
                    {"unpackAll", "(Ljava/lang/String;)V", (void*)core_Fupk::unpackAll}
            };
            if (env->RegisterNatives(clazz, natives,
                                     sizeof(natives)/sizeof(JNINativeMethod)) != JNI_OK) {
                env->ExceptionClear();
            }
        }

    }
    return true;
}

void ::core_Fupk::unpackAll(JNIEnv *env, jobject obj, jstring folder) {
    auto interface = FupkImpl::gUpkInterface;
    if (interface == nullptr) {
        FLOGE("Unable to found fupk interface");
        return;
    }
    // Hook all
    interface->ExportMethod = fupk_ExportMethod;

    FLOGD("Now start to dump all dex file");
    auto pFolder = env->GetStringUTFChars(folder, nullptr);
    std::string sFolder = pFolder;
    sFolder = sFolder + "/.fupk3";
    env->ReleaseStringUTFChars(folder, pFolder);

    mkdir(sFolder.c_str(), 0700);
    Fupk upk(env, sFolder, obj);
    upk.unpackAll();

}