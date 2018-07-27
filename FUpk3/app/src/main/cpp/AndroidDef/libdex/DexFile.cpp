/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Access the contents of a .dex file.
 */

#include "DexFile.h"
#include "DexOptData.h"
#include "DexProto.h"
#include "DexCatch.h"
#include "Leb128.h"
#include "sha1.h"
#include "ZipArchive.h"

#include <zlib.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <jni.h>


/*
 * Verifying checksums is good, but it slows things down and causes us to
 * touch every page.  In the "optimized" world, it doesn't work at all,
 * because we rewrite the contents.
 */
static const bool kVerifyChecksum = false;
static const bool kVerifySignature = false;

/* (documented in header) */
char dexGetPrimitiveTypeDescriptorChar(PrimitiveType type) {
    const char* string = dexGetPrimitiveTypeDescriptor(type);

    return (string == NULL) ? '\0' : string[0];
}

/* (documented in header) */
const char* dexGetPrimitiveTypeDescriptor(PrimitiveType type) {
    switch (type) {
        case PRIM_VOID:    return "V";
        case PRIM_BOOLEAN: return "Z";
        case PRIM_BYTE:    return "B";
        case PRIM_SHORT:   return "S";
        case PRIM_CHAR:    return "C";
        case PRIM_INT:     return "I";
        case PRIM_LONG:    return "J";
        case PRIM_FLOAT:   return "F";
        case PRIM_DOUBLE:  return "D";
        default:           return NULL;
    }

    return NULL;
}

/* (documented in header) */
const char* dexGetBoxedTypeDescriptor(PrimitiveType type) {
    switch (type) {
        case PRIM_VOID:    return NULL;
        case PRIM_BOOLEAN: return "Ljava/lang/Boolean;";
        case PRIM_BYTE:    return "Ljava/lang/Byte;";
        case PRIM_SHORT:   return "Ljava/lang/Short;";
        case PRIM_CHAR:    return "Ljava/lang/Character;";
        case PRIM_INT:     return "Ljava/lang/Integer;";
        case PRIM_LONG:    return "Ljava/lang/Long;";
        case PRIM_FLOAT:   return "Ljava/lang/Float;";
        case PRIM_DOUBLE:  return "Ljava/lang/Double;";
        default:           return NULL;
    }
}

/* (documented in header) */
PrimitiveType dexGetPrimitiveTypeFromDescriptorChar(char descriptorChar) {
    switch (descriptorChar) {
        case 'V': return PRIM_VOID;
        case 'Z': return PRIM_BOOLEAN;
        case 'B': return PRIM_BYTE;
        case 'S': return PRIM_SHORT;
        case 'C': return PRIM_CHAR;
        case 'I': return PRIM_INT;
        case 'J': return PRIM_LONG;
        case 'F': return PRIM_FLOAT;
        case 'D': return PRIM_DOUBLE;
        default:  return PRIM_NOT;
    }
}

/* Return the UTF-8 encoded string with the specified string_id index,
 * also filling in the UTF-16 size (number of 16-bit code points).*/
const char* dexStringAndSizeById(const DexFile* pDexFile, u4 idx,
        u4* utf16Size) {
    const DexStringId* pStringId = dexGetStringId(pDexFile, idx);
    const u1* ptr = pDexFile->baseAddr + pStringId->stringDataOff;

    *utf16Size = readUnsignedLeb128(&ptr);
    return (const char*) ptr;
}

/*
 * Format an SHA-1 digest for printing.  tmpBuf must be able to hold at
 * least kSHA1DigestOutputLen bytes.
 */
const char* dvmSHA1DigestToStr(const unsigned char digest[], char* tmpBuf);

/*
 * Compute a SHA-1 digest on a range of bytes.
 */
static void dexComputeSHA1Digest(const unsigned char* data, size_t length,
    unsigned char digest[])
{
    SHA1_CTX context;
    SHA1Init(&context);
    SHA1Update(&context, data, length);
    SHA1Final(digest, &context);
}

