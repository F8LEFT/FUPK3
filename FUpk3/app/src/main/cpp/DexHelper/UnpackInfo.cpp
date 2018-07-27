//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "UnpackInfo.h"
#include "AndroidDef.h"

#include <fstream>
#include <sstream>
/* Config json type
 * {"name":"cookiename", "signature":"sha1signature", "status":"Success|Fail|Unpack|Wait", "index":"1|2|3..."}
 * */

UnpackInfo::UnpackInfo(std::string path) {
    mConfigPath = path;
}

bool UnpackInfo::loadConfigFile() {
    std::ifstream iFile(mConfigPath);
    if (!iFile.is_open()) {
        FLOGE("Unable to open Config File");
        return false;
    }
    Json::CharReaderBuilder reader;
    std::string errs;

    auto succ = Json::parseFromStream(reader, iFile, &mConfig, &errs);
    iFile.close();
    if (!succ) {
        FLOGE("loading config error %s", errs.c_str());
    }
    return succ;
}

bool UnpackInfo::saveConfigFile() {
    std::ofstream oFile(mConfigPath, std::ios::trunc);
    if (!oFile.is_open()) {
        FLOGE("Unable to save Config File");
        return false;
    }
    oFile << mConfig.toStyledString();
    oFile.close();
    return true;
}

bool UnpackInfo::addCookie(const std::string &cookieName,
                           const std::string &signature) {
    if (getCookieIndex(cookieName, signature) != -1) {
        return false;
    }
    Json::Value cookie;
    cookie["name"] = cookieName;
    cookie["signature"] = signature;
    cookie["status"] = Status::Wait;
    cookie["index"] = getCookiesCount();

    mConfig.append(cookie);
    return true;
}

int UnpackInfo::getCookiesCount() {
    return mConfig.size();
}

int UnpackInfo::getCookieIndex(const std::string &cookieName, const std::string &signature) {
    auto size = mConfig.size();
    for(auto i = 0; i < size; i++) {
        Json::Value empty;
        auto cookie = mConfig.get(i, empty);
        if (cookie.empty()) {
            continue;
        }
        if (cookie["name"].asString() == cookieName
            && cookie["signature"].asString() == signature) {
            return cookie["index"].asInt();
        }
    }
    return -1;
}

UnpackInfo::Status UnpackInfo::getCookieStatus(int cookieIdx) {
    Json::Value empty;
    auto cookie = mConfig.get(cookieIdx, empty);
    if (cookie.empty()) {
        return Status::Error;
    }
    return (UnpackInfo::Status)cookie["status"].asInt();
}

bool UnpackInfo::setCookieStatus(int cookieIdx, UnpackInfo::Status status) {
    if (mConfig.isValidIndex(cookieIdx)) {
        mConfig[(Json::ArrayIndex) cookieIdx]["status"] = status;
        return true;
    }
    return false;
}
