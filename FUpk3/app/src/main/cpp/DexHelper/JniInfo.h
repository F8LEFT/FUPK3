//===------------------------------------------------------------*- C++ -*-===//
//
//                     Created by F8LEFT on 2017/4/18.
//                   Copyright (c) 2017. All rights reserved.
//===--------------------------------------------------------------------------
//  JniInfo is used to record some process information
//===----------------------------------------------------------------------===//

#ifndef FSHELL_JNIINFO_H
#define FSHELL_JNIINFO_H

#include <string>
#include <jni.h>
#include <assert.h>
#include <vector>

#ifdef NDEBUG
#define ANTIDEBUG
#endif

class VarArgs;
/*
 * Java primitive types:
 * B - jbyte
 * C - jchar
 * D - jdouble
 * F - jfloat
 * I - jint
 * J - jlong
 * S - jshort
 * Z - jboolean (shown as true and false)
 * V - void
 *
 * Java reference types:
 * L - jobject
 * a - jarray
 * c - jclass
 * s - jstring
 * t - jthrowable
 *
 * JNI types:
 * b - jboolean (shown as JNI_TRUE and JNI_FALSE)
 * f - jfieldID
 * i - JNI error value (JNI_OK, JNI_ERR, JNI_EDETACHED, JNI_EVERSION)
 * m - jmethodID
 * p - void*
 * r - jint (for release mode arguments)
 * u - const char* (Modified UTF-8)
 * z - jsize (for lengths; use i if negative values are okay)
 * v - JavaVM*
 * w - jobjectRefType
 * E - JNIEnv*
 * . - no argument; just print "..." (used for varargs JNI calls)
 *
 */
union JniValueType {
    jarray a;
    jboolean b;
    jclass c;
    jfieldID f;
    jint i;
    jmethodID m;
    const void* p;  // Pointer.
    jint r;  // Release mode.
    jstring s;
    jthrowable t;
    const char* u;  // Modified UTF-8.
    JavaVM* v;
    jobjectRefType w;
    jsize z;
    jbyte B;
    jchar C;
    jdouble D;
    JNIEnv* E;
    jfloat F;
    jint I;
    jlong J;
    jobject L;
    jshort S;
    const void* V;  // void
    jboolean Z;
    const VarArgs* va;
};

/*
 * A structure containing all the information needed to validate varargs arguments.
 *
 * Note that actually getting the arguments from this structure mutates it so should only be done on
 * owned copies.
 */
struct VarArgs {
public:
    VarArgs(va_list var) : type_(kTypeVaList), cnt_(0) {
        va_copy(vargs_, var);
    }

    VarArgs(const jvalue* vals) : type_(kTypePtr), cnt_(0), ptr_(vals) {}

    ~VarArgs() {
        if (type_ == kTypeVaList) {
            va_end(vargs_);
        }
    }

    VarArgs(VarArgs&& other) {
        cnt_ = other.cnt_;
        type_ = other.type_;
        if (other.type_ == kTypeVaList) {
            va_copy(vargs_, other.vargs_);
        } else {
            ptr_ = other.ptr_;
        }
    }

    va_list& vaargs() {
        assert(type_ == kTypeVaList && "type not match");
        return vargs_;
    }
    const jvalue* vaptrs() {
        assert(type_ == kTypePtr && "type not match");
        return ptr_;
    }

    // This method is const because we need to ensure that one only uses the GetValue method on an
    // owned copy of the VarArgs. This is because getting the next argument from a va_list is a
    // mutating operation. Therefore we pass around these VarArgs with the 'const' qualifier and when
    // we want to use one we need to Clone() it.
    VarArgs Clone() const {
        if (type_ == kTypeVaList) {
            // const_cast needed to make sure the compiler is okay with va_copy, which (being a macro) is
            // messed up if the source argument is not the exact type 'va_list'.
            return VarArgs(cnt_, const_cast<VarArgs*>(this)->vargs_);
        } else {
            return VarArgs(cnt_, ptr_);
        }
    }

