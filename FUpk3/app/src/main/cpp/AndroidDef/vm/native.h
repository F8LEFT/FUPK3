//
// Created by F8LEFT on 2017/2/21.
//

#ifndef VMSHELL_NATIVE_H
#define VMSHELL_NATIVE_H


#include "Common.h"
#include "DvmDex.h"
#include "../libdex/ZipArchive.h"

struct RawDexFile {
    char*       cacheFileName;
    DvmDex*     pDvmDex;
};

struct JarFile {
    ZipArchive  archive;
    //MemMapping  map;
    char*       cacheFileName;
    DvmDex*     pDvmDex;
};


/*
 * Internal struct for managing DexFile.
 */
struct DexOrJar {
    char*       fileName;
    bool        isDex;
    bool        okayToFree;
    RawDexFile* pRawDexFile;
    JarFile*    pJarFile;
    u1*         pDexMemory; // malloc()ed memory, if any
};



#endif //VMSHELL_NATIVE_H
