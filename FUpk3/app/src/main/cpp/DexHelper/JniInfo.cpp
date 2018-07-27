//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/4/18.
//                   Copyright (c) 2017. All rights reserved.
//===--------------------------------------------------------------------------
//
//===----------------------------------------------------------------------===//

#include "JniInfo.h"
#include <sys/system_properties.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <asm/mman.h>
#include <sys/mman.h>

#include "AndroidDef.h"

std::string JniInfo::jstrToCstr(JNIEnv *env, jstring jstr) {
    if(jstr == nullptr) {
        return std::move(std::string());
    }
    jboolean isCopy;
    const char* str = env->GetStringUTFChars(jstr, &isCopy);
    std::string rel = str;
    env->ReleaseStringUTFChars(jstr, str);
    return std::move(rel);
}

void JniInfo::logProcessMap() {
    FLOGD("============loging process map data========");
    auto fd = fopen("/proc/self/maps", "r");
    if (fd != nullptr) {
        char buf[512];
        while (fgets(buf, 512, fd) != nullptr) {
            FLOGD("%s", buf);
        }
        fclose(fd);
    }
    FLOGD("==============log map end ================");
}

//=========------------------------------------------------------
// expended jni bridge
#define CALL_VIRTUAL(_ctype, _jname, _retfail)                              \
     _ctype JniInfo::Call##_jname##Method(JNIEnv* env, jobject obj,         \
        const char *name, const char *sig, ...)                             \
    {                                                                       \
         va_list  args;                                                     \
         va_start(args, sig);                                               \
         VarArgs vargs(args);                                               \
         va_end(args);                                                      \
         return Call##_jname##MethodV(env, obj, name, sig, vargs.vaargs()); \
    }                                                                       \
     _ctype JniInfo::Call##_jname##MethodV(JNIEnv* env, jobject obj,        \
        const char *name, const char *sig, va_list args)                    \
    {                                                                       \
        assert(name != nullptr && sig != nullptr);                          \
        if(obj == nullptr) {                                                \
            return _retfail;                                                \
        }                                                                   \
        jclass clazz = env->GetObjectClass(obj);                            \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        jmethodID methodId = env->GetMethodID(clazz, name, sig);            \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->Call##_jname##MethodV(obj, methodId, args);             \
    }                                                                       \
     _ctype JniInfo::Call##_jname##MethodA(JNIEnv* env, jobject obj,        \
        const char *name, const char *sig, jvalue* args)                    \
    {                                                                       \
        assert(name != nullptr && sig != nullptr);                          \
        if(obj == nullptr) {                                                \
            return _retfail;                                                \
        }                                                                   \
        jclass clazz = env->GetObjectClass(obj);                            \
        jmethodID methodId = env->GetMethodID(clazz, name, sig);            \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->Call##_jname##MethodA(obj, methodId, args);             \
    }

CALL_VIRTUAL(jobject, Object, nullptr)
CALL_VIRTUAL(jboolean, Boolean, 0)
CALL_VIRTUAL(jbyte, Byte, 0)
CALL_VIRTUAL(jchar, Char, 0)
CALL_VIRTUAL(jshort, Short, 0)
CALL_VIRTUAL(jint, Int, 0)
CALL_VIRTUAL(jlong, Long, 0)
CALL_VIRTUAL(jfloat, Float, 0.0f)
CALL_VIRTUAL(jdouble, Double, 0.0)
CALL_VIRTUAL(void, Void, )

