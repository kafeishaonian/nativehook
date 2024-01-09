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

typedef void *(*bh_dl_monitor_loader_android_dlopen_ext_t)(const char *, int, const void *,
                                                           const void *);

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
    TAILQ_ENTRY(bh_dl_monitor_cb,) link;
} bh_dl_monitor_cb_t;
typedef TAILQ_HEAD(bh_dl_monitor_cb_queue, bh_dl_monitor_cb,) bh_dl_monitor_cb_queue_t;
static bh_dl_monitor_cb_queue_t bh_dl_monitor_cbs = TAILQ_HEAD_INITIALIZER(bh_dl_monitor_cbs);
static pthread_rwlock_t bh_dl_monitor_cbs_lock = PTHREAD_RWLOCK_INITIALIZER;


static void bh_dl_monitor_call_cb_pre(const char *filename) {
    if (TAILQ_EMPTY(&bh_dl_monitor_cbs)) {
        return;
    }

    pthread_rwlock_rdlock(&bh_dl_monitor_cbs_lock);
    bh_dl_monitor_cb_t *cb;
    TAILQ_FOREACH(cb, &bh_dl_monitor_cbs, link) {
        if (NULL != cb->pre) {
            cb->pre(filename, cb->data);
        }
    }
    pthread_rwlock_unlock(&bh_dl_monitor_cbs_lock);
}


static void bh_dl_monitor_call_cb_post(const char *filename, int result) {
    if (TAILQ_EMPTY(&bh_dl_monitor_cbs)) {
        return;
    }

    pthread_rwlock_rdlock(&bh_dl_monitor_cbs_lock);
    bh_dl_monitor_cb_t *cb;
    TAILQ_FOREACH(cb, &bh_dl_monitor_cbs, link) {
        if (NULL != cb->post) {
            cb->post(filename, result, cb->data);
        }
    }
    pthread_rwlock_unlock(&bh_dl_monitor_cbs_lock);
}


// callback for hooking dlopen()
static void bh_dl_monitor_proxy_dlopen_hooked(plt_hook_stub_t task_stub, int status_code,
                                              const char *caller_path_name, const char *sym_name,
                                              void *new_func, void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_dlopen != prev_func) {
        bh_dl_monitor_orig_dlopen = (bh_dl_monitor_dlopen_t) prev_func;
    }
}

// callback for hooking android_dlopen_ext()
static void
bh_dl_monitor_proxy_android_dlopen_ext_hooked(plt_hook_stub_t task_stub, int status_code,
                                              const char *caller_path_name, const char *sym_name,
                                              void *new_func, void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_android_dlopen_ext != prev_func) {
        bh_dl_monitor_orig_android_dlopen_ext = (bh_dl_monitor_android_dlopen_ext_t) prev_func;
    }
}


// callback for hooking __loader_dlopen()
static void bh_dl_monitor_proxy_loader_dlopen_hooked(plt_hook_stub_t task_stub, int status_code,
                                                     const char *caller_path_name,
                                                     const char *sym_name,
                                                     void *new_func, void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_loader_dlopen != prev_func) {
        bh_dl_monitor_orig_loader_dlopen = (bh_dl_monitor_loader_dlopen_t) prev_func;
    }
}

// callback for hooking __loader_android_dlopen_ext()
static void
bh_dl_monitor_proxy_loader_android_dlopen_ext_hooked(plt_hook_stub_t task_stub, int status_code,
                                                     const char *caller_path_name,
                                                     const char *sym_name, void *new_func,
                                                     void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_loader_android_dlopen_ext != prev_func) {
        bh_dl_monitor_orig_loader_android_dlopen_ext = (bh_dl_monitor_loader_android_dlopen_ext_t) prev_func;
    }
}

// callback for hooking dlclose()
static void bh_dl_monitor_proxy_dlclose_hooked(plt_hook_stub_t task_stub, int status_code,
                                               const char *caller_path_name, const char *sym_name,
                                               void *new_func, void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_dlclose != prev_func) {
        bh_dl_monitor_orig_dlclose = (bh_dl_monitor_dlclose_t) prev_func;
    }
}

