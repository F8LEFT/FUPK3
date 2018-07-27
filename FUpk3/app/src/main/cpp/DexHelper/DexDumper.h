//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// used to dump dex file
//===----------------------------------------------------------------------===//


#ifndef FUPK3_DEXDUMPER_H
#define FUPK3_DEXDUMPER_H


#include "AndroidDef.h"
#include "DexHashTable.h"
#include "ClassDefBuilder.h"

#include <jni.h>
#include <string>

#define DUMP_CLAZZ_NAME ""

// This data is used to rebuild dexfile.
// and shared for all
struct DexSharedData {
    u4 num_class_defs;
    u4 total_point; // total rewrite data (to the end)
    u4 start;       // rewrite start
    char padding;

    DexHeader* mDexHeader;
    DexHashTable* mHash;
    DexMethod* mCurMethod;      // current fix method

    std::string classFile;
    std::string extra;
};

class DexDumper {
public:
    DexDumper(JNIEnv *env, DvmDex *dvmDex, jobject upkObj);
    bool rebuild();

    std::string mRebuilded;
private:
    bool fixDexHeader();
    bool fixMethodByMemory(DexSharedData& shared, DexMethod* dexMethod, DexFile* dexFile);
    bool fixMethodByDvm(DexSharedData &shared, DexMethod *dexMethod,
                        ClassDefBuilder* builder, u4 &lastIndex);

    JNIEnv* mEnv;
    DvmDex* mDvmDex;
    jobject mUpkObj;

    // rebuilded data
    DexOptHeader mDexOptHeader;
    DexHeader mDexHeader;


};

bool fupk_ExportMethod(void* thread, Method* method);

#endif //FUPK3_DEXDUMPER_H
