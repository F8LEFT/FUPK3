//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// Import Fupk from libdvm.so
//===----------------------------------------------------------------------===//


#ifndef FUPK3_FUPKCORE_H
#define FUPK3_FUPKCORE_H


#include <AndroidDef.h>

struct FupkInterface {
    void* reserved0;
    void* reserved1;
    void* reserved2;
    void* reserved3;

    bool (*ExportMethod)(void* thread, Method* method);
};

namespace FupkImpl {
    extern FupkInterface* gUpkInterface;
    extern HashTable* gDvmUserDexFiles;

    extern Object* (*fdvmDecodeIndirectRef)(void* self, jobject jobj);
    extern Thread* (*fdvmThreadSelf)();
    extern void (*fupkInvokeMethod)(Method* meth);
    extern ClassObject* (*floadClassFromDex)(DvmDex* pDvmDex,
                                         const DexClassDef* pClassDef, Object* classLoader);

    bool initAll();
}

#endif //FUPK3_FUPKCORE_H