/*
 * Format the SHA-1 digest into the buffer, which must be able to hold at
 * least kSHA1DigestOutputLen bytes.  Returns a pointer to the buffer,
 */
static const char* dexSHA1DigestToStr(const unsigned char digest[],char* tmpBuf)
{
    static const char hexDigit[] = "0123456789abcdef";
    char* cp;
    int i;

    cp = tmpBuf;
    for (i = 0; i < kSHA1DigestLen; i++) {
        *cp++ = hexDigit[digest[i] >> 4];
        *cp++ = hexDigit[digest[i] & 0x0f];
    }
    *cp++ = '\0';

    assert(cp == tmpBuf + kSHA1DigestOutputLen);

    return tmpBuf;
}

/*
 * Compute a hash code on a UTF-8 string, for use with internal hash tables.
 *
 * This may or may not be compatible with UTF-8 hash functions used inside
 * the Dalvik VM.
 *
 * The basic "multiply by 31 and add" approach does better on class names
 * than most other things tried (e.g. adler32).
 */
static u4 classDescriptorHash(const char* str)
{
    u4 hash = 1;

    while (*str != '\0')
        hash = hash * 31 + *str++;

    return hash;
}

/*
 * Add an entry to the class lookup table.  We hash the string and probe
 * until we find an open slot.
 */
static void classLookupAdd(DexFile* pDexFile, DexClassLookup* pLookup,
    int stringOff, int classDefOff, int* pNumProbes)
{
    const char* classDescriptor =
        (const char*) (pDexFile->baseAddr + stringOff);
    const DexClassDef* pClassDef =
        (const DexClassDef*) (pDexFile->baseAddr + classDefOff);
    u4 hash = classDescriptorHash(classDescriptor);
    int mask = pLookup->numEntries-1;
    int idx = hash & mask;

    /*
     * Find the first empty slot.  We oversized the table, so this is
     * guaranteed to finish.
     */
    int probes = 0;
    while (pLookup->table[idx].classDescriptorOffset != 0) {
        idx = (idx + 1) & mask;
        probes++;
    }
    //if (probes > 1)
    //    ALOGW("classLookupAdd: probes=%d", probes);

    pLookup->table[idx].classDescriptorHash = hash;
    pLookup->table[idx].classDescriptorOffset = stringOff;
    pLookup->table[idx].classDefOffset = classDefOff;
    *pNumProbes = probes;
}

/*
 * Create the class lookup hash table.
 *
 * Returns newly-allocated storage.
 */
DexClassLookup* dexCreateClassLookup(DexFile* pDexFile)
{
    DexClassLookup* pLookup;
    int allocSize;
    int i, numEntries;
    int numProbes, totalProbes, maxProbes;

    numProbes = totalProbes = maxProbes = 0;

    assert(pDexFile != NULL);

    /*
     * Using a factor of 3 results in far less probing than a factor of 2,
     * but almost doubles the flash storage requirements for the bootstrap
     * DEX files.  The overall impact on class loading performance seems
     * to be minor.  We could probably get some performance improvement by
     * using a secondary hash.
     */
    numEntries = dexRoundUpPower2(pDexFile->pHeader->classDefsSize * 2);
    allocSize = offsetof(DexClassLookup, table)
                    + numEntries * sizeof(pLookup->table[0]);

    pLookup = (DexClassLookup*) calloc(1, allocSize);
    if (pLookup == NULL)
        return NULL;
    pLookup->size = allocSize;
    pLookup->numEntries = numEntries;

    for (i = 0; i < (int)pDexFile->pHeader->classDefsSize; i++) {
        const DexClassDef* pClassDef;
        const char* pString;

        pClassDef = dexGetClassDef(pDexFile, i);
        pString = dexStringByTypeIdx(pDexFile, pClassDef->classIdx);

        classLookupAdd(pDexFile, pLookup,
            (u1*)pString - pDexFile->baseAddr,
            (u1*)pClassDef - pDexFile->baseAddr, &numProbes);

        if (numProbes > maxProbes)
            maxProbes = numProbes;
        totalProbes += numProbes;
    }

    ALOGV("Class lookup: classes=%d slots=%d (%d%% occ) alloc=%d"
         " total=%d max=%d",
        pDexFile->pHeader->classDefsSize, numEntries,
        (100 * pDexFile->pHeader->classDefsSize) / numEntries,
        allocSize, totalProbes, maxProbes);

    return pLookup;
}


