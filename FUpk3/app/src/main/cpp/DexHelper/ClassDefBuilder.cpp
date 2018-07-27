//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//


#include "ClassDefBuilder.h"
#include <list>
#include <memory>
#include <algorithm>
#include <vector>


#define ACFlag 0x3ffff


// 这里必须跳过脏数据，就是那种本来就是dataOff为0的数据，这样clazz的数据本身就是有问题的。
// 可以检查clazz 的 status来决定是否进行修复
// 如果是CLASS_NOTREADY是不是该修为全0？又或者是不修复？
ClassDefBuilder::ClassDefBuilder(ClassObject *_clazz, DexClassDef *_classDef, DexFile *_pDexFile,
                                 DexHashTable *_sHash) {
    clazz = _clazz;
    pDexFile = _pDexFile;
    sHash = _sHash;

    memcpy(&classDef, _classDef, sizeof(DexClassDef));
    pDataFix = NULL;

    if (clazz == nullptr) {
        auto data = dexGetClassData(_pDexFile, _classDef);
        pDataFix = ReadClassData(&data);
        return ;
    }
    // build with class
    DexClassDataHeader header;
    header.staticFieldsSize = clazz->sfieldCount;
    header.instanceFieldsSize = clazz->ifieldCount;
    header.directMethodsSize = clazz->directMethodCount;
    header.virtualMethodsSize = clazz->virtualMethodCount;


    if (header.staticFieldsSize + header.instanceFieldsSize +
            header.directMethodsSize + header.virtualMethodsSize == 0) {
        // This class does not have any field/method data, just skip it
        classDef.classDataOff = 0;
        return ;
    }
    // 这个resultSize应该为最大长度
    size_t resultSize = sizeof(DexClassData) + (header.staticFieldsSize * sizeof(DexField)) +
                        (header.instanceFieldsSize * sizeof(DexField)) +
                        (header.directMethodsSize * sizeof(DexMethod)) +
                        (header.virtualMethodsSize * sizeof(DexMethod));

    pDataFix = (DexClassData *) new char[resultSize];

    uint8_t *ptr = ((uint8_t *) pDataFix) + sizeof(DexClassData);

    pDataFix->header = header;
    // 分配空间,与移动指针
    if (header.staticFieldsSize != 0) {
        pDataFix->staticFields = (DexField *) ptr;
        ptr += header.staticFieldsSize * sizeof(DexField);
    } else {
        pDataFix->staticFields = NULL;
    }

    if (header.instanceFieldsSize != 0) {
        pDataFix->instanceFields = (DexField *) ptr;
        ptr += header.instanceFieldsSize * sizeof(DexField);
    } else {
        pDataFix->instanceFields = NULL;
    }

    if (header.directMethodsSize != 0) {
        pDataFix->directMethods = (DexMethod *) ptr;
        ptr += header.directMethodsSize * sizeof(DexMethod);
    } else {
        pDataFix->directMethods = NULL;
    }

    if (header.virtualMethodsSize != 0) {
        pDataFix->virtualMethods = (DexMethod *) ptr;
    } else {
        pDataFix->virtualMethods = NULL;
    }

    rebuildClassDefWithClassObject();
}

ClassDefBuilder::~ClassDefBuilder() {
    if (pDataFix != NULL)
        delete[](u1 *) pDataFix;
}

bool ClassDefBuilder::rebuildClassDefWithClassObject() {
    // rebuild from ClassObject* clazz
    DexClassDataHeader *header = &pDataFix->header;
    // -------------build
    auto dumpField = [this](DexField *dexField, Field* field) {
        u4 fieldId = sHash->getFieldId(field);
        dexField->fieldIdx = fieldId;
        dexField->accessFlags = field->accessFlags & ACFlag;
    };
    auto dumpMethod = [this](DexMethod* dexMethod, Method* method) {
        if (method->accessFlags & ACC_MIRANDA) {    // fack method from interface
            dexMethod->methodIdx = -1;  // make it fack
            return;
        }
        dexMethod->methodIdx = sHash->getMethodId(method);
        if (dexMethod->methodIdx == -1) {
            return;
        }

        dexMethod->accessFlags = method->accessFlags & ACFlag;
        if (method->accessFlags & ACC_NATIVE) {
            dexMethod->codeOff = 0;
        }
        if (method->accessFlags & ACC_ABSTRACT) {
            dexMethod->accessFlags = dexMethod->accessFlags & ~ACC_NATIVE;
            dexMethod->codeOff = 0;
        }

        addMethodMap(dexMethod->methodIdx, method);
    };
    for(auto i = 0; i < header->staticFieldsSize; i++) {
        dumpField(&pDataFix->staticFields[i], &clazz->sfields[i]);
    }
    for(auto i = 0; i < header->instanceFieldsSize; i++) {
        dumpField(&pDataFix->instanceFields[i], &clazz->ifields[i]);
    }
    for(auto i = 0; i < header->directMethodsSize; i++) {
        dumpMethod(&pDataFix->directMethods[i], &clazz->directMethods[i]);
    }
    for(auto i = 0; i < header->virtualMethodsSize; i++) {
        dumpMethod(&pDataFix->virtualMethods[i], &clazz->virtualMethods[i]);
    }

    // ---------------sort
    auto fixField = [](DexField* dexField, u4& size) {
        std::vector<DexField> list;
        for(auto i = 0; i < size; i++) {
            auto &f = dexField[i];
            if (f.fieldIdx == -1) {
                continue;
            }
            list.push_back(f);
        }
        size = list.size();
        if (size == 0) {
            return false;
        }
        std::sort(list.begin(), list.end(), [](const DexField& a, const DexField& b) {
            return a.fieldIdx < b.fieldIdx;
        });
        // rewrite field data
        int index = 0;
        auto p = dexField;
        for(auto it = list.begin(), itEnd = list.end(); it != itEnd; it++, p++) {
            *p = *it;
            p->fieldIdx = it->fieldIdx - index;
            index = it->fieldIdx;
            assert(p->fieldIdx >= 0);
        }
        return true;
    };

    auto fixMethod = [](DexMethod* dexMethod, u4& size) {
        std::vector<DexMethod> list;
        for(auto i = 0; i < size; i++) {
            auto &f = dexMethod[i];
            if (f.methodIdx == -1) {
                continue;
            }
            list.push_back(f);
        }
        size = list.size();
        if (size == 0) {
            return false;
        }
        std::sort(list.begin(), list.end(), [](DexMethod& a, DexMethod& b) {
            return a.methodIdx < b.methodIdx;
        });
        // rewrite method
        int index = 0;
        auto p = dexMethod;
        for(auto it = list.begin(), itEnd = list.end(); it != itEnd; it++, p++) {
            *p = *it;
            p->methodIdx = it->methodIdx - index;
            index = it->methodIdx;
            assert(p->methodIdx >= 0);
        }
        return true;
    };
    fixField(pDataFix->staticFields, header->staticFieldsSize);
    fixField(pDataFix->instanceFields, header->instanceFieldsSize);
    fixMethod(pDataFix->directMethods, header->directMethodsSize);
    fixMethod(pDataFix->virtualMethods, header->virtualMethodsSize);

    // It is not a good idea to use classDataOff here,
    classDef.classDataOff = (u1 *) pDataFix - pDexFile->baseAddr;
    return true;
}

