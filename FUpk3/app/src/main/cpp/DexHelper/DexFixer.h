//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/11.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// Fix DexHeader, and some information
//===----------------------------------------------------------------------===//


#ifndef FUPK3_DEXFIXER_H
#define FUPK3_DEXFIXER_H

#include "AndroidDef.h"


class DexFixer {
public:
    DexFixer(u1* dexData, int length);
    ~DexFixer();

    bool fixAll();
private:
    u4 getDexOff(u1 *mem);
    bool parseDexFile(u1 *memDex);
//    bool fixShaAndChecksum();  //修复checksum和sha值,无任何意义，本来dump出来的数据就不是标准dex文件
    bool fixOptInSns(u2 *insns, size_t len);    // u2* insns, size_t len
    //只有 insns段会被优化,所以不需要考虑tries之类的情况

    bool isValid(size_t v);
private:
    DexFile dexFile;
    DexFile* mDexFile;

    size_t start;
    size_t end;
};

// fix Opcode
void rewriteInvokeObjectInit(u2 *insns, DexFile *dexFile);
void rewriteReturnVoid(u2 *insns, DexFile *dexFile);
void rewriteInstField(u2 *insns, DexFile *dexFile);
void rewriteStaticField(u2 *insns, DexFile *dexFile);
void rewriteQuickMethod(u2 *insns, DexFile *dexFile);
#endif //FUPK3_DEXFIXER_H