// callback for hooking __loader_dlclose()
static void bh_dl_monitor_proxy_loader_dlclose_hooked(plt_hook_stub_t task_stub, int status_code,
                                                      const char *caller_path_name,
                                                      const char *sym_name,
                                                      void *new_func, void *prev_func, void *arg) {

    (void) task_stub, (void) caller_path_name, (void) sym_name, (void) new_func, (void) arg;
    if (PLT_HOOK_STATUS_CODE_ORIG_ADDR == status_code &&
        (void *) bh_dl_monitor_orig_loader_dlclose != prev_func) {
        bh_dl_monitor_orig_loader_dlclose = (bh_dl_monitor_loader_dlclose_t) prev_func;
    }
}


static pthread_key_t bh_dl_monitor_dlerror_msg_tls_key;

static void bh_dl_monitor_set_dlerror_msg(void) {
#define LIBC_TLS_SLOT_DLERROR  6
#define ERRMSG_SZ              256

    char *msg = "dlopen failed";
    char *detail = const_cast<char *>(NULL != bh_linker_get_error_buffer
                                      ? bh_linker_get_error_buffer() : "");

    if (NULL != bh_linker_bionic_format_dlerror) {
        // call linker's __bionic_format_dlerror()
        bh_linker_bionic_format_dlerror(msg, detail);
    } else {
        // get libc TLS array
        void **libc_tls = NULL;
#if defined(__aarch64__)
        __asm__("mrs %0, tpidr_el0" : "=r"(libc_tls));
#elif defined(__arm__)
        __asm__("mrc p15, 0, %0, c13, c0, 3" : "=r"(libc_tls));
#elif defined(__i386__)
        __asm__("movl %%gs:0, %0" : "=r"(libc_tls));
#elif defined(__x86_64__)
        __asm__("mov %%fs:0, %0" : "=r"(libc_tls));
#endif


        char *errmsg = msg;
        if ('\0' != detail[0]) {
            char *errmsg_tls = (char *) pthread_getspecific(bh_dl_monitor_dlerror_msg_tls_key);
            if (NULL == errmsg_tls) {
                if (NULL == (errmsg_tls = static_cast<char *>(malloc(ERRMSG_SZ)))) {
                    goto end;
                }
                pthread_setspecific(bh_dl_monitor_dlerror_msg_tls_key, (void *) errmsg_tls);
            }
            snprintf(errmsg_tls, ERRMSG_SZ, "%s: %s", msg, detail);
            errmsg = errmsg_tls;
        }

        end:
        ((char **) libc_tls)[LIBC_TLS_SLOT_DLERROR] = errmsg;
    }
}


// dlerror message TLS DTOR
static void bh_dl_monitor_dlerror_msg_tls_dtor(void *buf) {
    if (NULL != buf) {
        free(buf);
    }
}


// lock between "dlclose"(wrlock) and "read elf cache"(rdlock)
static pthread_rwlock_t bh_dl_monitor_dlclose_lock = PTHREAD_RWLOCK_INITIALIZER;


static int bh_dl_monitor_dlclose_wrlock(void) {
    return pthread_rwlock_wrlock(&bh_dl_monitor_dlclose_lock);
}

void bh_dl_monitor_dlclose_rdlock(void) {
    pthread_rwlock_rdlock(&bh_dl_monitor_dlclose_lock);
}

void bh_dl_monitor_dlclose_unlock(void) {
    pthread_rwlock_unlock(&bh_dl_monitor_dlclose_lock);
}

