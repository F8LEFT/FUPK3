//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//


#include "DexDumper.h"

#include "utils/myfile.h"
#include "DexHashTable.h"
#include "FupkImpl.h"
#include "ClassDefBuilder.h"

#include "JniInfo.h"

using namespace FupkImpl;

#define OPTMAGIC "dey\n036\0"
#define DEXMAGIC "dex\n035\0"

#define mask 0x3ffff

uint8_t* codeitem_end(const u1** pData);
uint8_t* EncodeClassData(DexClassData *pData, int& len);

// =============== DexDumper ====================

DexDumper::DexDumper(JNIEnv *env, DvmDex *dvmDex, jobject upkObj) {
    mEnv = env;
    mDvmDex = dvmDex;
    mUpkObj = upkObj;
}

bool DexDumper::rebuild() {
    // scan for basic data ---- DexFile Header
    AutoJniEnvRelease envGuard(mEnv);

    jclass upkClazz = mEnv->GetObjectClass(mUpkObj);
    auto loaderObject = JniInfo::GetObjectField(mEnv, mUpkObj, "appLoader", "Ljava/lang/ClassLoader;");
    if (loaderObject == nullptr) {
        // not valid ... just kill it
        return false;
    }
    auto self = FupkImpl::fdvmThreadSelf();
    auto tryLoadClass_method = mEnv->GetMethodID(upkClazz, "tryLoadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    Object* gLoader = FupkImpl::fdvmDecodeIndirectRef(self, loaderObject);
    mEnv->DeleteLocalRef(loaderObject);


    DexFile *pDexFile = mDvmDex->pDexFile;
    if (pDexFile->pOptHeader) {
        mymemcpy(&mDexOptHeader, pDexFile->pOptHeader, sizeof(DexOptHeader));
        mymemcpy(mDexOptHeader.magic, OPTMAGIC, sizeof(mDexOptHeader.magic));
    }
    mymemcpy(&mDexHeader, pDexFile->pHeader, sizeof(DexHeader));
    mymemcpy(mDexHeader.magic, DEXMAGIC, sizeof(mDexHeader.magic));

    fixDexHeader();


    // copytofile
    DexSharedData shared;

    shared.num_class_defs = mDexHeader.classDefsSize;
    shared.total_point = mDexHeader.dataOff + mDexHeader.dataSize;
    shared.start = shared.total_point;
    shared.padding = 0;
    // All classDef and methodDefine is rebuilded, Non of the value in dex is used
    DexHashTable sHash = DexHashTable(pDexFile);
    shared.mHash = &sHash;

    // the interface reserved field is used to transport data
    gUpkInterface->reserved0 = &shared;

    while(shared.total_point & 3) {
        shared.total_point += 1;
        shared.extra.push_back(shared.padding);
    }

    FLOGI("num class def: %u", shared.num_class_defs);
    for(int i = 0; i < shared.num_class_defs; i++) {
        FLOGI("cur class: %u Total: %u", i, shared.num_class_defs);

        // try use interpret first
        auto origClassDef = dexGetClassDef(mDvmDex->pDexFile, i);

        auto descriptor = dexGetClassDescriptor(mDvmDex->pDexFile, origClassDef);
        // descriptor must look like Ljava/lang/String;, so just change into java.lang.String
        // get dot name

        std::string dotDescriptor = descriptor;
        dotDescriptor = dotDescriptor.substr(1, dotDescriptor.length() - 2);
        for(char *c = (char*)dotDescriptor.c_str(); *c != '\0'; c++) {
            if (*c == '/') {
                *c = '.';
            }
        }
        ClassObject* Clazz = nullptr;
        // try load class by original loader
        if (Clazz == nullptr) {
            jstring jDotDescriptor = mEnv->NewStringUTF(dotDescriptor.c_str());
            auto jClazz = mEnv->CallObjectMethod(mUpkObj, tryLoadClass_method, jDotDescriptor);
            if (jClazz != nullptr) {
                Clazz = (ClassObject*)FupkImpl::fdvmDecodeIndirectRef(self, jClazz);
                mEnv->DeleteLocalRef(jClazz);
            } else {
                FLOGE("Class loading and init false changed into normal load");
            }
            mEnv->DeleteLocalRef(jDotDescriptor);
        }

        // just loadClassFromDex. I do not really care if the class has been inited
        // No link is needed, please invoke all valid method in it
        // if failed, just scorped into fupk loader
        if (Clazz == nullptr) {
            Clazz = FupkImpl::floadClassFromDex(mDvmDex,
                                                dexGetClassDef(mDvmDex->pDexFile, i), gLoader);
        }

        // class loaded, then use ClassObject to rebuild classDef
        auto defBuilder = ClassDefBuilder(Clazz, (DexClassDef*)origClassDef, pDexFile, &sHash);
        auto newDef = defBuilder.getClassDef();


        if (newDef->classDataOff == 0) {
            FLOGI("des: %s is passed", descriptor);
            goto writeClassDef;
        } else {
            FLOGI("des %s", descriptor);
            auto newData = defBuilder.getClassData();
            if (Clazz != nullptr) {
                FLOGI("fix with dvm ClassObject");
                u4 lastIndex = 0;
                for(int i = 0; i < newData->header.directMethodsSize; i++) {
                    fixMethodByDvm(shared, &newData->directMethods[i],
                                   &defBuilder, lastIndex);
                }
                lastIndex = 0;
                for(int i = 0; i < newData->header.virtualMethodsSize; i++) {
                    fixMethodByDvm(shared, &newData->virtualMethods[i],
                                   &defBuilder, lastIndex);
                }
            } else {
                FLOGI("fix with memory classDef");

                if (newData->directMethods) {
                    for(auto j = 0; j < newData->header.directMethodsSize; j++) {
                        fixMethodByMemory(shared, &newData->directMethods[j], pDexFile);
                    }
                }
                if (newData->virtualMethods) {
                    for(auto j = 0; j < newData->header.virtualMethodsSize; j++) {
                        fixMethodByMemory(shared, &newData->virtualMethods[j], pDexFile);
                    }
                }
            }

            int class_data_len = 0;
            u1* out = EncodeClassData(newData, class_data_len);
            newDef->classDataOff = shared.total_point;
            shared.extra.append((char*)out, class_data_len);
            shared.total_point += class_data_len;
            while (shared.total_point & 3) {
                shared.extra.push_back(shared.padding);
                shared.total_point++;
            }
            delete[] out;
        }


        writeClassDef:
        shared.classFile.append((char*)newDef, sizeof(DexClassDef));
    }

    // the local value is not used anymore, just clear it
    gUpkInterface->reserved0 = nullptr;

    // finally, rebuilt the whold dex file
    if (pDexFile->pOptHeader != nullptr) {
        // dump optheader, no need????
        u1* optDex = (u1*) (mDexOptHeader.depsOffset + (u4)pDexFile->pOptHeader);
        shared.extra.append((char*)optDex, mDexOptHeader.optOffset - mDexOptHeader.depsOffset + mDexOptHeader.optLength);
        mDexOptHeader.optOffset = shared.total_point + mDexOptHeader.optOffset - mDexOptHeader.depsOffset + 40;
        mDexOptHeader.depsOffset = shared.total_point + 40;
    }

    // header(s)
    if (pDexFile->pOptHeader) {
        mRebuilded.append((char*)&mDexOptHeader, sizeof(DexOptHeader));
    }
    mRebuilded.append((char*)&mDexHeader, sizeof(DexHeader));
    // skipping ClassDef
    mRebuilded.append((char*)pDexFile->pStringIds, (u1*)pDexFile->pClassDefs - (u1*)pDexFile->pStringIds);
    // write rebuilded classdef
    mRebuilded.append(shared.classFile);
    // write rest data
    u1* addr = (u1*)pDexFile->baseAddr + mDexHeader.classDefsOff
               + mDexHeader.classDefsSize * sizeof(DexClassDef);
    u4 len = shared.start - (addr - pDexFile->baseAddr);
    mRebuilded.append((char*)addr, len);

    // write extra data
    mRebuilded.append(shared.extra);

    return true;
}

bool DexDumper::fixDexHeader() {
    DexFile *pDexFile = mDvmDex->pDexFile;

    mDexHeader.stringIdsOff = (u4) ((u1 *) pDexFile->pStringIds - (u1 *) pDexFile->pHeader);
    mDexHeader.typeIdsOff = (u4) ((u1 *) pDexFile->pTypeIds - (u1 *) pDexFile->pHeader);
    mDexHeader.fieldIdsOff = (u4) ((u1 *) pDexFile->pFieldIds - (u1 *) pDexFile->pHeader);
    mDexHeader.methodIdsOff = (u4) ((u1 *) pDexFile->pMethodIds - (u1 *) pDexFile->pHeader);
    mDexHeader.protoIdsOff = (u4) ((u1 *) pDexFile->pProtoIds - (u1 *) pDexFile->pHeader);
    mDexHeader.classDefsOff = (u4) ((u1 *) pDexFile->pClassDefs - (u1 *) pDexFile->pHeader);
    return true;
}

bool DexDumper::fixMethodByMemory(DexSharedData &shared, DexMethod *dexMethod,
                                  DexFile *dexFile) {
    if(dexMethod->codeOff == 0 ||
       dexMethod->accessFlags & ACC_NATIVE) {
        dexMethod->codeOff = 0;
        return false;
    }

    auto code = dexGetCode(dexFile, dexMethod);

    u1 *item = (u1 *) code;
    int code_item_len = 0;
    if (code->triesSize) {
        const u1 *handler_data = dexGetCatchHandlerData(code);
        const u1 **phandler = (const u1 **) &handler_data;
        u1 *tail = codeitem_end(phandler);
        code_item_len = (int) (tail - item);
    } else {
        code_item_len = 16 + code->insnsSize * 2;
    }

    // dump and reset dexMethod info
    dexMethod->codeOff = shared.total_point;
    shared.extra.append((char*)item, code_item_len);
    shared.total_point += code_item_len;
    while (shared.total_point & 3) {
        shared.extra.push_back(shared.padding);
        shared.total_point++;
    }
    return true;
}

bool DexDumper::fixMethodByDvm(DexSharedData &shared, DexMethod *dexMethod,
                               ClassDefBuilder* builder, u4 &lastIndex) {
    lastIndex = lastIndex + dexMethod->methodIdx;
    auto m = builder->getMethodMap(lastIndex);

    assert(m != nullptr && "Unable to fix MethodBy Dvm, this should happened");

    shared.mCurMethod = dexMethod;
    FupkImpl::fupkInvokeMethod(m);
    shared.mCurMethod = nullptr;
    return true;
}

bool fupk_ExportMethod(void *thread, Method *method) {
    DexSharedData* shared = (DexSharedData*)gUpkInterface->reserved0;
    DexMethod* dexMethod = shared->mCurMethod;
    u4 ac = (method->accessFlags) & mask;
    if (method->insns == nullptr || ac & ACC_NATIVE) {
        if (ac & ACC_ABSTRACT) {
            ac = ac & ~ACC_NATIVE;
        }
        dexMethod->accessFlags = ac;
        dexMethod->codeOff = 0;
        return false;
    }

    if (ac != dexMethod->accessFlags) {
        dexMethod->accessFlags = ac;
    }
    dexMethod->codeOff = shared->total_point;
    DexCode *code = (DexCode*)((const u1*) method->insns - 16);

    u1 *item = (u1*) code;
    int code_item_len = 0;
    if (code->triesSize) {
        const u1*handler_data = dexGetCatchHandlerData(code);
        const u1 **phandler = (const u1**) &handler_data;
        u1 *tail = codeitem_end(phandler);
        code_item_len = (int)(tail - item);
    } else {
        code_item_len = 16 + code->insnsSize * 2;
    }
    shared->extra.append((char*)item, code_item_len);
    shared->total_point += code_item_len;
    while(shared->total_point & 3) {
        shared->extra.push_back(shared->padding);
        shared->total_point++;
    }
    return true;
}


// =============== helper function ==============
uint8_t* codeitem_end(const u1** pData)
{
    uint32_t num_of_list = readUnsignedLeb128(pData);
    for (;num_of_list>0;num_of_list--) {
        int32_t num_of_handlers=readSignedLeb128(pData);
        int num=num_of_handlers;
        if (num_of_handlers<=0) {
            num=-num_of_handlers;
        }
        for (; num > 0; num--) {
            readUnsignedLeb128(pData);
            readUnsignedLeb128(pData);
        }
        if (num_of_handlers<=0) {
            readUnsignedLeb128(pData);
        }
    }
    return (uint8_t*)(*pData);
}


uint8_t* EncodeClassData(DexClassData *pData, int& len)
{
    len=0;

    len+=unsignedLeb128Size(pData->header.staticFieldsSize);
    len+=unsignedLeb128Size(pData->header.instanceFieldsSize);
    len+=unsignedLeb128Size(pData->header.directMethodsSize);
    len+=unsignedLeb128Size(pData->header.virtualMethodsSize);

    if (pData->staticFields) {
        for (uint32_t i = 0; i < pData->header.staticFieldsSize; i++) {
            len+=unsignedLeb128Size(pData->staticFields[i].fieldIdx);
            len+=unsignedLeb128Size(pData->staticFields[i].accessFlags);
        }
    }

    if (pData->instanceFields) {
        for (uint32_t i = 0; i < pData->header.instanceFieldsSize; i++) {
            len+=unsignedLeb128Size(pData->instanceFields[i].fieldIdx);
            len+=unsignedLeb128Size(pData->instanceFields[i].accessFlags);
        }
    }

    if (pData->directMethods) {
        for (uint32_t i=0; i<pData->header.directMethodsSize; i++) {
            len+=unsignedLeb128Size(pData->directMethods[i].methodIdx);
            len+=unsignedLeb128Size(pData->directMethods[i].accessFlags);
            len+=unsignedLeb128Size(pData->directMethods[i].codeOff);
        }
    }

    if (pData->virtualMethods) {
        for (uint32_t i=0; i<pData->header.virtualMethodsSize; i++) {
            len+=unsignedLeb128Size(pData->virtualMethods[i].methodIdx);
            len+=unsignedLeb128Size(pData->virtualMethods[i].accessFlags);
            len+=unsignedLeb128Size(pData->virtualMethods[i].codeOff);
        }
    }

    // TODO delete []stroe
    uint8_t * store = (uint8_t *) new u1[len];

    uint8_t * result=store;

    store = writeUnsignedLeb128(store,pData->header.staticFieldsSize);
    store = writeUnsignedLeb128(store,pData->header.instanceFieldsSize);
    store = writeUnsignedLeb128(store,pData->header.directMethodsSize);
    store = writeUnsignedLeb128(store,pData->header.virtualMethodsSize);

    if (pData->staticFields) {
        for (uint32_t i = 0; i < pData->header.staticFieldsSize; i++) {
            store = writeUnsignedLeb128(store,pData->staticFields[i].fieldIdx);
            store = writeUnsignedLeb128(store,pData->staticFields[i].accessFlags);
        }
    }

    if (pData->instanceFields) {
        for (uint32_t i = 0; i < pData->header.instanceFieldsSize; i++) {
            store = writeUnsignedLeb128(store,pData->instanceFields[i].fieldIdx);
            store = writeUnsignedLeb128(store,pData->instanceFields[i].accessFlags);
        }
    }

    if (pData->directMethods) {
        for (uint32_t i=0; i<pData->header.directMethodsSize; i++) {
            store = writeUnsignedLeb128(store,pData->directMethods[i].methodIdx);
            store = writeUnsignedLeb128(store,pData->directMethods[i].accessFlags);
            store = writeUnsignedLeb128(store,pData->directMethods[i].codeOff);
        }
    }

    if (pData->virtualMethods) {
        for (uint32_t i=0; i<pData->header.virtualMethodsSize; i++) {
            store = writeUnsignedLeb128(store,pData->virtualMethods[i].methodIdx);
            store = writeUnsignedLeb128(store,pData->virtualMethods[i].accessFlags);
            store = writeUnsignedLeb128(store,pData->virtualMethods[i].codeOff);
        }
    }

    return result;
}