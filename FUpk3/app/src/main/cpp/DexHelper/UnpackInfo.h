//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2018/4/5.
//                   Copyright (c) 2018. All rights reserved.
//===----------------------------------------------------------------------===//
// I will use a json file to store all packed information
//===----------------------------------------------------------------------===//


#ifndef FUPK3_UNPACKINFO_H
#define FUPK3_UNPACKINFO_H

#include <json/json.h>
#include <string>
class UnpackInfo {
public:
    enum Status {
        Wait = 0,
        Unpack,
        Success,
        Fail,
        Error,
    };

    UnpackInfo(std::string path);

    // If I add/set a cookie, I must store config file at the same time
    // The unpack process may be crashed at any time.
    int getCookiesCount();
    bool addCookie(const std::string &cookieName, const std::string &signature);

    int getCookieIndex(const std::string &cookieName,
                       const std::string &signature);
    Status getCookieStatus(int cookieIdx);
    bool setCookieStatus(int cookieIdx, UnpackInfo::Status status);


    bool loadConfigFile();
    bool saveConfigFile();
private:
    std::string mConfigPath;
    Json::Value mConfig;
};


#endif //FUPK3_UNPACKINFO_H
