//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "Cookie.h"

#include "FupkImpl.h"
#include "utils/RWGuard.h"

Cookie::Cookie() {
    // valid has been checked in Fupk, at this time, I will not to
    // check anymore.
    userDexFiles = FupkImpl::gDvmUserDexFiles;
}

int Cookie::size() {
    return userDexFiles->tableSize;
}

DvmDex *Cookie::getCookieAt(int idx, const char *&dexName, const char** ignore) {
    if (idx >= size())
        return nullptr;
    HashEntry *hashEntry = userDexFiles->pEntries + idx;
    // valid check
    if (hashEntry->data == nullptr)
        return nullptr;
    if (!RWGuard::getInstance()->isReadable(reinterpret_cast<unsigned int>(hashEntry->data))) {
        FLOGE("I Found an no empty hashEntry but it is not readable %d %08x", idx, hashEntry->data);
        return nullptr;
    }
    DvmDex *dvmDex = nullptr;
    DexOrJar *dexOrJar = (DexOrJar*) hashEntry->data;
    if (dexOrJar->isDex) {
        RawDexFile *rawDexFile = dexOrJar->pRawDexFile;
        dvmDex = rawDexFile->pDvmDex;
    } else {
        JarFile *jarFile = dexOrJar->pJarFile;
        dvmDex = jarFile->pDvmDex;
    }

    // check the ignore case
    if (ignore != nullptr) {
        bool isSkip = false;
        while(*ignore != nullptr) {
            if (strstr(dexOrJar->fileName, *ignore) != nullptr) {
                isSkip = true;
                break;
            }
            ignore++;
        }
        if (isSkip)
            return nullptr;
    }

    // right, just return
    dexName = dexOrJar->fileName;
    return dvmDex;
}

void Cookie::print() {
    for(int idx = 0; idx < size(); idx ++) {
        const char* name;
        auto dvmDex = getCookieAt(idx, name);
        if (dvmDex == nullptr)
            continue;
        FLOGD("Cookie: %d name: %s DvmPtr: %X", idx, name, (int)dvmDex);
    }
}

std::string Cookie::getDvmMagic(DvmDex *dvmDex) {
    if (dvmDex == nullptr)
        return "";
    DexFile *pDexFile = dvmDex->pDexFile;
    auto header = pDexFile->pHeader;

    std::string sig;
    sig.resize(kSHA1DigestLen*2 + 1);

    auto p = (char*)sig.c_str();
    for(auto i = 0; i < kSHA1DigestLen; i++) {
        sprintf(p, "%02x", header->signature[i]);
        p += 2;
    }
    return sig;
}