    JniValueType GetValue(char fmt) {
        JniValueType o;
        if (type_ == kTypeVaList) {
            switch (fmt) {
                case 'Z': o.Z = static_cast<jboolean>(va_arg(vargs_, jint)); break;
                case 'B': o.B = static_cast<jbyte>(va_arg(vargs_, jint)); break;
                case 'C': o.C = static_cast<jchar>(va_arg(vargs_, jint)); break;
                case 'S': o.S = static_cast<jshort>(va_arg(vargs_, jint)); break;
                case 'I': o.I = va_arg(vargs_, jint); break;
                case 'J': o.J = va_arg(vargs_, jlong); break;
                case 'F': o.F = static_cast<jfloat>(va_arg(vargs_, jdouble)); break;
                case 'D': o.D = va_arg(vargs_, jdouble); break;
                case 'L': o.L = va_arg(vargs_, jobject); break;
                default:
                    assert(false && "Unknown format");
                    break;
            }
        } else {
            jvalue v = ptr_[cnt_];
            cnt_++;
            switch (fmt) {
                case 'Z': o.Z = v.z; break;
                case 'B': o.B = v.b; break;
                case 'C': o.C = v.c; break;
                case 'S': o.S = v.s; break;
                case 'I': o.I = v.i; break;
                case 'J': o.J = v.j; break;
                case 'F': o.F = v.f; break;
                case 'D': o.D = v.d; break;
                case 'L': o.L = v.l; break;
                default:
                    assert(false && "Unkonwn format");
                    break;
            }
        }
        return o;
    }

private:
    VarArgs(uint32_t cnt, va_list var) : type_(kTypeVaList), cnt_(cnt) {
        va_copy(vargs_, var);
    }

    VarArgs(uint32_t cnt, const jvalue* vals) : type_(kTypePtr), cnt_(cnt), ptr_(vals) {}

    enum VarArgsType {
        kTypeVaList,
        kTypePtr,
    };

    VarArgsType type_;
    uint32_t cnt_;
    union {
        va_list vargs_;
        const jvalue* ptr_;
    };
};

struct AutoJniEnvRelease {
    AutoJniEnvRelease(JNIEnv* env) {
        assert(env != nullptr);
        m_env = env;
        m_env->PushLocalFrame(0x10);
    }
    ~AutoJniEnvRelease() {
        if(!framehasbeenPop) {
            m_env->PopLocalFrame(nullptr);
        }
    }
    jobject releaseFrame(jobject obj) {
        framehasbeenPop = true;
        return m_env->PopLocalFrame(obj);
    }

    JNIEnv* m_env;
    bool framehasbeenPop = false;
};

struct AutoJniRefRelease {
    AutoJniRefRelease(JNIEnv*env, jobject obj) {
        assert(env != nullptr);
        assert(obj != nullptr);
        m_env = env;
        m_obj = obj;
    }
    ~AutoJniRefRelease() {
        m_env->DeleteLocalRef(m_obj);
    }
    JNIEnv* m_env;
    jobject m_obj;
};

#define  SOINFO_NAME_LEN            128
struct soinfo {
    const char name[SOINFO_NAME_LEN];
    void *phdr;
    int phnum;
    unsigned entry;
    unsigned base;
    unsigned size;
    // buddy-allocator index, negative for prelinked libraries
    int ba_index;
    unsigned *dynamic;
    unsigned wrprotect_start;
    unsigned wrprotect_end;
    soinfo *next;
    unsigned flags;
};


#define BUILD_VERSION_INT_1_MIN		1
#define BUILD_VERSION_INT_1_MAX		4
#define BUILD_VERSION_INT_2_MIN		5
#define BUILD_VERSION_INT_2_1		7
#define BUILD_VERSION_INT_2_2		8
#define BUILD_VERSION_INT_2_3_MIN	9
#define BUILD_VERSION_INT_2_MAX		10
#define BUILD_VERSION_INT_3_MIN		11
#define BUILD_VERSION_INT_3_MAX		13
#define BUILD_VERSION_INT_4_MIN		14


#define  DVM_VERSION_UNKNOW     0
#define  DVM_VERSION_V1X        1
#define  DVM_VERSION_V21        2
#define  DVM_VERSION_V22        3
#define  DVM_VERSION_V23        4
#define  DVM_VERSION_V4X        5


#ifndef PROP_VALUE_MAX
#define PROP_VALUE_MAX 	            92
#endif

namespace JniInfo {

    std::string jstrToCstr(JNIEnv*env, jstring jstr);

    void logProcessMap();
//    static jobject CallObjectMethod(JNIEnv* env, jobject obj, const char* name, const char* sig, ...);
#define JniInfo_CALLTYPE(_jtype, _jname)                    \
    _jtype Call##_jname##Method(JNIEnv* env, jobject obj,   \
        const char* name, const char* sig, ...);            \
    _jtype Call##_jname##MethodV(JNIEnv* env, jobject obj,  \
        const char* name, const char* sig, va_list args);   \
    _jtype Call##_jname##MethodA(JNIEnv* env, jobject obj,  \
        const char* name, const char* sig, jvalue* args);

    JniInfo_CALLTYPE(jobject, Object)
    JniInfo_CALLTYPE(jboolean, Boolean)
    JniInfo_CALLTYPE(jbyte, Byte)
    JniInfo_CALLTYPE(jchar, Char)
    JniInfo_CALLTYPE(jshort, Short)
    JniInfo_CALLTYPE(jint, Int)
    JniInfo_CALLTYPE(jlong, Long)
    JniInfo_CALLTYPE(jfloat, Float)
    JniInfo_CALLTYPE(jdouble, Double)
    JniInfo_CALLTYPE(void, Void)