// proxy for dlopen() when API level [16, 25]
static void *bh_dl_monitor_proxy_dlopen(const char *filename, int flags) {
    bh_dl_monitor_call_cb_pre(filename);
    int api_level = bh_util_get_api_level();

    bh_linker_add_lock_count();
    void *handle = NULL;
    if (api_level >= __ANDROID_API_J__ && api_level <= __ANDROID_API_M__) {
        if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
            handle = bh_dl_monitor_orig_dlopen(filename, flags);
        } else
            handle = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_dlopen, bh_dl_monitor_dlopen_t,
                                        filename, flags);

    } else if (__ANDROID_API_N__ == api_level || __ANDROID_API_N_MR1__ == api_level) {
        void *caller_addr = PLT_HOOK_RETURN_ADDRESS();
        if (NULL != bh_linker_dlopen_ext) {
            handle = bh_linker_dlopen_ext(filename, flags, NULL, caller_addr);
        } else {
            bh_linker_lock();
            handle = bh_linker_do_dlopen(filename, flags, NULL, caller_addr);
            if (NULL == handle) {
                bh_dl_monitor_set_dlerror_msg();
            }
            bh_linker_unlock();
        }
    }
    bh_linker_sub_lock_count();

    // call dl_iterate_phdr() to update ELF-info-cache
    if (!bh_linker_is_in_lock() && NULL != handle && NULL != bh_dl_monitor_post_dlopen) {
        BH_LOG_INFO("DL monitor: post dlopen(), filename: %s", filename);
        bh_dl_monitor_post_dlopen(bh_dl_monitor_post_dlopen_arg);
    }

    PLT_HOOK_POP_STACK();
    bh_dl_monitor_call_cb_post(filename, NULL != handle ? 0 : -1);
    return handle;
}

// proxy for android_dlopen_ext() when API level [21, 25]
static void *
bh_dl_monitor_proxy_android_dlopen_ext(const char *filename, int flags, const void *extinfo) {
    bh_dl_monitor_call_cb_pre(filename);
    int api_level = bh_util_get_api_level();

    bh_linker_add_lock_count();
    void *handle = NULL;
    if (api_level >= __ANDROID_API_L__ && api_level <= __ANDROID_API_M__) {
        if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
            handle = bh_dl_monitor_orig_android_dlopen_ext(filename, flags, extinfo);
        } else {
            handle = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_android_dlopen_ext,
                                        bh_dl_monitor_android_dlopen_ext_t,
                                        filename, flags, extinfo);
        }
    } else if (__ANDROID_API_N__ == api_level || __ANDROID_API_N_MR1__ == api_level) {
        void *caller_address = PLT_HOOK_RETURN_ADDRESS();
        if (NULL != bh_linker_dlopen_ext) {
            handle = bh_linker_dlopen_ext(filename, flags, extinfo, caller_address);
        } else {
            bh_linker_lock();
            handle = bh_linker_do_dlopen(filename, flags, extinfo, caller_address);
            if (NULL == handle) {
                bh_dl_monitor_set_dlerror_msg();
            }
            bh_linker_unlock();
        }
    }
    bh_linker_sub_lock_count();

    if (!bh_linker_is_in_lock() && NULL != handle && NULL != bh_dl_monitor_post_dlopen) {
        BH_LOG_INFO("DL monitor: post android_dlopen_ext(), filename: %s", filename);
        bh_dl_monitor_post_dlopen(bh_dl_monitor_post_dlopen_arg);
    }

    PLT_HOOK_POP_STACK();
    bh_dl_monitor_call_cb_post(filename, NULL != handle ? 0 : -1);
    return handle;
}


// proxy for __loader_dlopen() when API level >= 26
static void *
bh_dl_monitor_proxy_loader_dlopen(const char *filename, int flags, const void *caller_address) {
    bh_dl_monitor_call_cb_pre(filename);

    bh_linker_add_lock_count();
    void *handle = NULL;
    if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
        handle = bh_dl_monitor_orig_loader_dlopen(filename, flags, caller_address);
    } else {
        handle = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_loader_dlopen,
                                    bh_dl_monitor_loader_dlopen_t, filename,
                                    flags, caller_address);
    }
    bh_linker_sub_lock_count();

    if (!bh_linker_is_in_lock() && NULL != handle && NULL != bh_dl_monitor_post_dlopen) {
        BH_LOG_INFO("DL monitor: post __loader_dlopen(), filename: %s", filename);
        bh_dl_monitor_post_dlopen(bh_dl_monitor_post_dlopen_arg);
    }

    PLT_HOOK_POP_STACK();
    bh_dl_monitor_call_cb_post(filename, NULL != handle ? 0 : -1);
    return handle;
}