#define CALL_NONVIRTUAL(_ctype, _jname, _retfail)                              \
     _ctype JniInfo::CallNonvirtual##_jname##Method(JNIEnv* env, jobject obj,  \
        const char* classSig, const char *name, const char *sig, ...)       \
    {                                                                       \
         va_list  args;                                                     \
         va_start(args, sig);                                               \
         VarArgs vargs(args);                                               \
         va_end(args);                                                      \
         return CallNonvirtual##_jname##MethodV(env, obj, classSig, name, sig, vargs.vaargs()); \
    }                                                                       \
     _ctype JniInfo::CallNonvirtual##_jname##MethodV(JNIEnv* env, jobject obj, \
        const char* classSig, const char *name, const char *sig, va_list args) \
    {                                                                       \
        assert(classSig != nullptr && name != nullptr && sig != nullptr);   \
        if(obj == nullptr) {                                                \
            return _retfail;                                                \
        }                                                                   \
        jclass clazz = env->FindClass(classSig);                            \
        if(clazz == nullptr) {                                              \
            FLOGE("unable to find clazz %s", classSig);                       \
            return _retfail;                                                \
        }                                                                   \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        jmethodID methodId = env->GetMethodID(clazz, name, sig);            \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->CallNonvirtual##_jname##MethodV(obj, clazz, methodId, args);  \
    }                                                                       \
     _ctype JniInfo::CallNonvirtual##_jname##MethodA(JNIEnv* env, jobject obj,  \
        const char* classSig, const char *name, const char *sig, jvalue* args)  \
    {                                                                       \
        assert(classSig != nullptr && name != nullptr && sig != nullptr);   \
        if(obj == nullptr) {                                                \
            return _retfail;                                                \
        }                                                                   \
        jclass clazz = env->FindClass(classSig);                            \
        if(clazz == nullptr) {                                              \
            FLOGE("unable to find clazz %s", classSig);                       \
            return _retfail;                                                \
        }                                                                   \
        jmethodID methodId = env->GetMethodID(clazz, name, sig);            \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->CallNonvirtual##_jname##MethodA(obj, clazz, methodId, args); \
    }

CALL_NONVIRTUAL(jobject, Object, nullptr)
CALL_NONVIRTUAL(jboolean, Boolean, 0)
CALL_NONVIRTUAL(jbyte, Byte, 0)
CALL_NONVIRTUAL(jchar, Char, 0)
CALL_NONVIRTUAL(jshort, Short, 0)
CALL_NONVIRTUAL(jint, Int, 0)
CALL_NONVIRTUAL(jlong, Long, 0)
CALL_NONVIRTUAL(jfloat, Float, 0.0f)
CALL_NONVIRTUAL(jdouble, Double, 0.0)
CALL_NONVIRTUAL(void, Void, )

#define CALL_STATIC(_ctype, _jname, _retfail)                               \
     _ctype JniInfo::CallStatic##_jname##Method(JNIEnv* env,                \
        const char* classSig, const char *name, const char *sig, ...)       \
    {                                                                       \
         va_list  args;                                                     \
         va_start(args, sig);                                               \
         VarArgs vargs(args);                                               \
         va_end(args);                                                      \
         return CallStatic##_jname##MethodV(env, classSig, name, sig, vargs.vaargs()); \
    }                                                                       \
     _ctype JniInfo::CallStatic##_jname##MethodV(JNIEnv* env,               \
        const char* classSig, const char *name, const char *sig, va_list args) \
    {                                                                       \
        assert(classSig != nullptr && name != nullptr && sig != nullptr);   \
        jclass clazz = env->FindClass(classSig);                            \
        if(clazz == nullptr) {                                              \
            FLOGE("unable to find clazz %s", classSig);                       \
            return _retfail;                                                \
        }                                                                   \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        jmethodID methodId = env->GetStaticMethodID(clazz, name, sig);      \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->CallStatic##_jname##MethodV(clazz, methodId, args);     \
    }                                                                       \
     _ctype JniInfo::CallStatic##_jname##MethodA(JNIEnv* env,               \
        const char* classSig, const char *name, const char *sig, jvalue* args)  \
    {                                                                       \
        assert(classSig != nullptr && name != nullptr && sig != nullptr);   \
        jclass clazz = env->FindClass(classSig);                            \
        if(clazz == nullptr) {                                              \
            FLOGE("unable to find clazz %s", classSig);                       \
            return _retfail;                                                \
        }                                                                   \
        jmethodID methodId = env->GetStaticMethodID(clazz, name, sig);      \
        AutoJniRefRelease _autoRel = AutoJniRefRelease(env, clazz);         \
        if(methodId == nullptr) {                                           \
            FLOGE("unable to call method %s %s", name, sig);                  \
            env->ExceptionClear();                                          \
            return _retfail;                                                \
        }                                                                   \
        return env->CallStatic##_jname##MethodA(clazz, methodId, args);     \
    }

CALL_STATIC(jobject, Object, nullptr)
CALL_STATIC(jboolean, Boolean, 0)
CALL_STATIC(jbyte, Byte, 0)
CALL_STATIC(jchar, Char, 0)
CALL_STATIC(jshort, Short, 0)
CALL_STATIC(jint, Int, 0)
CALL_STATIC(jlong, Long, 0)
CALL_STATIC(jfloat, Float, 0.0f)
CALL_STATIC(jdouble, Double, 0.0)
CALL_STATIC(void, Void, )