/*
 * Set up the basic raw data pointers of a DexFile. This function isn't
 * meant for general use.
 */
void dexFileSetupBasicPointers(DexFile* pDexFile, const u1* data) {
    DexHeader *pHeader = (DexHeader*) data;

    pDexFile->baseAddr = data;
    pDexFile->pHeader = pHeader;
    pDexFile->pStringIds = (const DexStringId*) (data + pHeader->stringIdsOff);
    pDexFile->pTypeIds = (const DexTypeId*) (data + pHeader->typeIdsOff);
    pDexFile->pFieldIds = (const DexFieldId*) (data + pHeader->fieldIdsOff);
    pDexFile->pMethodIds = (const DexMethodId*) (data + pHeader->methodIdsOff);
    pDexFile->pProtoIds = (const DexProtoId*) (data + pHeader->protoIdsOff);
    pDexFile->pClassDefs = (const DexClassDef*) (data + pHeader->classDefsOff);
    pDexFile->pLinkData = (const DexLink*) (data + pHeader->linkOff);
}

/*
 * Parse an optimized or unoptimized .dex file sitting in memory.  This is
 * called after the byte-ordering and structure alignment has been fixed up.
 *
 * On success, return a newly-allocated DexFile.
 */
DexFile* dexFileParse(const u1* data, size_t length, int flags)
{
    DexFile* pDexFile = NULL;
    const DexHeader* pHeader;
    const u1* magic;
    int result = -1;

    if (length < sizeof(DexHeader)) {
        ALOGE("too short to be a valid .dex");
        goto bail;      /* bad file format */
    }

    pDexFile = new DexFile;
    if (pDexFile == NULL)
        goto bail;      /* alloc failure */
    memset(pDexFile, 0, sizeof(DexFile));

    /*
     * Peel off the optimized header.
     */
    if (memcmp(data, DEX_OPT_MAGIC, 4) == 0) {
        magic = data;
        if (memcmp(magic+4, DEX_OPT_MAGIC_VERS, 4) != 0) {
            ALOGE("bad opt version (0x%02x %02x %02x %02x)",
                 magic[4], magic[5], magic[6], magic[7]);
            goto bail;
        }

        pDexFile->pOptHeader = (const DexOptHeader*) data;
        ALOGV("Good opt header, DEX offset is %d, flags=0x%02x",
            pDexFile->pOptHeader->dexOffset, pDexFile->pOptHeader->flags);

        /* parse the optimized dex file tables */
        if (!dexParseOptData(data, length, pDexFile))
            goto bail;

        /* ignore the opt header and appended data from here on out */
        data += pDexFile->pOptHeader->dexOffset;
        length -= pDexFile->pOptHeader->dexOffset;
        if (pDexFile->pOptHeader->dexLength > length) {
            ALOGE("File truncated? stored len=%d, rem len=%d",
                pDexFile->pOptHeader->dexLength, (int) length);
            goto bail;
        }
        length = pDexFile->pOptHeader->dexLength;
    }

    dexFileSetupBasicPointers(pDexFile, data);
    pHeader = pDexFile->pHeader;

    if (!dexHasValidMagic(pHeader)) {
        goto bail;
    }

    /*
     * Verify the checksum(s).  This is reasonably quick, but does require
     * touching every byte in the DEX file.  The base checksum changes after
     * byte-swapping and DEX optimization.
     */
    if (flags & kDexParseVerifyChecksum) {
        u4 adler = dexComputeChecksum(pHeader);
        if (adler != pHeader->checksum) {
            ALOGE("ERROR: bad checksum (%08x vs %08x)",
                adler, pHeader->checksum);
            if (!(flags & kDexParseContinueOnError))
                goto bail;
        } else {
            ALOGV("+++ adler32 checksum (%08x) verified", adler);
        }

        const DexOptHeader* pOptHeader = pDexFile->pOptHeader;
        if (pOptHeader != NULL) {
            adler = dexComputeOptChecksum(pOptHeader);
            if (adler != pOptHeader->checksum) {
                ALOGE("ERROR: bad opt checksum (%08x vs %08x)",
                    adler, pOptHeader->checksum);
                if (!(flags & kDexParseContinueOnError))
                    goto bail;
            } else {
                ALOGV("+++ adler32 opt checksum (%08x) verified", adler);
            }
        }
    }

    /*
     * Verify the SHA-1 digest.  (Normally we don't want to do this --
     * the digest is used to uniquely identify the original DEX file, and
     * can't be computed for verification after the DEX is byte-swapped
     * and optimized.)
     */
    if (kVerifySignature) {
        unsigned char sha1Digest[kSHA1DigestLen];
        const int nonSum = sizeof(pHeader->magic) + sizeof(pHeader->checksum) +
                            kSHA1DigestLen;

        dexComputeSHA1Digest(data + nonSum, length - nonSum, sha1Digest);
        if (memcmp(sha1Digest, pHeader->signature, kSHA1DigestLen) != 0) {
            char tmpBuf1[kSHA1DigestOutputLen];
            char tmpBuf2[kSHA1DigestOutputLen];
            ALOGE("ERROR: bad SHA1 digest (%s vs %s)",
                dexSHA1DigestToStr(sha1Digest, tmpBuf1),
                dexSHA1DigestToStr(pHeader->signature, tmpBuf2));
            if (!(flags & kDexParseContinueOnError))
                goto bail;
        } else {
            ALOGV("+++ sha1 digest verified");
        }
    }

    if (pHeader->fileSize != length) {
        ALOGE("ERROR: stored file size (%d) != expected (%d)",
            (int) pHeader->fileSize, (int) length);
        if (!(flags & kDexParseContinueOnError))
            goto bail;
    }

    if (pHeader->classDefsSize == 0) {
        ALOGE("ERROR: DEX file has no classes in it, failing");
        goto bail;
    }

    /*
     * Success!
     */
    result = 0;

bail:
    if (result != 0 && pDexFile != NULL) {
        dexFileFree(pDexFile);
        pDexFile = NULL;
    }
    return pDexFile;
}