static void *bh_dl_monitor_proxy_loader_android_dlopen_ext(const char *filename, int flags,
                                                           const void *extinfo,
                                                           const void *caller_address) {
    bh_dl_monitor_call_cb_pre(filename);

    bh_linker_add_lock_count();
    void *handle = NULL;
    if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
        handle = bh_dl_monitor_orig_loader_android_dlopen_ext(filename, flags, extinfo,
                                                              caller_address);
    } else {
        handle = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_loader_android_dlopen_ext,
                                    bh_dl_monitor_loader_android_dlopen_ext_t,
                                    filename, flags, extinfo, caller_address);
    }
    bh_linker_sub_lock_count();

    if (!bh_linker_is_in_lock() && NULL != handle && NULL != bh_dl_monitor_post_dlopen) {
        BH_LOG_INFO("DL monitor: post __loader_android_dlopen_ext(), filename: %s", filename);
        bh_dl_monitor_post_dlopen(bh_dl_monitor_post_dlopen_arg);
    }

    PLT_HOOK_POP_STACK();
    bh_dl_monitor_call_cb_post(filename, NULL != handle ? 0 : -1);
    return handle;
}

// proxy for dlclose()
static int bh_dl_monitor_proxy_dlclose(void *handle) {
    bool wrlocked = false;
    if (!bh_linker_is_in_lock()) {
        wrlocked = (0 == bh_dl_monitor_dlclose_wrlock());
    }

    // call dlclose()
    bh_linker_add_lock_count();
    int ret;
    if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
        ret = bh_dl_monitor_orig_dlclose(handle);
    } else {
        ret = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_dlclose, bh_dl_monitor_dlclose_t, handle);
    }
    bh_linker_sub_lock_count();

    // call dl_iterate_phdr() to update ELF-info-cache
    if (!bh_linker_is_in_lock() && 0 == ret && NULL != bh_dl_monitor_post_dlclose) {
        BH_LOG_INFO("DL monitor: post dlclose(), handle: %p", handle);
        bh_dl_monitor_post_dlclose(wrlocked, bh_dl_monitor_post_dlclose_arg);
    }

    if (wrlocked) bh_dl_monitor_dlclose_unlock();
    PLT_HOOK_POP_STACK();
    return ret;
}

// proxy for __loader_dlclose()
static int bh_dl_monitor_proxy_loader_dlclose(void *handle) {
    bool wrlocked = false;
    if (!bh_linker_is_in_lock()) {
        wrlocked = (0 == bh_dl_monitor_dlclose_wrlock());
    }

    // call __loader_dlclose()
    bh_linker_add_lock_count();
    int ret;
    if (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) {
        ret = bh_dl_monitor_orig_loader_dlclose(handle);
    } else {
        ret = PLT_HOOK_CALL_PREV(bh_dl_monitor_proxy_loader_dlclose, bh_dl_monitor_loader_dlclose_t,
                                 handle);
    }
    bh_linker_sub_lock_count();

    // call dl_iterate_phdr() to update ELF-info-cache
    if (!bh_linker_is_in_lock() && 0 == ret && NULL != bh_dl_monitor_post_dlclose) {
        BH_LOG_INFO("DL monitor: post __loader_dlclose(), handle: %p", handle);
        bh_dl_monitor_post_dlclose(wrlocked, bh_dl_monitor_post_dlclose_arg);
    }

    if (wrlocked) {
        bh_dl_monitor_dlclose_unlock();
    }
    PLT_HOOK_POP_STACK();
    return ret;
}


