//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// rebuilding class define file
//===----------------------------------------------------------------------===//


#ifndef FUPK3_CLASSDEFBUILDER_H
#define FUPK3_CLASSDEFBUILDER_H

#include "DexHashTable.h"
#include "AndroidDef.h"

#include <map>

// 根据ClassObject和classDef的数据，修复classDef的数据
class ClassDefBuilder {
private:
    DexClassDef classDef;
    DexClassData *pDataFix;            //变长，
    DexFile *pDexFile;
    DexHashTable *sHash;

    ClassObject *clazz;

public:
    ClassDefBuilder(ClassObject *clazz, DexClassDef *_classDef, DexFile *pDexFile,
                    DexHashTable *sHash);
    ~ClassDefBuilder();
    DexClassDef* getClassDef() { return &classDef; }
    DexClassData* getClassData() { return pDataFix; }

    bool addMethodMap(int idx, Method* m);
    Method* getMethodMap(int idx);
private:
    bool rebuildClassDefWithClassObject();

    std::map<u4, Method*> methodMaps;
};


void ReadClassDataHeader(const uint8_t** pData,
                         DexClassDataHeader *pHeader);
void ReadClassDataField(const uint8_t** pData, DexField* pField);
void ReadClassDataMethod(const uint8_t** pData, DexMethod* pMethod);
// must use delete[] (char*)result to release memory
DexClassData* ReadClassData(const uint8_t** pData);

#endif //FUPK3_CLASSDEFBUILDER_H