/*
 * Free up the DexFile and any associated data structures.
 *
 * Note we may be called with a partially-initialized DexFile.
 */
void dexFileFree(DexFile* pDexFile)
{
    if (pDexFile == NULL)
        return;

    delete pDexFile;
}

/*
 * Look up a class definition entry by descriptor.
 *
 * "descriptor" should look like "Landroid/debug/Stuff;".
 */
const DexClassDef* dexFindClass(const DexFile* pDexFile,
    const char* descriptor)
{
    const DexClassLookup* pLookup = pDexFile->pClassLookup;
    u4 hash;
    int idx, mask;

    hash = classDescriptorHash(descriptor);
    mask = pLookup->numEntries - 1;
    idx = hash & mask;

    /*
     * Search until we find a matching entry or an empty slot.
     */
    while (true) {
        int offset;

        offset = pLookup->table[idx].classDescriptorOffset;
        if (offset == 0)
            return NULL;

        if (pLookup->table[idx].classDescriptorHash == hash) {
            const char* str;

            str = (const char*) (pDexFile->baseAddr + offset);
            if (strcmp(str, descriptor) == 0) {
                return (const DexClassDef*)
                    (pDexFile->baseAddr + pLookup->table[idx].classDefOffset);
            }
        }

        idx = (idx + 1) & mask;
    }
}


