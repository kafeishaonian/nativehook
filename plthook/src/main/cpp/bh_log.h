//
// Created by MOMO on 2023/12/21.
//

#ifndef NATIVEHOOK_BH_LOG_H
#define NATIVEHOOK_BH_LOG_H

#pragma once
#include <android/log.h>
#include <stdarg.h>
#include <stdbool.h>

extern android_LogPriority bh_log_priority;

#define BH_LOG_TAG "plt_hook_tag"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"

#define BH_LOG_INFO(fmt, ...)                                                                       \
    do {                                                                                            \
       if(__predict_false(bh_log_priority <= ANDROID_LOG_INFO)) {                                   \
            __android_log_print(ANDROID_LOG_INFO, BH_LOG_TAG, fmt, ##__VA_ARGS__);                  \
       }                                                                                            \
    } while(0)

#define BH_LOG_WARN(fmt, ...)                                                                       \
  do {                                                                                              \
    if (__predict_false(bh_log_priority <= ANDROID_LOG_WARN)) {                                     \
        __android_log_print(ANDROID_LOG_WARN, BH_LOG_TAG, fmt, ##__VA_ARGS__);                        \
    }                                                                                               \
  } while (0)


#define BH_LOG_ERROR(fmt, ...)                                                \
  do {                                                                        \
    if (__predict_false(bh_log_priority <= ANDROID_LOG_ERROR))                \
      __android_log_print(ANDROID_LOG_ERROR, BH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)

#define BH_LOG_SHOW(fmt, ...)                                              \
  do {                                                                     \
    __android_log_print(ANDROID_LOG_WARN, BH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)

#pragma clang diagnostic pop

bool bh_log_get_debug(void);
void bh_log_set_debug(bool debug);

#endif //NATIVEHOOK_BH_LOG_H