static int bh_dl_monitor_hook(void) {
    int api_level = bh_util_get_api_level();

    if (__ANDROID_API_N__ == api_level || __ANDROID_API_N_MR1__ == api_level) {
        if (NULL != bh_linker_do_dlopen && NULL == bh_linker_bionic_format_dlerror &&
            NULL != bh_linker_get_error_buffer) {
            if (0 != pthread_key_create(&bh_dl_monitor_dlerror_msg_tls_key, bh_dl_monitor_dlerror_msg_tls_dtor)) {
                goto err;
            }
        }
    }

    if (api_level >= __ANDROID_API_J__ && api_level <= __ANDROID_API_N_MR1__) {
        if (NULL == (bh_dl_monitor_stub_dlopen = bh_core_hook_all(
                NULL, BH_CONST_SYM_DLOPEN, (void *)bh_dl_monitor_proxy_dlopen,
                (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) ? bh_dl_monitor_proxy_dlopen_hooked : NULL,
                NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }
    }

    if (api_level >= __ANDROID_API_L__ && api_level <= __ANDROID_API_N_MR1__) {
        if (NULL == (bh_dl_monitor_stub_android_dlopen_ext = bh_core_hook_all(
                    NULL, BH_CONST_SYM_ANDROID_DLOPEN_EXT, (void *)bh_dl_monitor_proxy_android_dlopen_ext,
                    (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) ? bh_dl_monitor_proxy_android_dlopen_ext_hooked: NULL,
                    NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }
    }

    if (api_level >= __ANDROID_API_O__) {
        if (NULL == (bh_dl_monitor_stub_loader_dlopen = bh_core_hook_single(
                    BH_CONST_BASENAME_DL, NULL,
                    BH_CONST_SYM_LOADER_DLOPEN,  // STT_FUNC or STT_NOTYPE
                    (void *)bh_dl_monitor_proxy_loader_dlopen,
                    (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) ? bh_dl_monitor_proxy_loader_dlopen_hooked : NULL,
                    NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }

        if (NULL == (bh_dl_monitor_stub_loader_android_dlopen_ext =
                             bh_core_hook_single(BH_CONST_BASENAME_DL, NULL,
                                                 BH_CONST_SYM_LOADER_ANDROID_DLOPEN_EXT,  // STT_FUNC or STT_NOTYPE
                                                 (void *)bh_dl_monitor_proxy_loader_android_dlopen_ext,
                                                 (PLT_HOOK_MODE_MANUAL == bh_core_get_mode())
                                                 ? bh_dl_monitor_proxy_loader_android_dlopen_ext_hooked
                                                 : NULL,
                                                 NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }
    }

    if (api_level < __ANDROID_API_O__) {
        if (NULL == (bh_dl_monitor_stub_dlclose = bh_core_hook_all(
                NULL, BH_CONST_SYM_DLCLOSE, (void *)bh_dl_monitor_proxy_dlclose,
                (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) ? bh_dl_monitor_proxy_dlclose_hooked : NULL,
                NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }
    } else {
        if (NULL == (bh_dl_monitor_stub_loader_dlclose = bh_core_hook_single(
                    BH_CONST_BASENAME_DL, NULL,
                    BH_CONST_SYM_LOADER_DLCLOSE,  // STT_FUNC or STT_NOTYPE
                    (void *)bh_dl_monitor_proxy_loader_dlclose,
                    (PLT_HOOK_MODE_MANUAL == bh_core_get_mode()) ? bh_dl_monitor_proxy_loader_dlclose_hooked : NULL,
                    NULL, (uintptr_t)(__builtin_return_address(0))))) {
            goto err;
        }
    }

    return 0;


err:
    bh_dl_monitor_uninit();
    return -1;
}


static bool bh_dl_monitor_initing = false;
bool bh_dl_monitor_is_initing(void) {
    return bh_dl_monitor_initing;
}

int bh_dl_monitor_init(void) {

}

void bh_dl_monitor_uninit(void) {

}

void bh_dl_monitor_set_post_dlopen(bh_dl_monitor_post_dlopen_t cb, void *cb_arg) {

}

void bh_dl_monitor_set_post_dlclose(bh_dl_monitor_post_dlclose_t cb, void *cb_arg) {

}

void bh_dl_monitor_add_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data) {

}

void bh_dl_monitor_delete_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data) {

}