/*
 * Compute the DEX file checksum for a memory-mapped DEX file.
 */
u4 dexComputeChecksum(const DexHeader* pHeader)
{
    const u1* start = (const u1*) pHeader;

    uLong adler = adler32(0L, Z_NULL, 0);
    const int nonSum = sizeof(pHeader->magic) + sizeof(pHeader->checksum);

    return (u4) adler32(adler, start + nonSum, pHeader->fileSize - nonSum);
}

/*
 * Compute the size, in bytes, of a DexCode.
 */
size_t dexGetDexCodeSize(const DexCode* pCode)
{
    /*
     * The catch handler data is the last entry.  It has a variable number
     * of variable-size pieces, so we need to create an iterator.
     */
    u4 handlersSize;
    u4 offset;
    u4 ui;

    if (pCode->triesSize != 0) {
        handlersSize = dexGetHandlersSize(pCode);
        offset = dexGetFirstHandlerOffset(pCode);
    } else {
        handlersSize = 0;
        offset = 0;
    }

    for (ui = 0; ui < handlersSize; ui++) {
        DexCatchIterator iterator;
        dexCatchIteratorInit(&iterator, pCode, offset);
        offset = dexCatchIteratorGetEndOffset(&iterator, pCode);
    }

    const u1* handlerData = dexGetCatchHandlerData(pCode);

    //ALOGD("+++ pCode=%p handlerData=%p last offset=%d",
    //    pCode, handlerData, offset);

    /* return the size of the catch handler + everything before it */
    return (handlerData - (u1*) pCode) + offset;
}

/*
 * Round up to the next highest power of 2.
 *
 * Found on http://graphics.stanford.edu/~seander/bithacks.html.
 */
u4 dexRoundUpPower2(u4 val)
{
    val--;
    val |= val >> 1;
    val |= val >> 2;
    val |= val >> 4;
    val |= val >> 8;
    val |= val >> 16;
    val++;

    return val;
}


/* return the DexMapList of the file, if any */
DEX_INLINE const DexMapList* dexGetMap(const DexFile* pDexFile) {
    u4 mapOff = pDexFile->pHeader->mapOff;

    if (mapOff == 0) {
        return NULL;
    } else {
        return (const DexMapList*) (pDexFile->baseAddr + mapOff);
    }
}


/* return the const char* string data referred to by the given string_id */
DEX_INLINE const char* dexGetStringData(const DexFile* pDexFile,
                                        const DexStringId* pStringId) {
    const u1* ptr = pDexFile->baseAddr + pStringId->stringDataOff;

    // Skip the uleb128 length.
    while (*(ptr++) > 0x7f) /* empty */ ;

    return (const char*) ptr;
}


/* return the StringId with the specified index */
DEX_INLINE const DexStringId* dexGetStringId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->stringIdsSize);
    return &pDexFile->pStringIds[idx];
}

/* return the UTF-8 encoded string with the specified string_id index */
DEX_INLINE const char* dexStringById(const DexFile* pDexFile, u4 idx) {
    const DexStringId* pStringId = dexGetStringId(pDexFile, idx);
    return dexGetStringData(pDexFile, pStringId);
}


/* return the TypeId with the specified index */
DEX_INLINE const DexTypeId* dexGetTypeId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->typeIdsSize);
    return &pDexFile->pTypeIds[idx];
}

/*
 * Get the descriptor string associated with a given type index.
 * The caller should not free() the returned string.
 */
DEX_INLINE const char* dexStringByTypeIdx(const DexFile* pDexFile, u4 idx) {
    const DexTypeId* typeId = dexGetTypeId(pDexFile, idx);
    return dexStringById(pDexFile, typeId->descriptorIdx);
}

