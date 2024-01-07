//
// Created by MOMO on 2024/1/5.
//

#include "bh_dl_monitor.h"

#include <android/api-level.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bh_const.h"
#include "bh_core.h"
#include "bh_linker.h"
#include "bh_log.h"
#include "bh_util.h"
#include "plthook.h"
#include "queue.h"

// clang-format off
/*
 * This solution is derived from ByteDance Raphael (https://github.com/bytedance/memory-leak-detector),
 * which designed and implemented by (alphabetical order):
 *
 * Tianzhou Shen (shentianzhou@bytedance.com)
 * Yonggang Sun (sunyonggang@bytedance.com)
 *
 * ===================================================================================================================================================================
 * API-LEVEL  ANDROID-VERSION  TARGET-LIB  TARGET-FUNCTION                               SOLUTION
 * ===================================================================================================================================================================
 * 16         4.1              all-ELF     dlopen                                        HOOK -> CALL(original-function)
 * 17         4.2              all-ELF     dlopen                                        HOOK -> CALL(original-function)
 * 18         4.3              all-ELF     dlopen                                        HOOK -> CALL(original-function)
 * 19         4.4              all-ELF     dlopen                                        HOOK -> CALL(original-function)
 * 20         4.4W             all-ELF     dlopen                                        HOOK -> CALL(original-function)
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * 21         5.0              all-ELF     dlopen, android_dlopen_ext                    HOOK -> CALL(original-function)
 * 22         5.1              all-ELF     dlopen, android_dlopen_ext                    HOOK -> CALL(original-function)
 * 23         6.0              all-ELF     dlopen, android_dlopen_ext                    HOOK -> CALL(original-function)
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * 24         7.0              all-ELF     dlopen, android_dlopen_ext                    HOOK -> CALL(dlopen_ext IN linker/linker64) with caller's address
 *                                                                                            OR CALL(g_dl_mutex + do_dlopen IN linker/linker64) with caller's address
 * 25         7.1              all-ELF     dlopen, android_dlopen_ext                    HOOK -> CALL(dlopen_ext IN linker/linker64) with caller's address
 *                                                                                            OR CALL(g_dl_mutex + do_dlopen IN linker/linker64) with caller's address
 * -------------------------------------------------------------------------------------------------------------------------------------------------------------------
 * >= 26      >= 8.0           libdl.so    __loader_dlopen, __loader_android_dlopen_ext  HOOK -> CALL(original-function) with caller's address
 * ===================================================================================================================================================================
 */
// clang-format on

// hook function's type

typedef void *(*bh_dl_monitor_dlopen_t)(const char *, int);
typedef void *(*bh_dl_monitor_android_dlopen_ext_t)(const char *, int, const void *);
typedef void *(*bh_dl_monitor_loader_dlopen_t)(const char *, int, const void *);
typedef void *(*bh_dl_monitor_loader_android_dlopen_ext_t)(const char *, int, const void *, const void *);
typedef int (*bh_dl_monitor_dlclose_t)(void *);
typedef int (*bh_dl_monitor_loader_dlclose_t)(void *);

// hook function's origin function address
// Keep these values after uninit to prevent the concurrent access from crashing.
static bh_dl_monitor_dlopen_t bh_dl_monitor_orig_dlopen = NULL;
static bh_dl_monitor_android_dlopen_ext_t bh_dl_monitor_orig_android_dlopen_ext = NULL;
static bh_dl_monitor_loader_dlopen_t bh_dl_monitor_orig_loader_dlopen = NULL;
static bh_dl_monitor_loader_android_dlopen_ext_t bh_dl_monitor_orig_loader_android_dlopen_ext = NULL;
static bh_dl_monitor_dlclose_t bh_dl_monitor_orig_dlclose = NULL;
static bh_dl_monitor_loader_dlclose_t bh_dl_monitor_orig_loader_dlclose = NULL;


// hook task's stub for unhooking
// Reset to NULL after uninit.
static plt_hook_stub_t bh_dl_monitor_stub_dlopen = NULL;
static plt_hook_stub_t bh_dl_monitor_stub_android_dlopen_ext = NULL;
static plt_hook_stub_t bh_dl_monitor_stub_loader_dlopen = NULL;
static plt_hook_stub_t bh_dl_monitor_stub_loader_android_dlopen_ext = NULL;
static plt_hook_stub_t bh_dl_monitor_stub_dlclose = NULL;
static plt_hook_stub_t bh_dl_monitor_stub_loader_dlclose = NULL;


// the callback which will be called after dlopen() or android_dlopen_ext() and dlclose() successful
// Keep these values after uninit to prevent the concurrent access from crashing.
static bh_dl_monitor_post_dlopen_t bh_dl_monitor_post_dlopen = NULL;
static void *bh_dl_monitor_post_dlopen_arg = NULL;
static bh_dl_monitor_post_dlclose_t bh_dl_monitor_post_dlclose = NULL;
static void *bh_dl_monitor_post_dlclose_arg = NULL;


// the callbacks for every dlopen() or android_dlopen_ext()
typedef struct bh_dl_monitor_cb {
    plt_hook_pre_dlopen_t pre;
    plt_hook_post_dlopen_t post;
    void *data;
    TAILQ_ENTRY(bh_dl_monitor_cb, ) link;
} bh_dl_monitor_cb_t;
typedef TAILQ_HEAD(bh_dl_monitor_cb_queue, bh_dl_monitor_cb, ) bh_dl_monitor_cb_queue_t;
static bh_dl_monitor_cb_queue_t bh_dl_monitor_cbs = TAILQ_HEAD_INITIALIZER(bh_dl_monitor_cbs);
static pthread_rwlock_t bh_dl_monitor_cbs_lock = PTHREAD_RWLOCK_INITIALIZER;