#define GET_FIELD(_ctype, _jname, _retfail)                                   \
    _ctype JniInfo::Get##_jname##Field(JNIEnv* env, jobject obj,              \
        const char* name, const char* sig)                                    \
        {                                                                     \
            assert(name != nullptr && sig != nullptr);                        \
            if(obj == nullptr) {                                              \
                return _retfail;                                              \
            }                                                                 \
            jclass clazz = env->GetObjectClass(obj);                          \
            AutoJniRefRelease _autoRel1 = AutoJniRefRelease(env, clazz);      \
            jfieldID fieldId = env->GetFieldID(clazz, name, sig);             \
            if(fieldId == nullptr) {                                          \
                FLOGE("unable to get field %s %s", name, sig);                  \
                env->ExceptionClear();                                        \
                return _retfail;                                              \
            }                                                                 \
            return env->Get##_jname##Field(obj, fieldId);                     \
        }                                                                     \
    _ctype JniInfo::GetStatic##_jname##Field(JNIEnv*env, const char* classSig,\
        const char* name, const char* sig)                                    \
        {                                                                     \
            assert(classSig != nullptr && name != nullptr && sig != nullptr); \
            jclass clazz = env->FindClass(classSig);                          \
            if(clazz == nullptr) {                                            \
                FLOGE("Unable to find class %s", classSig);                     \
                return _retfail;                                              \
            }                                                                 \
            AutoJniRefRelease _autoRel1 = AutoJniRefRelease(env, clazz);      \
            jfieldID fieldId = env->GetStaticFieldID(clazz, name, sig);       \
            if(fieldId == nullptr) {                                          \
                FLOGE("unable to get field %s %s", name, sig);                  \
                env->ExceptionClear();                                        \
                return _retfail;                                              \
            }                                                                 \
            return env->GetStatic##_jname##Field(clazz, fieldId);             \
        }

GET_FIELD(jobject, Object, nullptr)
GET_FIELD(jboolean, Boolean, 0)
GET_FIELD(jbyte, Byte, 0)
GET_FIELD(jchar, Char, 0)
GET_FIELD(jshort, Short, 0)
GET_FIELD(jint, Int, 0)
GET_FIELD(jlong, Long, 0)
GET_FIELD(jfloat, Float, 0.0f)
GET_FIELD(jdouble, Double, 0.0)

#define SET_FIELD(_ctype, _jname)                                             \
    bool JniInfo::Set##_jname##Field(JNIEnv* env, jobject obj,                \
        const char* name, const char* sig, _ctype val)                        \
        {                                                                     \
            assert(name != nullptr && sig != nullptr);                        \
            if(obj == nullptr) {                                              \
                return false;                                                 \
            }                                                                 \
            jclass clazz = env->GetObjectClass(obj);                          \
            AutoJniRefRelease _autoRel1 = AutoJniRefRelease(env, clazz);      \
            jfieldID fieldId = env->GetFieldID(clazz, name, sig);             \
            if(fieldId == nullptr) {                                          \
                FLOGE("unable to set field %s %s", name, sig);                  \
                env->ExceptionClear();                                        \
                return false;                                                 \
            }                                                                 \
            env->Set##_jname##Field(obj, fieldId, val);                       \
            return true;                                                      \
        }                                                                     \
    bool JniInfo::SetStatic##_jname##Field(JNIEnv*env, const char* classSig,  \
        const char* name, const char* sig, _ctype val)                        \
        {                                                                     \
            assert(classSig != nullptr && name != nullptr && sig != nullptr); \
            jclass clazz = env->FindClass(classSig);                          \
            if(clazz == nullptr) {                                            \
                FLOGE("Unable to find class %s", classSig);                     \
                return false;                                                 \
            }                                                                 \
            AutoJniRefRelease _autoRel1 = AutoJniRefRelease(env, clazz);      \
            jfieldID fieldId = env->GetStaticFieldID(clazz, name, sig);       \
            if(fieldId == nullptr) {                                          \
                FLOGE("unable to set field %s %s", name, sig);                  \
                env->ExceptionClear();                                        \
                return false;                                                 \
            }                                                                 \
            env->SetStatic##_jname##Field(clazz, fieldId, val);               \
            return true;                                                      \
        }

SET_FIELD(jobject, Object)
SET_FIELD(jboolean, Boolean)
SET_FIELD(jbyte, Byte)
SET_FIELD(jchar, Char)
SET_FIELD(jshort, Short)
SET_FIELD(jint, Int)
SET_FIELD(jlong, Long)
SET_FIELD(jfloat, Float)
SET_FIELD(jdouble, Double)


