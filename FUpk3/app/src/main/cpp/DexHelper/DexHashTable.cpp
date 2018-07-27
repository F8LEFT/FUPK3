//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//


#include "DexHashTable.h"

DexHashTable::DexHashTable(DexFile *pdexFile) {
    auto *pDexHeader = pdexFile->pHeader;
    u4 strIdSize = pDexHeader->stringIdsSize;

    for (u4 i = 0; i < strIdSize; ++i) {
        strIndexMap[dexStringById(pdexFile, i)] = i;
    }

    u4 typeIdSize = pDexHeader->typeIdsSize;
    for (u4 i = 0; i < typeIdSize; ++i) {
        typeIndexMap[dexGetTypeId(pdexFile, i)->descriptorIdx] = i;
    }

    u4 fieldIdSize = pDexHeader->fieldIdsSize;
    for (u4 i = 0; i < fieldIdSize; ++i) {
        auto fId = dexGetFieldId(pdexFile, i);
        fieldIndexMap[fId->classIdx][fId->typeIdx][fId->nameIdx] = i;
    }

    u4 methodIdSize = pDexHeader->methodIdsSize;
    for (u4 i = 0; i < methodIdSize; ++i) {
        auto mId = dexGetMethodId(pdexFile, i);
        methodIndexMap[mId->classIdx][mId->protoIdx][mId->nameIdx] = i;
    }


}

DexHashTable::~DexHashTable() {
}

u4 DexHashTable::getStringId(const char *str) {
    auto iter = strIndexMap.find(str);
    if (iter != strIndexMap.end()) {
        return iter->second;
    }
    LOGE("unable to find string id %s", str);
    return -1;
}

u4 DexHashTable::getTypeId(const char *str) {
    return getTypeIdByStrId(getStringId(str));
}

u4 DexHashTable::getTypeIdByStrId(u4 id) {
    auto iter = typeIndexMap.find(id);
    if (iter != typeIndexMap.end()) {
        return iter->second;
    }
    LOGE("unable to find type id %d", id);
    return -1;
}

u4 DexHashTable::getClassId(const char *str) {
    return getTypeId(str);
}

u4 DexHashTable::getClassIdByStrId(u4 id) {
    return getTypeIdByStrId(id);
}

u4 DexHashTable::getFieldHashId(const char *strClass, const char *strType, const char *strName) {
    u4 classIdx = getClassId(strClass);
    u4 typeId = getTypeId(strType);
    u4 strId = getStringId(strName);
    auto itClassid = fieldIndexMap.find(classIdx);
    if (itClassid != fieldIndexMap.end()) {
        auto& typeMap = itClassid->second;
        auto itTypeid = typeMap.find(typeId);
        if (itTypeid != typeMap.end()) {
            auto& nameMap = itTypeid->second;
            auto itNameid = nameMap.find(strId);
            if(itNameid != nameMap.end()) {
                return itNameid->second;
            }
        }
    }
    LOGE("unable to find field id %s, %s, %s", strClass, strType, strName);
    LOGE("traceback classidx:%d, typeId:%d, strId:%d", classIdx, typeId, strId);
    return -1;
}

u4 DexHashTable::getMethodId(const char *strClass, u4 protoIdx, const char *strName) {
    u4 classIdx = getClassId(strClass);
    u4 strId = getStringId(strName);
    auto itClassid = methodIndexMap.find(classIdx);
    if (itClassid != methodIndexMap.end()) {
        auto& typeMap = itClassid->second;
        auto itProtoId = typeMap.find(protoIdx);
        if (itProtoId != typeMap.end()) {
            auto& nameMap = itProtoId->second;
            auto itNameid = nameMap.find(strId);
            if(itNameid != nameMap.end()) {
                return itNameid->second;
            }
        }
    }

    LOGE("unable to find method id %s, %d, %s", strClass, protoIdx, strName);
    LOGE("trackback classidx:%d, protoIdx:%d, strid:%d", classIdx, protoIdx, strId);
    return -1;
}

u4 DexHashTable::getFieldId(Field *field) {
    return getFieldHashId(field->clazz->descriptor, field->signature, field->name);
}

u4 DexHashTable::getMethodId(Method *method) {
    return getMethodId(method->clazz->descriptor, method->prototype.protoIdx, method->name);
}

u4 DexHashTable::getClassId(ClassObject *clazz) {
    return getClassId(clazz->descriptor);
}