bool ClassDefBuilder::addMethodMap(int idx, Method *m) {
    assert(getMethodMap(idx) == nullptr && "Current method existed");
    methodMaps[idx] = m;
    return true;
}

Method *ClassDefBuilder::getMethodMap(int idx) {
    auto m = methodMaps.find(idx);
    if (m == methodMaps.end()) {
        return nullptr;
    }
    return m->second;
}

void ReadClassDataHeader(const uint8_t **pData,
                         DexClassDataHeader *pHeader) {
    pHeader->staticFieldsSize = readUnsignedLeb128(pData);
    pHeader->instanceFieldsSize = readUnsignedLeb128(pData);
    pHeader->directMethodsSize = readUnsignedLeb128(pData);
    pHeader->virtualMethodsSize = readUnsignedLeb128(pData);
}

void ReadClassDataField(const uint8_t **pData, DexField *pField) {
    pField->fieldIdx = readUnsignedLeb128(pData);
    pField->accessFlags = readUnsignedLeb128(pData);
}

void ReadClassDataMethod(const uint8_t **pData, DexMethod *pMethod) {
    pMethod->methodIdx = readUnsignedLeb128(pData);
    pMethod->accessFlags = readUnsignedLeb128(pData);
    pMethod->codeOff = readUnsignedLeb128(pData);
}

// 读取的时候会让pData指针产生移动
DexClassData *ReadClassData(const uint8_t **pData) {

    DexClassDataHeader header;

    if (*pData == NULL) {
        DexClassData *result = new DexClassData;
        memset(result, 0, sizeof(DexClassData));
        return result;

    }

    ReadClassDataHeader(pData, &header);

    // 这个resultSize应该为最大长度
    size_t resultSize = sizeof(DexClassData) + (header.staticFieldsSize * sizeof(DexField)) +
                        (header.instanceFieldsSize * sizeof(DexField)) +
                        (header.directMethodsSize * sizeof(DexMethod)) +
                        (header.virtualMethodsSize * sizeof(DexMethod));

    DexClassData *result = (DexClassData *) new char[resultSize];

    uint8_t *ptr = ((uint8_t *) result) + sizeof(DexClassData);

    result->header = header;

    if (header.staticFieldsSize != 0) {
        result->staticFields = (DexField *) ptr;
        ptr += header.staticFieldsSize * sizeof(DexField);
    } else {
        result->staticFields = NULL;
    }

    if (header.instanceFieldsSize != 0) {
        result->instanceFields = (DexField *) ptr;
        ptr += header.instanceFieldsSize * sizeof(DexField);
    } else {
        result->instanceFields = NULL;
    }

    if (header.directMethodsSize != 0) {
        result->directMethods = (DexMethod *) ptr;
        ptr += header.directMethodsSize * sizeof(DexMethod);
    } else {
        result->directMethods = NULL;
    }

    if (header.virtualMethodsSize != 0) {
        result->virtualMethods = (DexMethod *) ptr;
    } else {
        result->virtualMethods = NULL;
    }

    for (uint32_t i = 0; i < header.staticFieldsSize; i++) {
        ReadClassDataField(pData, &result->staticFields[i]);
    }

    for (uint32_t i = 0; i < header.instanceFieldsSize; i++) {
        ReadClassDataField(pData, &result->instanceFields[i]);
    }

    for (uint32_t i = 0; i < header.directMethodsSize; i++) {
        ReadClassDataMethod(pData, &result->directMethods[i]);
    }

    for (uint32_t i = 0; i < header.virtualMethodsSize; i++) {
        ReadClassDataMethod(pData, &result->virtualMethods[i]);
    }

    return result;
}