/* return the MethodId with the specified index */
DEX_INLINE const DexMethodId* dexGetMethodId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->methodIdsSize);
    return &pDexFile->pMethodIds[idx];
}


/* return the FieldId with the specified index */
DEX_INLINE const DexFieldId* dexGetFieldId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->fieldIdsSize);
    return &pDexFile->pFieldIds[idx];
}

/* return the ProtoId with the specified index */
DEX_INLINE const DexProtoId* dexGetProtoId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->protoIdsSize);
    return &pDexFile->pProtoIds[idx];
}

/*
 * Get the parameter list from a ProtoId. The returns NULL if the ProtoId
 * does not have a parameter list.
 */
DEX_INLINE const DexTypeList* dexGetProtoParameters(
        const DexFile *pDexFile, const DexProtoId* pProtoId) {
    if (pProtoId->parametersOff == 0) {
        return NULL;
    }
    return (const DexTypeList*)
            (pDexFile->baseAddr + pProtoId->parametersOff);
}


/* return the ClassDef with the specified index */
DEX_INLINE const DexClassDef* dexGetClassDef(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->classDefsSize);
    return &pDexFile->pClassDefs[idx];
}

/* given a ClassDef pointer, recover its index */
DEX_INLINE u4 dexGetIndexForClassDef(const DexFile* pDexFile,
                                     const DexClassDef* pClassDef)
{
    assert(pClassDef >= pDexFile->pClassDefs &&
           pClassDef < pDexFile->pClassDefs + pDexFile->pHeader->classDefsSize);
    return pClassDef - pDexFile->pClassDefs;
}


/* get the interface list for a DexClass */
DEX_INLINE const DexTypeList* dexGetInterfacesList(const DexFile* pDexFile,
                                                   const DexClassDef* pClassDef)
{
    if (pClassDef->interfacesOff == 0)
        return NULL;
    return (const DexTypeList*)
            (pDexFile->baseAddr + pClassDef->interfacesOff);
}
/* return the Nth entry in a DexTypeList. */
DEX_INLINE const DexTypeItem* dexGetTypeItem(const DexTypeList* pList,
                                             u4 idx)
{
    assert(idx < pList->size);
    return &pList->list[idx];
}
/* return the type_idx for the Nth entry in a TypeList */
DEX_INLINE u4 dexTypeListGetIdx(const DexTypeList* pList, u4 idx) {
    const DexTypeItem* pItem = dexGetTypeItem(pList, idx);
    return pItem->typeIdx;
}


/* get the static values list for a DexClass */
DEX_INLINE const DexEncodedArray* dexGetStaticValuesList(
        const DexFile* pDexFile, const DexClassDef* pClassDef)
{
    if (pClassDef->staticValuesOff == 0)
        return NULL;
    return (const DexEncodedArray*)
            (pDexFile->baseAddr + pClassDef->staticValuesOff);
}

/* get the annotations directory item for a DexClass */
DEX_INLINE const DexAnnotationsDirectoryItem* dexGetAnnotationsDirectoryItem(
        const DexFile* pDexFile, const DexClassDef* pClassDef)
{
    if (pClassDef->annotationsOff == 0)
        return NULL;
    return (const DexAnnotationsDirectoryItem*)
            (pDexFile->baseAddr + pClassDef->annotationsOff);
}

/* get the source file string */
DEX_INLINE const char* dexGetSourceFile(
        const DexFile* pDexFile, const DexClassDef* pClassDef)
{
    if (pClassDef->sourceFileIdx == 0xffffffff)
        return NULL;
    return dexStringById(pDexFile, pClassDef->sourceFileIdx);
}


/* Get the list of "tries" for the given DexCode. */
DEX_INLINE const DexTry* dexGetTries(const DexCode* pCode) {
    const u2* insnsEnd = &pCode->insns[pCode->insnsSize];

    // Round to four bytes.
    if ((((uintptr_t) insnsEnd) & 3) != 0) {
        insnsEnd++;
    }

    return (const DexTry*) insnsEnd;
}

