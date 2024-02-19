//
// Created by MOMO on 2024/1/31.
//

#include <android/log.h>
#include <fcntl.h>
#include <jni.h>
#include <stdlib.h>
#include <unistd.h>

#define HOOKEE_TAG            "plt_hook_tag"
#define LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, HOOKEE_TAG, fmt, ##__VA_ARGS__)

static void hookee_test(JNIEnv *env, jclass thiz) {
    (void)env, (void)thiz;

    LOG("libhookee.so PRE open()");
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) close(fd);
    LOG("libhookee.so POST open()");
}


static JNINativeMethod methods[] = {
        {"nativeTest", "()V", (void *)hookee_test}
};


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {

    if (NULL == vm) {
        return JNI_ERR;
    }
    JNIEnv *env;
    if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass cls;
    if (NULL == env) {
        return JNI_ERR;
    }

    if (NULL == (cls = env->FindClass("com/example/nativehook/NativeHookee"))) {
        return JNI_ERR;
    }

    if (env->RegisterNatives(cls, methods, sizeof(methods) / sizeof(methods[0])) != 0) {
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}