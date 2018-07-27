//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "FupkImpl.h"
#include "MinAndroidDef.h"

#include <dlfcn.h>

namespace FupkImpl {
    FupkInterface* gUpkInterface = nullptr;
    HashTable* gDvmUserDexFiles = nullptr;
    Object* (*fdvmDecodeIndirectRef)(void* self, jobject jobj) = nullptr;
    Thread* (*fdvmThreadSelf)() = nullptr;
    void (*fupkInvokeMethod)(Method* meth) = nullptr;
    ClassObject* (*floadClassFromDex)(DvmDex* pDvmDex,
                                      const DexClassDef* pClassDef, Object* classLoader) = nullptr;

    bool initAll() {
        bool done = false;
        auto libdvm = dlopen("libdvm.so", RTLD_NOW);
        if (libdvm == nullptr)
            goto bail;
        gUpkInterface = (FupkInterface*)dlsym(libdvm, "gFupk");
        if (gUpkInterface == nullptr)
            goto bail;
        {
            auto fn = (HashTable* (*)())dlsym(libdvm, "dvmGetUserDexFiles");
            if (fn == nullptr) {
                goto bail;
            }
            gDvmUserDexFiles = fn();
        }
        fdvmDecodeIndirectRef = (Object *(*)(void *, jobject))
                (dlsym(libdvm, "_Z20dvmDecodeIndirectRefP6ThreadP8_jobject"));
        if (fdvmDecodeIndirectRef == nullptr)
            goto bail;
        fdvmThreadSelf = (Thread *(*)())(dlsym(libdvm, "_Z13dvmThreadSelfv"));
        if (fdvmThreadSelf == nullptr)
            goto bail;
        fupkInvokeMethod = (void (*)(Method*))dlsym(libdvm, "fupkInvokeMethod");
        if (fupkInvokeMethod == nullptr)
            goto bail;
        floadClassFromDex = (ClassObject* (*)(DvmDex*, const DexClassDef*, Object*)) dlsym(libdvm, "loadClassFromDex");
        if (floadClassFromDex == nullptr)
            goto bail;
        done = true;

        bail:
        if (!done) {
            FLOGE("Unable to initlize FupkImpl are you sure you are run in the correct machine");
        }
        return done;
    }
}

