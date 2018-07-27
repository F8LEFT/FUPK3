//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// A simple table to record some dex file information
//===----------------------------------------------------------------------===//


#ifndef FUPK3_DEXHASHTABLE_H
#define FUPK3_DEXHASHTABLE_H



#include "AndroidDef.h"
#include <map>
#include <string>

class DexHashTable {
private:
    std::map<std::string, u4> strIndexMap;      // map for string:idx
    std::map<u4, u4> typeIndexMap;            //type的map: 字符串idx， typeIdx|classIdx
// protoIdx ... 不需要，method中有现成的
    std::map<u4, std::map<u4, std::map<u4, u4>>> fieldIndexMap;          //class_idx, type_idx,name_idx -> idx
    std::map<u4, std::map<u4, std::map<u4, u4>>> methodIndexMap;        //class_idx, protoIdx, name_idx -> idx

public:
    DexHashTable(DexFile *pdexFile);

    ~DexHashTable();

    u4 getStringId(const char *str);

    u4 getTypeId(const char *str);
    u4 getTypeIdByStrId(u4 id);

    u4 getFieldHashId(const char *strClass, const char *strType, const char *strName);
    u4 getFieldId(Field *field);

    u4 getMethodId(const char *strClass, u4 protoIdx, const char *strName);
    u4 getMethodId(Method *method);

    u4 getClassId(const char *str);
    u4 getClassIdByStrId(u4 id);
    u4 getClassId(ClassObject* clazz);

};




#endif //FUPK3_DEXHASHTABLE_H
