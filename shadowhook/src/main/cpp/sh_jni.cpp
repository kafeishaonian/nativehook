//
// Created by MOMO on 2024/4/18.
//

#include <errno.h>
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>

#include "sh_log.h"
#include "shadowhook.h"

#define SH_JNI_VERSION JNI_VERSION_1_6
#define SH_JNI_CLASS_NAME "com/example/shadowhook/ShadowHook"

static jstring sh_jni_get_version(JNIEnv *env, jclass interface) {
    (void) interface;
    return env->NewStringUTF(shadowhook_get_version());
}

static jint sh_jni_init(JNIEnv *env, jclass interface, jint mode, jboolean debuggable) {
    return shadowhook_init(0 == mode ? SHADOWHOOK_MODE_SHARED : SHADOWHOOK_MODE_UNIQUE, (bool) debuggable);
}

static jint sh_jni_get_init_errno(JNIEnv *env, jclass interface) {
    return shadowhook_get_init_errno();
}

static jint sh_jni_get_mode(JNIEnv *env, jclass interface) {
    return SHADOWHOOK_MODE_SHARED == shadowhook_get_mode() ? 0 : 1;
}

static jboolean sh_jni_get_debuggable(JNIEnv *env, jclass interface) {
    return shadowhook_get_debuggable();
}

static void sh_jni_set_debuggable(JNIEnv *env, jclass interface, jboolean debuggable) {
    shadowhook_set_debuggable((bool)debuggable);
}

static jboolean sh_jni_get_recordable(JNIEnv *env, jclass interface) {
    return shadowhook_get_recordable();
}

static void sh_jni_set_recordable(JNIEnv *env, jclass interface, jboolean recordable) {
    shadowhook_set_recordable((bool)recordable);
}

static jstring sh_jni_to_errmsg(JNIEnv *env, jclass interface, jint error_number) {
    return env->NewStringUTF(shadowhook_to_errmsg(error_number));
}

static jstring sh_jni_get_records(JNIEnv *env, jclass interface, jint item_flags) {

    char *str = shadowhook_get_records((uint32_t)item_flags);
    if (NULL == str) return NULL;

    jstring jstr = env->NewStringUTF(str);
    free(str);
    return jstr;
}

static jstring sh_jni_get_arch(JNIEnv *env, jclass interface) {

#if defined(__arm__)
    char *arch = "arm";
#elif defined(__aarch64__)
    char *arch = "arm64";
#else
    char *arch = "unsupported";
#endif

    return env->NewStringUTF(arch);
}


static JNINativeMethod methods[] = {
        {"nativeGetVersion", "()Ljava/lang/String;", (void *)sh_jni_get_version},
        {"nativeInit", "(IZ)I", (void *)sh_jni_init},
        {"nativeGetInitErrno", "()I", (void *)sh_jni_get_init_errno},
        {"nativeGetMode", "()I", (void *)sh_jni_get_mode},
        {"nativeGetDebuggable", "()Z", (void *)sh_jni_get_debuggable},
        {"nativeSetDebuggable", "(Z)V", (void *)sh_jni_set_debuggable},
        {"nativeGetRecordable", "()Z", (void *)sh_jni_get_recordable},
        {"nativeSetRecordable", "(Z)V", (void *)sh_jni_set_recordable},
        {"nativeToErrmsg", "(I)Ljava/lang/String;", (void *)sh_jni_to_errmsg},
        {"nativeGetRecords", "(I)Ljava/lang/String;", (void *)sh_jni_get_records},
        {"nativeGetArch", "()Ljava/lang/String;", (void *)sh_jni_get_arch}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (__predict_false(NULL == vm)) {
        return JNI_ERR;
    }

    JNIEnv *env;
    if (__predict_false(vm->GetEnv((void **) &env, SH_JNI_VERSION) != JNI_OK)) {
        return JNI_ERR;
    }

    if (__predict_false(NULL == env)) {
        return JNI_ERR;
    }

    jclass cls;
    if (__predict_false(NULL == (cls = env->FindClass(SH_JNI_CLASS_NAME)))) {
        return JNI_ERR;
    }
    if (__predict_false(0 != env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0])))) {
        return JNI_ERR;
    }

    return SH_JNI_VERSION;

}