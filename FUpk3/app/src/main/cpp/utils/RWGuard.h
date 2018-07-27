//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/12.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// Check the target address is valid(only check for read at this time)
//===----------------------------------------------------------------------===//


#ifndef FUPK3_RWGUARD_H
#define FUPK3_RWGUARD_H


#include <cstddef>
#include <vector>

class RWGuard {
public:
    static RWGuard* getInstance() {
        static RWGuard* mInstance = nullptr;
        if (mInstance == nullptr) {
            mInstance = new RWGuard();
        }
        return mInstance;
    }

    bool hasFlag(unsigned int ptr, int flag);
    bool isReadable(unsigned int ptr);
    void* assertReadable(void* ptr);
    // parse from map and get memory information
    bool reflesh();
private:
    RWGuard();

    // only valid in 32 bit
    struct PMapInfo {
        unsigned int start;
        unsigned int end;
        unsigned int flag;
    };
    std::vector<PMapInfo> maps;
};


#endif //FUPK3_RWGUARD_H
