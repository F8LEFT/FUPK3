//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// Help to dump cookie data from system
//===----------------------------------------------------------------------===//


#ifndef FUPK3_COOKIE_H
#define FUPK3_COOKIE_H

#include <AndroidDef.h>
#include <string>

// search for cookie data DvmDex->DexFile
// I will use DvmDex instead of DexFile(DvmDex will record all loaded Method info)
class Cookie {
public:
    Cookie();
    // print cookie info
    void print();
    // cookie's size musb be 2^n, it store with hash
    int size();
    // may return null if the solt is empty
    DvmDex *getCookieAt(int idx, const char *&dexName, const char** ignore = nullptr);
    // getMagic Number
    static std::string getDvmMagic(DvmDex *dvmDex);
private:
    HashTable* userDexFiles;
};


#endif //FUPK3_COOKIE_H
