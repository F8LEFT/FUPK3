//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/6/8.
//                   Copyright (c) 2017. All rights reserved.
//===--------------------------------------------------------------------------
//
//===----------------------------------------------------------------------===//

#ifndef FSHELL_MINANDROIDDEF_H_H
#define FSHELL_MINANDROIDDEF_H_H

#include <android/log.h>

#ifndef FLOG_TAG
//#define FORCE_LOG

// 补充一些Android的定义
#if !defined(NDEBUG) || defined(FORCE_LOG)
#define TOSTR(fmt) #fmt
#define FLFMT TOSTR([%s:%d])

#ifdef __ANDROID__
#define FLOG_TAG "F8LEFT"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, FLOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, FLOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, FLOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, FLOG_TAG, __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, FLOG_TAG, __VA_ARGS__)

#define FLOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, FLOG_TAG, FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, FLOG_TAG, FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGW(fmt, ...) __android_log_print(ANDROID_LOG_WARN, FLOG_TAG, FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO, FLOG_TAG, FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGV(fmt, ...) __android_log_print(ANDROID_LOG_VERBOSE, FLOG_TAG, FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define LOGE printf
#define LOGD printf
#define LOGW printf
#define LOGI printf
#define LOGV printf

#define FLOGE(fmt, ...) printf(FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGD(fmt, ...) printf(FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGW(fmt, ...) printf(FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGI(fmt, ...) printf(FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define FLOGV(fmt, ...) printf(FLFMT fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#endif    // ANDROID

#else
// DO NOT LOG MESSAGE IN RELEASE VERSION
#define LOGE(...)
#define LOGD(...)
#define LOGW(...)
#define LOGI(...)
#define LOGV(...)

#define FLOGE(...)
#define FLOGD(...)
#define FLOGW(...)
#define FLOGI(...)
#define FLOGV(...)
#endif


#define LOG_MT_EXCEPTION(msg) __android_log_print(ANDROID_LOG_ERROR, "MTProtect", "MTProtect catch exception: %s", msg)

#define ALOGE LOGE
#define ALOGV LOGV
#define ALOGW LOGW
#define ALOGD LOGD
#define ALOGI LOGI

#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif

#ifndef PAGESIZE
#define PAGESIZE PAGE_SIZE
#endif

#ifndef PAGE_MASK
#define PAGE_MASK (~(PAGE_SIZE-1))
#endif

#define PAGE_START(x)  ((x) & PAGE_MASK)
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)
#define PAGE_END(x)    PAGE_START((x) + (PAGE_SIZE-1))

#define PACKED(x) __attribute__ ((__aligned__(x), __packed__))

// DISALLOW_COPY_AND_ASSIGN disallows the copy and operator= functions.
// It goes in the private: declarations in a class.
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)
#endif

#endif //FSHELL_MINANDROIDDEF_H_H
