#include <jni.h>
#include <string>
#include "plthook.h"

#define BH_JNI_VERSION JNI_VERSION_1_6
#define BH_JNI_CLASS_NAME "com/example/plthook/PLTHook"

static jstring bh_jni_get_version(JNIEnv *env, jclass interface) {
    (void) interface;
    return env->NewStringUTF(plt_hook_get_version());
}

static jint bh_jni_init(JNIEnv *env, jclass interface, jint mode, jboolean debug) {
    (void) env;
    (void) interface;
    return plt_hook_init(mode, debug);
}

static jint bh_jni_add_ignore(JNIEnv *env, jclass interface, jstring caller_path_name) {
    (void) env;
    (void) interface;

    int r = PLT_HOOK_STATUS_CODE_IGNORE;
    if (!caller_path_name){
        return r;
    }

    const char *c_caller_path_name;
    if (NULL == (c_caller_path_name = env->GetStringUTFChars(caller_path_name, 0))) {
        goto clean;
    }

//    r = plt_hook_add_ignore(c_caller_path_name);

    clean:
    if (caller_path_name && c_caller_path_name) {
        env->ReleaseStringUTFChars(caller_path_name, c_caller_path_name);
    }
    return r;
}

static jint bh_jni_get_mode(JNIEnv *env, jclass interface) {
    (void) env;
    (void) interface;

//    return PLT_HOOK_MODE_AUTOMATIC == plt_hook_get_mode() ? 0 : 1;
    return 1;
}
static jboolean bh_jni_get_debug(JNIEnv *env, jclass interface) {
    (void) env;
    (void) interface;

//    return plt_hook_get_debug();
    return false;
}
static void bh_jni_set_debug(JNIEnv *env, jclass interface, jboolean debug) {
    (void) env;
    (void) interface;

//    plt_hook_set_debug(debug);
}

static jboolean bh_jni_get_recordable(JNIEnv *env, jclass interface) {
    (void) env;
    (void) interface;

//    return plt_hook_get_recordable();
    return false;
}

static void bh_jni_set_recordable(JNIEnv *env, jclass interface, jboolean recordable) {
    (void) env;
    (void) interface;

//    plt_hook_set_recordable(recordable);
}

static jstring bh_jni_get_records(JNIEnv *env, jclass interface, jint item_flags) {
    (void) interface;

//    char *str = plt_hook_get_records(item_flags);
    char *str;
    if (NULL == str) {
        return NULL;
    }

    jstring jstr = env->NewStringUTF(str);
    free(str);
    return jstr;
}

static jstring bh_jni_get_arch(JNIEnv *env, jclass interface) {
    (void) interface;

#if defined(__arm__)
    char *arch = "arm";
#elif defined(__aarch64__)
    char *arch = "arm64";
#elif defined(__i386__)
    char *arch = "x86";
#elif defined(__x86_64__)
    char *arch = "x86_64";
#else
  char *arch = "unsupported";
#endif
    return env->NewStringUTF(arch);
}

static JNINativeMethod methods[] = {
        {"nativeGetVersion", "()Ljava/lang/String;", (void *)bh_jni_get_version},
        {"nativeInit", "(IZ)I", (void *)bh_jni_init},
        {"nativeAddIgnore", "(Ljava/lang/String;)I", (void *)bh_jni_add_ignore},
        {"nativeGetMode", "()I", (void *)bh_jni_get_mode},
        {"nativeGetDebug", "()Z", (void *)bh_jni_get_debug},
        {"nativeSetDebug", "(Z)V", (void *)bh_jni_set_debug},
        {"nativeGetRecordable", "()Z", (void *)bh_jni_get_recordable},
        {"nativeSetRecordable", "(Z)V", (void *)bh_jni_set_recordable},
        {"nativeGetRecords", "(I)Ljava/lang/String;", (void *)bh_jni_get_records},
        {"nativeGetArch", "()Ljava/lang/String;", (void *)bh_jni_get_arch}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)reserved;

    if (__predict_false(NULL == vm)) {
        return JNI_ERR;
    }

    JNIEnv *env;
    if (__predict_false(vm->GetEnv((void **) &env, BH_JNI_VERSION) != JNI_OK)) {
        return JNI_ERR;
    }

    if (__predict_false(NULL == env)) {
        return JNI_ERR;
    }

    jclass cls;
    if (__predict_false(NULL == (cls = env->FindClass(BH_JNI_CLASS_NAME)))) {
        return JNI_ERR;
    }
    if (__predict_false(0 != env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0])))) {
        return JNI_ERR;
    }

    return BH_JNI_VERSION;
}
