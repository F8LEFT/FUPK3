//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/11.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//


#include "DexFixer.h"
#include "DexDumper.h"
#include "ClassDefBuilder.h"

// for some class, is not valid, may just want to let me crash
static char *skipClazz [] = {
        // Ali protect, crash clazz
        "Lz/z/z/z0;",
        nullptr
};

DexFixer::DexFixer(u1 *dexData, int length) {
    parseDexFile(dexData);
    mDexFile = &dexFile;

    start = reinterpret_cast<size_t>(dexData);
    end = start + length;
}

DexFixer::~DexFixer() {
}

bool DexFixer::fixAll() {
    // I want to fix all data, but Not all the class is valid, just skip
    // the data outside of file
    const DexHeader *pHeader = mDexFile->pHeader;

    int num_class_defs = pHeader->classDefsSize;
    for (int i = 0; i < num_class_defs; ++i) {
        DexClassDef *classDef = const_cast<DexClassDef *>(dexGetClassDef(mDexFile, i));

        const char *descriptor = dexGetClassDescriptor(mDexFile, classDef);
        for(char** sc = skipClazz; *sc != nullptr; sc ++) {
            if (strcmp(*sc, descriptor) == 0) {
                FLOGI("Skip for class %s", descriptor);
                classDef->classDataOff = 0;
                continue;
            } else {
                FLOGI("Fix class %s", descriptor);
            }
        }
        if (classDef->classDataOff) {               //calculate methods, 寻找insns结构的内容，然后进行修复
            const u1 *data = dexGetClassData(mDexFile, classDef);
            DexClassData *pData = ReadClassData(&data);
            if (pData->directMethods) {
                for (int j = 0; j < pData->header.directMethodsSize; ++j) {
                    DexMethod method = pData->directMethods[j];
                    if (method.codeOff == 0 || method.accessFlags & (ACC_NATIVE | ACC_ABSTRACT)) {
                        continue;
                    }
                    auto dexCode = (DexCode *)dexGetCode(mDexFile, &method);
                    // usually, debug info is no need to used, anyway, just fix in server
//                    dexCode->debugInfoOff = 0;
                    fixOptInSns((u2 *) dexCode->insns, dexCode->insnsSize);
                }
            }
            if (pData->virtualMethods) {
                for (int j = 0; j < pData->header.virtualMethodsSize; ++j) {
                    DexMethod method = pData->virtualMethods[j];
                    if (method.codeOff == 0 || method.accessFlags & (ACC_NATIVE | ACC_ABSTRACT)) {
                        continue;
                    }
                    auto dexCode = (DexCode *)dexGetCode(mDexFile, &method);
//                    dexCode->debugInfoOff = 0;
                    fixOptInSns((u2 *) dexCode->insns, dexCode->insnsSize);
                }
            }

            delete [](u1*)pData;
        }

    }

//    fixShaAndChecksum();
    return true;
}

bool DexFixer::fixOptInSns(u2 *insns, size_t len) {
    while (len > 0) {
        Opcode opc = dexOpcodeFromCodeUnit(*insns);
        size_t width = dexGetWidthFromInstruction(insns);
        if (!isValid(reinterpret_cast<size_t>(insns + width))) {
            return false;
        }
        // essential substitutions:
        switch (opc) {
            case OP_INVOKE_OBJECT_INIT_RANGE:           // invoke-direct for Object
                rewriteInvokeObjectInit(insns, mDexFile);
                break;
            case OP_RETURN_VOID_BARRIER:                        // return-void-barrier
                rewriteReturnVoid(insns, mDexFile);
                break;
            case OP_IGET_QUICK:                                //instField
            case OP_IGET_VOLATILE:
            case OP_IGET_WIDE_QUICK:
            case OP_IGET_WIDE_VOLATILE:
            case OP_IGET_OBJECT_QUICK:
            case OP_IGET_OBJECT_VOLATILE:
            case OP_IPUT_QUICK:
            case OP_IPUT_VOLATILE:
            case OP_IPUT_WIDE_QUICK:
            case OP_IPUT_WIDE_VOLATILE:
            case OP_IPUT_OBJECT_QUICK:
            case OP_IPUT_OBJECT_VOLATILE:
                rewriteInstField(insns, mDexFile);
                break;
            case OP_SGET_VOLATILE:                           // staticField
            case OP_SGET_WIDE_VOLATILE:
            case OP_SGET_OBJECT_VOLATILE:
            case OP_SPUT_VOLATILE:
            case OP_SPUT_WIDE_VOLATILE:
            case OP_SPUT_OBJECT_VOLATILE:
                rewriteStaticField(insns, mDexFile);
                break;
            default:
                break;
        }
        // non-essential substitutios:
        switch (opc) {
            case OP_EXECUTE_INLINE:
            case OP_EXECUTE_INLINE_RANGE:
            case OP_INVOKE_VIRTUAL_QUICK:
            case OP_INVOKE_VIRTUAL_QUICK_RANGE:
            case OP_INVOKE_SUPER_QUICK:
            case OP_INVOKE_SUPER_QUICK_RANGE:
                rewriteQuickMethod(insns, mDexFile);
                break;
            default:
                break;
        }
        insns += width;
        len -= width;
    }
    return true;
}



