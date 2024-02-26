//
// Created by MOMO on 2024/2/21.
//

#include "sh_log.h"

#include <android/log.h>
#include <stdbool.h>

#include "sh_config.h"

android_LogPriority sh_log_priority =
#ifdef SH_CONFIG_DEBUG
        ANDROID_LOG_INFO
#else
        ANDROID_LOG_SILENT
#endif
;

bool sh_log_get_debuggable(void) {
    return sh_log_priority <= ANDROID_LOG_INFO;
}

void sh_log_set_debuggable(bool debuggable) {
#ifdef SH_CONFIG_DEBUG
    (void)debuggable;
  sh_log_priority = ANDROID_LOG_INFO;
#else
    if (__predict_false(debuggable))
        sh_log_priority = ANDROID_LOG_INFO;
    else
        sh_log_priority = ANDROID_LOG_SILENT;
#endif
}