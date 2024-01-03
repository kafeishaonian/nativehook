//
// Created by MOMO on 2024/1/2.
//

#include "bh_linker.h"

#include <android/api-level.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "bh_const.h"
#include "bh_dl.h"
#include "bh_linker_mutex.h"
#include "bh_log.h"
#include "bh_util.h"


typedef struct {
    uint16_t state;
#if defined(__LP64__)
    uint16_t pad;
    int owner_tid;
    char reserved[28];
#else
    uint16_t owner_tid;
#endif
} bh_linker_mutex_internal_t __attribute__((aligned(4)));


#define BH_LINKER_MUTEX_IS_UNLOCKED(v) (((v)&0x3) == 0)
#define BH_LINKER_MUTEX_COUNTER(v)     (((v)&0x1FFC) >> 2)

bh_linker_dlopen_ext_t bh_linker_dlopen_ext = NULL;
bh_linker_do_dlopen_t bh_linker_do_dlopen = NULL;
bh_linker_get_error_buffer_t bh_linker_get_error_buffer = NULL;
bh_linker_bionic_format_dlerror_t bh_linker_bionic_format_dlerror = NULL;

static pthread_mutex_t *bh_linker_g_dl_mutex = NULL;
static bool bh_linker_g_dl_mutex_compatible = false;
static pthread_key_t bh_linker_g_dl_mutex_key;

static bool bh_linker_check_lock_compatible_helper(bh_linker_mutex_internal_t *m, bool unlocked,
                                                   uint16_t counter, int owner_tid) {
    return BH_LINKER_MUTEX_IS_UNLOCKED(m->state) == unlocked
    && BH_LINKER_MUTEX_COUNTER(m->state) == counter &&
    m->owner_tid == owner_tid;
}

static bool bh_linker_check_lock_compatible(void) {
    bool result = true;
    int tid = gettid();
    pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
    bh_linker_mutex_internal_t *m = (bh_linker_mutex_internal_t *)&mutex;

    if (!bh_linker_check_lock_compatible_helper(m, true, 0, 0)) {
        result = false;
    }
    pthread_mutex_lock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, false, 0, tid)) {
        result = false;
    }
    pthread_mutex_lock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, false, 1, tid)) {
        result = false;
    }
    pthread_mutex_lock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, false, 2, tid)) {
        result = false;
    }
    pthread_mutex_unlock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, false, 1, tid)) {
        result = false;
    }
    pthread_mutex_unlock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, false, 0, tid)) {
        result = false;
    }
    pthread_mutex_unlock(&mutex);
    if (!bh_linker_check_lock_compatible_helper(m, true, 0, 0)) {
        result = false;
    }

    return result;
}

#if __ANDROID_API__ < __ANDROID_API_L__
static int bh_linker_init_android_4x(void) {
#if defined(__arm__)
  return NULL == (bh_linker_g_dl_mutex = bh_linker_mutex_get_by_stack()) ? -1 : 0;
#else
  return 0 != pthread_key_create(&bh_linker_g_dl_mutex_key, NULL) ? -1 : 0;
#endif
}
#endif

int bh_linker_init(void) {
    bh_linker_g_dl_mutex_compatible = bh_linker_check_lock_compatible();
    int api_level = bh_util_get_api_level();

#if __ANDROID_API__ < __ANDROID_API_L__
    if (api_level < __ANDROID_API_L__) return bh_linker_init_android_4x();
#endif

    if (!bh_linker_g_dl_mutex_compatible) {
        if (0 != pthread_key_create(&bh_linker_g_dl_mutex_key, NULL)) {
            return -1;
        }
    }

    void *linker = bh_dl_open_linker();
    if (NULL == linker) {
        goto err;
    }

    if (__ANDROID_API_L__ == api_level || __ANDROID_API_L_MR1__ == api_level ||
        __ANDROID_API_N__ == api_level || __ANDROID_API_N_MR1__ == api_level ||
        bh_linker_g_dl_mutex_compatible) {
        if (NULL == (bh_linker_g_dl_mutex = (pthread_mutex_t *)(bh_dl_dsym(linker, BH_CONST_SYM_G_DL_MUTEX)))) {
            goto err;
        }
    }

    if (__ANDROID_API_N__ == api_level || __ANDROID_API_N_MR1__ == api_level) {
        bh_linker_dlopen_ext = (bh_linker_dlopen_ext_t)(bh_dl_dsym(linker, BH_CONST_SYM_DLOPEN_EXT));
        if (NULL == bh_linker_dlopen_ext) {
            if (NULL == (bh_linker_do_dlopen = (bh_linker_do_dlopen_t)(bh_dl_dsym(linker, BH_CONST_SYM_DO_DLOPEN)))) {
                goto err;
            }
            bh_linker_get_error_buffer =
                    (bh_linker_get_error_buffer_t)(bh_dl_dsym(linker, BH_CONST_SYM_LINKER_GET_ERROR_BUFFER));
            bh_linker_bionic_format_dlerror =
                    (bh_linker_bionic_format_dlerror_t)(bh_dl_dsym(linker, BH_CONST_SYM_BIONIC_FORMAT_DLERROR));
        }
    }

    bh_dl_close(linker);
    return 0;

    err:
    if (NULL != linker) bh_dl_close(linker);
    bh_linker_do_dlopen = NULL;
    bh_linker_dlopen_ext = NULL;
    bh_linker_g_dl_mutex = NULL;
    bh_linker_get_error_buffer = NULL;
    bh_linker_bionic_format_dlerror = NULL;
    return -1;
}


void bh_linker_lock(void) {
    if (__predict_true(NULL != bh_linker_g_dl_mutex)) {
        pthread_mutex_lock(bh_linker_g_dl_mutex);
    }
}

void bh_linker_unlock(void) {
    if (__predict_true(NULL != bh_linker_g_dl_mutex)) {
        pthread_mutex_unlock(bh_linker_g_dl_mutex);
    }
}

bool bh_linker_is_in_lock(void) {
    if (__predict_false(!bh_linker_g_dl_mutex_compatible || NULL == bh_linker_g_dl_mutex)) {
        return (intptr_t)(pthread_getspecific(bh_linker_g_dl_mutex_key)) > 0;
    } else {
        bh_linker_mutex_internal_t *m = (bh_linker_mutex_internal_t *)bh_linker_g_dl_mutex;
        uint16_t state = __atomic_load_n(&(m->state), __ATOMIC_RELAXED);
        int owner_tid = (int)__atomic_load_n(&(m->owner_tid), __ATOMIC_RELAXED);
        return !BH_LINKER_MUTEX_IS_UNLOCKED(state) && owner_tid == gettid();
    }
}

void bh_linker_add_lock_count(void) {
    if (__predict_false(!bh_linker_g_dl_mutex_compatible || NULL == bh_linker_g_dl_mutex)) {
        intptr_t count = (intptr_t)(pthread_getspecific(bh_linker_g_dl_mutex_key));
        pthread_setspecific(bh_linker_g_dl_mutex_key, (void *)(count + 1));
    }
}

void bh_linker_sub_lock_count(void) {
    if (__predict_false(!bh_linker_g_dl_mutex_compatible || NULL == bh_linker_g_dl_mutex)) {
        intptr_t count = (intptr_t)(pthread_getspecific(bh_linker_g_dl_mutex_key));
        pthread_setspecific(bh_linker_g_dl_mutex_key, (void *)(count - 1));
    }
}