u4 DexFixer::getDexOff(u1 *mem) {
    // dey
    if (*((u4 *) mem) == 0x0a796564) {
        return sizeof(DexOptHeader);
    }
    return 0;
}


void rewriteInvokeObjectInit(u2 *insns, DexFile *dexFile) {
    u2 origOp = insns[0];
    if (origOp != 0x01F0) {
        return;
    }
    u2 methodIdx = insns[1];
    const DexMethodId *method = dexGetMethodId(dexFile, methodIdx);
    const char* methodName = dexStringById(dexFile, method->nameIdx);
    const char *methodProto = dexStringById(dexFile,
                                            dexGetProtoId(dexFile, method->protoIdx)->shortyIdx);
    const char *methodClass = dexStringByTypeIdx(dexFile, method->classIdx);
    if (!strcmp(methodName, "<init>") && !strcmp(methodProto, "V") &&
        !strcmp(methodClass, "Ljava/lang/Object;")) {
        u2 vdst = insns[2];
        if (vdst > 15) {
            insns[0] = 0x0176;
        } else {
            insns[0] = 0x1070;
        }
    }
}

void rewriteReturnVoid(u2 *insns, DexFile *dexFile) {
    u2 origOp = insns[0];
    if ((origOp & 0xff) == OP_RETURN_VOID_BARRIER) {
        ((u1*)insns)[0] = OP_RETURN_VOID;
    }
}

void rewriteInstField(u2 *insns, DexFile *dexFile) {
    u2 volatileOpc = insns[0];
    Opcode origOp;
    bool match = false;
    // is volatileInst
    switch (volatileOpc & 0xff) {
        case OP_IGET_VOLATILE:
            origOp = OP_IGET;
            match = true;
            break;
        case OP_IGET_WIDE_VOLATILE:
            origOp = OP_IGET_WIDE;
            match = true;
            break;
        case OP_IGET_OBJECT_VOLATILE:
            origOp = OP_IGET_OBJECT;
            match = true;
            break;
        case OP_IPUT_VOLATILE:
            origOp = OP_IPUT;
            match = true;
            break;
        case OP_IPUT_WIDE_VOLATILE:
            origOp = OP_IPUT_WIDE;
            match = true;
            break;
        case OP_IPUT_OBJECT_VOLATILE:
            origOp = OP_IPUT_OBJECT;
            match = true;
            break;
    }

    if (match) {
        u2 fieldIdx = insns[1];
        const char *typestr = dexStringByTypeIdx(dexFile,
                                                 dexGetFieldId(dexFile, fieldIdx)->typeIdx);
        if (origOp == OP_IGET) {
            switch (*typestr) {
                case 'Z':
                    origOp = OP_IGET_BOOLEAN;
                    break;
                case 'B':
                    origOp = OP_IGET_BYTE;
                    break;
                case 'C':
                    origOp = OP_IGET_CHAR;
                    break;
                case 'S':
                    origOp = OP_IGET_SHORT;
                    break;
            }
        } else if (origOp == OP_IPUT) {
            switch (*typestr) {
                case 'Z':
                    origOp = OP_IPUT_BOOLEAN;
                    break;
                case 'B':
                    origOp = OP_IPUT_BYTE;
                    break;
                case 'C':
                    origOp = OP_IPUT_CHAR;
                    break;
                case 'S':
                    origOp = OP_IPUT_SHORT;
                    break;
            }
        }
        ((u1*)insns)[0] = origOp;
        return;
    }
    // is quick
    match = false;
    u2 quickOpc = insns[0];
    switch (quickOpc & 0xff) {
        case OP_IGET_QUICK:
            origOp = OP_IGET;
            match = true;
            break;
        case OP_IGET_WIDE_QUICK:
            origOp = OP_IGET_WIDE;
            match = true;
            break;
        case OP_IGET_OBJECT_QUICK:
            origOp = OP_IGET_OBJECT;
            match = true;
            break;
        case OP_IPUT_QUICK:
            origOp = OP_IPUT;
            match = true;
            break;
        case OP_IPUT_WIDE_QUICK:
            origOp = OP_IPUT_WIDE;
            match = true;
            break;
        case OP_IPUT_OBJECT_QUICK:
            origOp = OP_IPUT_OBJECT;
            match = true;
            break;
    }
    if (match) {
        LOGE("inst field -> quick opcode need to be fixed");
        return;
    }
}

