//
// Created by MOMO on 2023/12/21.
//

#include "bh_log.h"

android_LogPriority bh_log_priority = ANDROID_LOG_SILENT;

bool bh_log_get_debug(void) {
    return bh_log_priority <= ANDROID_LOG_INFO;
}

void bh_log_set_debug(bool debug) {
    if (__predict_false(debug)) {
        bh_log_priority = ANDROID_LOG_INFO;
    } else {
        bh_log_priority = ANDROID_LOG_SILENT;
    }
}