#define JniInfo_CALL_NONVIRT_TYPE(_jtype, _jname)                               \
    _jtype CallNonvirtual##_jname##Method(JNIEnv* env, jobject obj,             \
        const char* classSig,const char* name, const char* sig, ...);           \
    _jtype CallNonvirtual##_jname##MethodV(JNIEnv* env, jobject obj,            \
        const char* classSig, const char* name, const char* sig, va_list args); \
    _jtype CallNonvirtual##_jname##MethodA(JNIEnv* env, jobject obj,            \
        const char* classSig, const char* name, const char* sig, jvalue* args);

    JniInfo_CALL_NONVIRT_TYPE(jobject, Object)
    JniInfo_CALL_NONVIRT_TYPE(jboolean, Boolean)
    JniInfo_CALL_NONVIRT_TYPE(jbyte, Byte)
    JniInfo_CALL_NONVIRT_TYPE(jchar, Char)
    JniInfo_CALL_NONVIRT_TYPE(jshort, Short)
    JniInfo_CALL_NONVIRT_TYPE(jint, Int)
    JniInfo_CALL_NONVIRT_TYPE(jlong, Long)
    JniInfo_CALL_NONVIRT_TYPE(jfloat, Float)
    JniInfo_CALL_NONVIRT_TYPE(jdouble, Double)
    JniInfo_CALL_NONVIRT_TYPE(void, Void)

#define JniInfo_CALL_STATIC_TYPE(_jtype, _jname)                                    \
    _jtype CallStatic##_jname##Method(JNIEnv* env,            \
        const char* classSig,const char* name, const char* sig, ...);           \
    _jtype CallStatic##_jname##MethodV(JNIEnv* env,         \
        const char* classSig, const char* name, const char* sig, va_list args); \
    _jtype CallStatic##_jname##MethodA(JNIEnv* env,            \
        const char* classSig, const char* name, const char* sig, jvalue* args);

    JniInfo_CALL_STATIC_TYPE(jobject, Object)
    JniInfo_CALL_STATIC_TYPE(jboolean, Boolean)
    JniInfo_CALL_STATIC_TYPE(jbyte, Byte)
    JniInfo_CALL_STATIC_TYPE(jchar, Char)
    JniInfo_CALL_STATIC_TYPE(jshort, Short)
    JniInfo_CALL_STATIC_TYPE(jint, Int)
    JniInfo_CALL_STATIC_TYPE(jlong, Long)
    JniInfo_CALL_STATIC_TYPE(jfloat, Float)
    JniInfo_CALL_STATIC_TYPE(jdouble, Double)
    JniInfo_CALL_STATIC_TYPE(void, Void)

#define JniInfo_GETFIELD(_jtype, _jname)    \
    _jtype Get##_jname##Field(JNIEnv* env, jobject obj, \
        const char* name, const char* sig);  \
    _jtype GetStatic##_jname##Field(JNIEnv*env, \
        const char* classSig, const char* name, const char* sig);

    JniInfo_GETFIELD(jobject, Object)
    JniInfo_GETFIELD(jboolean, Boolean)
    JniInfo_GETFIELD(jbyte, Byte)
    JniInfo_GETFIELD(jchar, Char)
    JniInfo_GETFIELD(jshort, Short)
    JniInfo_GETFIELD(jint, Int)
    JniInfo_GETFIELD(jlong, Long)
    JniInfo_GETFIELD(jfloat, Float)
    JniInfo_GETFIELD(jdouble, Double)

#define JniInfo_SETFIELD(_jtype, _jname)    \
    bool Set##_jname##Field(JNIEnv* env, jobject obj, \
        const char* name, const char* sig, _jtype val); \
    bool SetStatic##_jname##Field(JNIEnv* env, \
        const char* classSig, const char* name, const char* sig, _jtype val);

    JniInfo_SETFIELD(jobject, Object)
    JniInfo_SETFIELD(jboolean, Boolean)
    JniInfo_SETFIELD(jbyte, Byte)
    JniInfo_SETFIELD(jchar, Char)
    JniInfo_SETFIELD(jshort, Short)
    JniInfo_SETFIELD(jint, Int)
    JniInfo_SETFIELD(jlong, Long)
    JniInfo_SETFIELD(jfloat, Float)
    JniInfo_SETFIELD(jdouble, Double)
};


#endif //FSHELL_JNIINFO_H
