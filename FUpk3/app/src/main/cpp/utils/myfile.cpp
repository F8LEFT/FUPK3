//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//



#include "myfile.h"

#define encryptKey 0xF8         // all byte will xor encryptKey
//#define encryptKey 0            // all byte will xor encryptKey

void *mymemcpy(void *dest, const void *src, size_t n) {
    char* dst = (char*)dest; char* s = (char*)src;
    for (int i = 0; i < n; ++i) {
        dst[i] = s[i];
    }
    return dest;
}


FILE *  myfopen(const char * path,const char * mode) {
    return fopen(path, mode);
}

size_t myfwrite(const void* buffer, size_t size, size_t count, FILE* stream) {
    char *tmp = new char[size * count];
    mymemcpy(tmp, buffer, size * count);
    for (size_t i = 0; i < size * count; ++i) {
        tmp[i] ^= encryptKey;
    }
    size_t rel = fwrite(tmp, size, count, stream);
    delete []tmp;
    return rel;
}

int myfclose( FILE *fp ) {
    return fclose(fp);
}

size_t myfread ( void *buffer, size_t size, size_t count, FILE *stream) {
    size_t rSize = fread(buffer, size, count, stream);
    char* tmp = (char*)buffer;
    for (size_t i = 0; i < rSize * size; ++i) {
        tmp[i] ^= encryptKey;
    }
    return rSize;
}

int myfflush(FILE *stream) {
    return fflush(stream);
}

size_t myfwrite(int pos, const void* buffer, size_t size, size_t count, FILE *stream) {
    if (!fseek(stream, pos, SEEK_SET)) {
        return myfwrite(buffer, size, count, stream);
    }
    return 0;
}

size_t myfread(int pos, void *buffer, size_t size, size_t count, FILE *stream) {
    if (!fseek(stream, pos, SEEK_SET)) {
        return myfread(buffer, size, count, stream);
    }
    return 0;
}
