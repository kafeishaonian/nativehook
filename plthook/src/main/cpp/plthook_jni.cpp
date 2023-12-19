#include <jni.h>
#include <string>

#define BH_JNI_VERSION JNI_VERSION_1_6
#define BH_JNI_CLASS_NAME "com/example/plthook/PLTHook"

static jstring bh_jni_get_version(JNIEnv *env, jclass interface) {
    return NULL;
}

static jint bh_jni_init(JNIEnv *env, jclass interface, jint mode, jboolean debug) {
    return 1;
}

static jint bh_jni_add_ignore(JNIEnv *env, jclass interface, jstring caller_path_name) {
    return 1;
}

static jint bh_jni_get_mode(JNIEnv *env, jclass interface) {
    return 1;
}
static jboolean bh_jni_get_debug(JNIEnv *env, jclass interface) {
    return JNI_TRUE;
}
static void bh_jni_set_debug(JNIEnv *env, jclass interface, jboolean debug) {

}

static jboolean bh_jni_get_recordable(JNIEnv *env, jclass interface) {
    return JNI_TRUE;
}

static void bh_jni_set_recordable(JNIEnv *env, jclass interface, jboolean recordable) {

}

static jstring bh_jni_get_records(JNIEnv *env, jclass interface, jint item_flags) {
    return NULL;
}

static jstring bh_jni_get_arch(JNIEnv *env, jclass interface) {
    return NULL;
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
