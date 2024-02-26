//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_SH_LOG_H
#define NATIVEHOOK_SH_LOG_H

#include <android/log.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

extern android_LogPriority sh_log_priority;

#define SH_LOG_TAG "shadowhook_tag"

#ifdef SH_CONFIG_DEBUG
#define SH_LOG_DEBUG(fmt, ...)                                               \
  do {                                                                       \
    if (sh_log_priority <= ANDROID_LOG_INFO)                                 \
      __android_log_print(ANDROID_LOG_INFO, SH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)
#else
#define SH_LOG_DEBUG(fmt, ...)
#endif

#define SH_LOG_INFO(fmt, ...)                                                \
  do {                                                                       \
    if (__predict_false(sh_log_priority <= ANDROID_LOG_INFO))                \
      __android_log_print(ANDROID_LOG_INFO, SH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)
#define SH_LOG_WARN(fmt, ...)                                                \
  do {                                                                       \
    if (__predict_false(sh_log_priority <= ANDROID_LOG_WARN))                \
      __android_log_print(ANDROID_LOG_WARN, SH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)
#define SH_LOG_ERROR(fmt, ...)                                                \
  do {                                                                        \
    if (__predict_false(sh_log_priority <= ANDROID_LOG_ERROR))                \
      __android_log_print(ANDROID_LOG_ERROR, SH_LOG_TAG, fmt, ##__VA_ARGS__); \
  } while (0)
#define SH_LOG_ALWAYS_SHOW(fmt, ...) __android_log_print(ANDROID_LOG_WARN, SH_LOG_TAG, fmt, ##__VA_ARGS__)

bool sh_log_get_debuggable(void);
void sh_log_set_debuggable(bool debuggable);

#endif //NATIVEHOOK_SH_LOG_H
