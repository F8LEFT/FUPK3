//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/4.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "Fupk.h"

#include <AndroidDef/AndroidDef.h>
#include <sys/stat.h>
#include "DexDumper.h"
#include "DexFixer.h"
#include "utils/myfile.h"
#include "utils/RWGuard.h"

const char* gIgnoreCasee[] = {
        "de.robv.android.xposed.installer",
        "f8left",
        nullptr
};

// ================ define for Fuk ==================
Fupk::Fupk(JNIEnv *env, std::string unpackRoot, jobject fupkObj)
        :mInfo(unpackRoot + "/information.json"){
    mEnv = env;
    mRoot = unpackRoot;
    mUpkObj = fupkObj;
    mIgnoreCase = gIgnoreCasee;
}

bool Fupk::unpackAll() {
    FLOGD("================ Start to dump all dex files ===============");
    mCookie.print();
    restoreLastStatus();
    RWGuard::getInstance()->reflesh();
    // now start to dump
    for(int i = 0; i < mCookie.size(); i++) {
        const char* name;
        auto dvmDex = mCookie.getCookieAt(i, name, mIgnoreCase);
        if (dvmDex == nullptr) {
            continue;
        }
        auto sig = mCookie.getDvmMagic(dvmDex);
        auto dumpIndex = mInfo.getCookieIndex(name, sig);
        if (dumpIndex == -1) {
            // no recorded in config.json???
            FLOGE("unable to find cookie config %s", name);
            dumpIndex = -i;
        } else {
            // turn status from wait into unpack
            if (mInfo.getCookieStatus(dumpIndex) == UnpackInfo::Status::Wait) {
                FLOGI("------------Dumping dex file %i %s---------------", dumpIndex, name);
                mInfo.setCookieStatus(dumpIndex, UnpackInfo::Status::Unpack);
                mInfo.saveConfigFile();
            } else {
                FLOGE("------------Skipping dex file %i %s-------------", dumpIndex, name);
                continue;
            }
        }

        std::stringstream ss;
        ss << mRoot << "/" << dumpIndex;
        std::string dumpFile = ss.str();
//        mkdir(dumpRoot.c_str(), 0700);


        FLOGI("=================Rebuinding dex file=================");
        DexDumper dumper(mEnv, dvmDex, mUpkObj);
        dumper.rebuild();
        FLOGI("===============Rebuinding dex file End================");

        auto fd = myfopen(dumpFile.c_str(), "w+");
        myfwrite(dumper.mRebuilded.c_str(), 1, dumper.mRebuilded.length(), fd);
        myfflush(fd);
        myfclose(fd);

        FLOGI("=================== Fix odex instruction==============");
        DexFixer fixer((u1 *) dumper.mRebuilded.c_str(), dumper.mRebuilded.length());
        fixer.fixAll();
        FLOGI("===================== odex fix end ===================");
        // generate
        fd = myfopen(dumpFile.c_str(), "w+");
        myfwrite(dumper.mRebuilded.c_str(), 1, dumper.mRebuilded.length(), fd);
        myfflush(fd);
        myfclose(fd);

        mInfo.setCookieStatus(dumpIndex, UnpackInfo::Status::Success);
        mInfo.saveConfigFile();
        FLOGI("===================== dump dex file end %i %s", dumpIndex, name);
    }



    FLOGD("======================== Dump end ==========================");

    return false;
}

bool Fupk::restoreLastStatus() {
    // loading unpack information(avoid re unpack if crash)
    FLOGD("Loading configure file");
    if (!mInfo.loadConfigFile()) {
        // may at the first time
        FLOGE("unable to load configure file");
    }
    for(int i = 0; i < mCookie.size(); i++) {
        const char* name;
        auto dvmDex = mCookie.getCookieAt(i, name, mIgnoreCase);
        if (dvmDex == nullptr)
            continue;
        auto sig = mCookie.getDvmMagic(dvmDex);
        mInfo.addCookie(name, sig);
    }
    // turn status
    for(int i = 0; i < mInfo.getCookiesCount(); i++) {
        // the last time, the program is crash when unpacking at i(dx),
        // just ignore i(dx) at this time
        if (mInfo.getCookieStatus(i) == UnpackInfo::Status::Unpack) {
            mInfo.setCookieStatus(i, UnpackInfo::Status::Fail);
        }
    }
    // if there are no more cookie to wait for unpack, just turn all failure
    // cookie into wait to try unpack at this time.
    bool retry = true;
    for(int i = 0; i < mInfo.getCookiesCount(); i++) {
        if (mInfo.getCookieStatus(i) == UnpackInfo::Status::Wait) {
            retry = false;
            break;
        }
    }
    if (retry) {
        for(int i = 0; i < mInfo.getCookiesCount(); i++) {
            if (mInfo.getCookieStatus(i) == UnpackInfo::Status::Fail) {
                mInfo.setCookieStatus(i, UnpackInfo::Status::Wait);
            }
        }
    }
    // the status may changed, just save config file again
    mInfo.saveConfigFile();
    return true;
}