void rewriteStaticField(u2 *insns, DexFile *dexFile) {
    u2 volatileOpc = insns[0];
    Opcode origOp;
    bool match = false;
    // is volatileStatic
    switch (volatileOpc & 0xff) {
        case OP_SGET_VOLATILE:
            origOp = OP_SGET;
            match = true;
            break;
        case OP_SGET_WIDE_VOLATILE:
            origOp = OP_SGET_WIDE;
            match = true;
            break;
        case OP_SGET_OBJECT_VOLATILE:
            origOp = OP_SGET_OBJECT;
            match = true;
            break;
        case OP_SPUT_VOLATILE:
            origOp = OP_SPUT;
            match = true;
            break;
        case OP_SPUT_WIDE_VOLATILE:
            origOp = OP_SPUT_WIDE;
            match = true;
            break;
        case OP_SPUT_OBJECT_VOLATILE:
            origOp = OP_SPUT_OBJECT;
            match = true;
            break;
    }
    if (match) {
        u2 fieldIdx = insns[1];
        const char *typestr = dexStringByTypeIdx(dexFile,
                                                 dexGetFieldId(dexFile, fieldIdx)->typeIdx);
//        int origOp;
        if (origOp == OP_SGET) {
            switch (*typestr) {
                case 'Z':
                    origOp = OP_SGET_BOOLEAN;
                    break;
                case 'B':
                    origOp = OP_SGET_BYTE;
                    break;
                case 'C':
                    origOp = OP_SGET_CHAR;
                    break;
                case 'S':
                    origOp = OP_SGET_SHORT;
                    break;
            }
        } else if (origOp == OP_SPUT) {
            switch (*typestr) {
                case 'Z':
                    origOp = OP_SPUT_BOOLEAN;
                    break;
                case 'B':
                    origOp = OP_SPUT_BYTE;
                    break;
                case 'C':
                    origOp = OP_SPUT_CHAR;
                    break;
                case 'S':
                    origOp = OP_SPUT_SHORT;
                    break;
            }
        }
        ((u1*)insns)[0] = origOp;
        return;
    }

    // is quick
    match = false;
    u2 quickOpc = insns[0];
    switch (quickOpc & 0xff) {
        case OP_IGET_VOLATILE:
            origOp = OP_IGET;
            match = true;
            break;
        case OP_IGET_WIDE_VOLATILE:
            origOp = OP_IGET_WIDE;
            match = true;
            break;
        case OP_IGET_OBJECT_VOLATILE:
            origOp = OP_IGET_OBJECT;
            match = true;
            break;
        case OP_IPUT_VOLATILE:
            origOp = OP_IPUT;
            match = true;
            break;
        case OP_IPUT_WIDE_VOLATILE:
            origOp = OP_IPUT_WIDE;
            match = true;
            break;
        case OP_IPUT_OBJECT_VOLATILE:
            origOp = OP_IPUT_OBJECT;
            match = true;
            break;
    }
    if (match) {
        LOGE("static field -> quick opcode need to be fixed");
        return;
    }
}

void rewriteQuickMethod(u2 *insns, DexFile *dexFile) {
    LOGE("method invoke -> quick opcode need to be fixed");
}

bool DexFixer::parseDexFile(u1 *memDex) {
    memset(&dexFile, 0, sizeof(DexFile));
    u4 dexOff = getDexOff(memDex);
    if (dexOff) {
        dexFile.pOptHeader = (DexOptHeader *) memDex;
    } else {
        dexFile.pOptHeader = nullptr;
    }
    dexFile.pHeader = (DexHeader *) (memDex + dexOff);
    dexFile.baseAddr = (u1*)dexFile.pHeader;

    const DexHeader *pDexHeader = dexFile.pHeader;
    dexFile.pStringIds = (DexStringId *) (pDexHeader->stringIdsOff + dexFile.baseAddr);
    dexFile.pTypeIds = (DexTypeId *) (pDexHeader->typeIdsOff + dexFile.baseAddr);
    dexFile.pFieldIds = (DexFieldId *) (pDexHeader->fieldIdsOff + dexFile.baseAddr);
    dexFile.pMethodIds = (DexMethodId *) (pDexHeader->methodIdsOff + dexFile.baseAddr);
    dexFile.pProtoIds = (DexProtoId *) (pDexHeader->protoIdsOff + dexFile.baseAddr);
    dexFile.pClassDefs = (DexClassDef *) (pDexHeader->classDefsOff + dexFile.baseAddr);
    dexFile.pLinkData = (DexLink *) (pDexHeader->linkOff + dexFile.baseAddr);

    return true;
}

bool DexFixer::isValid(size_t v) {
    return v >= start && v < end;
}

//bool OptFixer::fixShaAndChecksum() {
//    DexFile* pDexFile = &dexFile;
//    auto pHeader = (DexHeader *)pDexFile->pHeader;
//    auto pOptHeader = pDexFile->pOptHeader;
//
//    //-----Sha1
//    const int nonSum = sizeof(pHeader->magic) + sizeof(pHeader->checksum) + kSHA1DigestLen;
//    size_t sumLen = pHeader->dataSize + pHeader->dataOff;      //dex文件长度,怎么计算？
//    char* sumData = (char*)pDexFile->baseAddr;
//    if (pOptHeader != NULL) {
//        sumLen = pOptHeader->dexLength;
//    }
//    dexComputeSHA1Digest((unsigned char *) sumData + nonSum, sumLen - nonSum, pHeader->signature);
//
//    pHeader->checksum = dexComputeChecksum(pHeader);
//    if (pOptHeader != NULL) {
//        pOptHeader->checksum = dexComputeOptChecksum(pDexFile->pOptHeader);
//    }
//
//    return true;
//}