/* Get the base of the encoded data for the given DexCode. */
DEX_INLINE const u1* dexGetCatchHandlerData(const DexCode* pCode) {
    const DexTry* pTries = dexGetTries(pCode);
    return (const u1*) &pTries[pCode->triesSize];
}


/* get a pointer to the start of the debugging data */
DEX_INLINE const u1* dexGetDebugInfoStream(const DexFile* pDexFile,
                                           const DexCode* pCode)
{
    if (pCode->debugInfoOff == 0) {
        return NULL;
    } else {
        return pDexFile->baseAddr + pCode->debugInfoOff;
    }
}

/* DexClassDef convenience - get class descriptor */
DEX_INLINE const char* dexGetClassDescriptor(const DexFile* pDexFile,
                                             const DexClassDef* pClassDef)
{
    return dexStringByTypeIdx(pDexFile, pClassDef->classIdx);
}


/* DexClassDef convenience - get superclass descriptor */
DEX_INLINE const char* dexGetSuperClassDescriptor(const DexFile* pDexFile,
                                                  const DexClassDef* pClassDef)
{
    if (pClassDef->superclassIdx == 0)
        return NULL;
    return dexStringByTypeIdx(pDexFile, pClassDef->superclassIdx);
}

/* DexClassDef convenience - get class_data_item pointer */
DEX_INLINE const u1* dexGetClassData(const DexFile* pDexFile,
                                     const DexClassDef* pClassDef)
{
    if (pClassDef->classDataOff == 0)
        return NULL;
    return (const u1*) (pDexFile->baseAddr + pClassDef->classDataOff);
}


/* Get an annotation set at a particular offset. */
DEX_INLINE const DexAnnotationSetItem* dexGetAnnotationSetItem(
        const DexFile* pDexFile, u4 offset)
{
    if (offset == 0) {
        return NULL;
    }
    return (const DexAnnotationSetItem*) (pDexFile->baseAddr + offset);
}
/* get the class' annotation set */
DEX_INLINE const DexAnnotationSetItem* dexGetClassAnnotationSet(
        const DexFile* pDexFile, const DexAnnotationsDirectoryItem* pAnnoDir)
{
    return dexGetAnnotationSetItem(pDexFile, pAnnoDir->classAnnotationsOff);
}


/* get the class' field annotation list */
DEX_INLINE const DexFieldAnnotationsItem* dexGetFieldAnnotations(
        const DexFile* pDexFile, const DexAnnotationsDirectoryItem* pAnnoDir)
{
    if (pAnnoDir->fieldsSize == 0)
        return NULL;

    // Skip past the header to the start of the field annotations.
    return (const DexFieldAnnotationsItem*) &pAnnoDir[1];
}

/* get field annotation list size */
DEX_INLINE int dexGetFieldAnnotationsSize(const DexFile* pDexFile,
                                          const DexAnnotationsDirectoryItem* pAnnoDir)
{
    return pAnnoDir->fieldsSize;
}


/* return a pointer to the field's annotation set */
DEX_INLINE const DexAnnotationSetItem* dexGetFieldAnnotationSetItem(
        const DexFile* pDexFile, const DexFieldAnnotationsItem* pItem)
{
    return dexGetAnnotationSetItem(pDexFile, pItem->annotationsOff);
}

/* get the class' method annotation list */
DEX_INLINE const DexMethodAnnotationsItem* dexGetMethodAnnotations(
        const DexFile* pDexFile, const DexAnnotationsDirectoryItem* pAnnoDir)
{
    if (pAnnoDir->methodsSize == 0)
        return NULL;

    /*
     * Skip past the header and field annotations to the start of the
     * method annotations.
     */
    const u1* addr = (const u1*) &pAnnoDir[1];
    addr += pAnnoDir->fieldsSize * sizeof (DexFieldAnnotationsItem);
    return (const DexMethodAnnotationsItem*) addr;
}


