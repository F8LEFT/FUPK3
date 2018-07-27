//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// imple for file open/write (with encrypt)
//===----------------------------------------------------------------------===//


#ifndef FUPK3_MYFILE_H
#define FUPK3_MYFILE_H

#include <stdio.h>
//提供一系列新的api，与文件钩子做对抗
void *mymemcpy(void *dest, const void *src, size_t n);
FILE *  myfopen(const char * path,const char * mode);
size_t myfwrite(const void* buffer, size_t size, size_t count, FILE* stream);
int myfclose( FILE *fp );
size_t myfread ( void *buffer, size_t size, size_t count, FILE *stream);
int myfflush(FILE *stream);
size_t myfwrite(int pos, const void* buffer, size_t size, size_t count, FILE *stream);
size_t myfread(int pos, void *buffer, size_t size, size_t count, FILE *stream);

#endif //FUPK3_MYFILE_H
