//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/12.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//


#include "RWGuard.h"
#include <sys/mman.h>
#include <cstdio>
#include <assert.h>

#define MAX_BUF 128
#define MAPS "/proc/self/maps"

RWGuard::RWGuard() {
    reflesh();
}

bool RWGuard::reflesh() {
    maps.clear();

    char buf[MAX_BUF];
    auto fp = fopen(MAPS, "r");
    if (fp == nullptr) {
        return false;
    }

    unsigned int start, end, size, id, i1, i2;
    char prv[5], fs[MAX_BUF];
    // b6f40000-b6f87000 r-xp 00000000 b3:19 717        /system/lib/libc.so
    while(fgets(buf, MAX_BUF, fp)) {
        sscanf(buf, "%08x-%08x %04s %08x %02x:%02x %d\t%s", &start, &end, prv, &size, &i1, &i2, &id, fs);
        PMapInfo info = {start, end, 0};
        for(int i = 0; i < 4; i++) {
            switch (prv[i]) {
                case 'r': info.flag |= PROT_READ;
                    break;
                case 'w': info.flag |= PROT_WRITE;
                    break;
                case 'x': info.flag |= PROT_EXEC;
                    break;
                case 'p': info.flag |= PROT_SEM;
                    break;
                default:
                    break;
            }
        }
        maps.push_back(info);
    }
    return true;
}

bool RWGuard::isReadable(unsigned int ptr) {
    return hasFlag(ptr, PROT_READ);
}

bool RWGuard::hasFlag(unsigned int ptr, int flag) {
    for(auto it = maps.begin(), itEnd = maps.end(); it != itEnd; it++) {
        if (ptr >= it->start && ptr < it->end) {
            if (it->flag && flag) {
                return true;
            } else {
                return false;
            }
        }
    }
    return false;
}

void *RWGuard::assertReadable(void *ptr) {
    if (isReadable(reinterpret_cast<unsigned int>(ptr))) {
        return ptr;
    }
    assert(false && "Target address is not readable");
    return nullptr;
}