/* get method annotation list size */
DEX_INLINE int dexGetMethodAnnotationsSize(const DexFile* pDexFile,
                                           const DexAnnotationsDirectoryItem* pAnnoDir)
{
    return pAnnoDir->methodsSize;
}

/* return a pointer to the method's annotation set */
DEX_INLINE const DexAnnotationSetItem* dexGetMethodAnnotationSetItem(
        const DexFile* pDexFile, const DexMethodAnnotationsItem* pItem)
{
    return dexGetAnnotationSetItem(pDexFile, pItem->annotationsOff);
}


/* get the class' parameter annotation list */
DEX_INLINE const DexParameterAnnotationsItem* dexGetParameterAnnotations(
        const DexFile* pDexFile, const DexAnnotationsDirectoryItem* pAnnoDir)
{
    if (pAnnoDir->parametersSize == 0)
        return NULL;

    /*
     * Skip past the header, field annotations, and method annotations
     * to the start of the parameter annotations.
     */
    const u1* addr = (const u1*) &pAnnoDir[1];
    addr += pAnnoDir->fieldsSize * sizeof (DexFieldAnnotationsItem);
    addr += pAnnoDir->methodsSize * sizeof (DexMethodAnnotationsItem);
    return (const DexParameterAnnotationsItem*) addr;
}

/* get method annotation list size */
DEX_INLINE int dexGetParameterAnnotationsSize(const DexFile* pDexFile,
                                              const DexAnnotationsDirectoryItem* pAnnoDir)
{
    return pAnnoDir->parametersSize;
}


/* return the parameter annotation ref list */
DEX_INLINE const DexAnnotationSetRefList* dexGetParameterAnnotationSetRefList(
        const DexFile* pDexFile, const DexParameterAnnotationsItem* pItem)
{
    if (pItem->annotationsOff == 0) {
        return NULL;
    }
    return (const DexAnnotationSetRefList*) (pDexFile->baseAddr + pItem->annotationsOff);
}

/* get method annotation list size */
DEX_INLINE int dexGetParameterAnnotationSetRefSize(const DexFile* pDexFile,
                                                   const DexParameterAnnotationsItem* pItem)
{
    if (pItem->annotationsOff == 0) {
        return 0;
    }
    return dexGetParameterAnnotationSetRefList(pDexFile, pItem)->size;
}


/* return the Nth entry from an annotation set ref list */
DEX_INLINE const DexAnnotationSetRefItem* dexGetParameterAnnotationSetRef(
        const DexAnnotationSetRefList* pList, u4 idx)
{
    assert(idx < pList->size);
    return &pList->list[idx];
}

/* given a DexAnnotationSetRefItem, return the DexAnnotationSetItem */
DEX_INLINE const DexAnnotationSetItem* dexGetSetRefItemItem(
        const DexFile* pDexFile, const DexAnnotationSetRefItem* pItem)
{
    return dexGetAnnotationSetItem(pDexFile, pItem->annotationsOff);
}

/* return the Nth annotation offset from a DexAnnotationSetItem */
DEX_INLINE u4 dexGetAnnotationOff(
        const DexAnnotationSetItem* pAnnoSet, u4 idx)
{
    assert(idx < pAnnoSet->size);
    return pAnnoSet->entries[idx];
}


/* return the Nth annotation item from a DexAnnotationSetItem */
DEX_INLINE const DexAnnotationItem* dexGetAnnotationItem(
        const DexFile* pDexFile, const DexAnnotationSetItem* pAnnoSet, u4 idx)
{
    u4 offset = dexGetAnnotationOff(pAnnoSet, idx);
    if (offset == 0) {
        return NULL;
    }
    return (const DexAnnotationItem*) (pDexFile->baseAddr + offset);
}


