//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// The define for unpack.
//===----------------------------------------------------------------------===//


#ifndef FUPK3_FUPK_H
#define FUPK3_FUPK_H


#include "AndroidDef.h"
#include "Cookie.h"
#include "UnpackInfo.h"

#include <jni.h>
#include <string>


class Fupk {
public:
    Fupk(JNIEnv *env, std::string unpackRoot, jobject fupkObj);
    bool unpackAll();
private:
    // load unpack status from config file
    bool restoreLastStatus();

    JNIEnv* mEnv;
    std::string mRoot;
    jobject mUpkObj;
    Cookie mCookie;
    const char** mIgnoreCase;

    UnpackInfo mInfo;
};







#endif //FUPK3_FUPK_H
