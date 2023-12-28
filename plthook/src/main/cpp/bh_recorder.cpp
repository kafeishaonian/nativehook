//
// Created by MOMO on 2023/12/27.
//

#include "bh_recorder.h"

#include <dlfcn.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "bh_util.h"
#include "plthook.h"

#define BH_RECORDER_OP_HOOK     0
#define BH_RECORDER_OP_UNHOOK   1

#define BH_RECORDER_LIB_NAME_MAX    512
#define BH_RECORDER_SYM_NAME_MAX    1024

#define BH_RECORDER_STRINGS_BUF_EXPAND_STEP     (1024 * 16)
#define BH_RECORDER_STRINGS_BUG_MAX             (1024 * 128)
#define BH_RECORDER_RECORDS_BUG_EXPAND_STEP     (1024 * 32)
#define BH_RECORDER_RECORDS_BUF_MAX             (1024 * 384)
#define BH_RECORDER_OUTPUT_BUF_EXPAND_STEP      (1024 * 128)
#define BH_RECORDER_OUTPUT_BUF_MAX              (1024 * 1024)

static bool bh_recorder_recordable = false;

bool bh_recorder_get_recordable(void ){
    return bh_recorder_recordable;
}

void bh_recorder_set_recordable(bool recordable) {
    bh_recorder_recordable = recordable;
}


typedef struct {
    void *ptr;
    size_t cap;
    size_t sz;
    pthread_mutex_t lock;
} bh_recorder_buf_t;


static int bh_recorder_buf_append(bh_recorder_buf_t *buf, size_t step, size_t max, const void *header,
                                  size_t header_sz, const void *body, size_t body_sz) {
    size_t needs = (header_sz + (NULL != body ? body_sz : 0));
    if (needs > step) {
        return -1;
    }

    if (buf->cap - buf->sz < needs) {
        size_t new_cap = buf->cap + step;
        if (new_cap > max) {
            return -1;
        }
        void *new_ptr = realloc(buf->ptr, new_cap);
        if (NULL == new_ptr) {
            return -1;
        }
        buf->ptr = new_ptr;
        buf->cap = new_cap;
    }

    memcpy((void *)((uintptr_t)buf->ptr + buf->sz), header, header_sz);
    if (NULL != body) {
        memcpy((void *)((uintptr_t)buf->ptr + buf->sz + header_sz), body, body_sz);
    }
    buf->sz += needs;
    return 0;
}

static void bh_recorder_buf_free(bh_recorder_buf_t *buf) {
    if (NULL != buf->ptr) {
        free(buf->ptr);
        buf->ptr = NULL;
    }
}

static bh_recorder_buf_t bh_recorder_strings = {NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER};
static bh_recorder_buf_t bh_recorder_records = {NULL, 0, 0, PTHREAD_MUTEX_INITIALIZER};
static bool bh_recorder_error = false;

typedef struct {
    uint16_t str_len;
} __attribute__((packed)) bh_recorder_str_header_t;


typedef struct {
    uint64_t op : 8;
    uint64_t error_number : 8;
    uint64_t  ts_ms : 48;
    uintptr_t stub;
    uint16_t caller_lib_name_idx;
    uint16_t lib_name_idx;
    uint16_t sym_name_idx;
    uintptr_t new_addr;
} __attribute__((packed)) bh_recorder_record_hook_header_t;

typedef struct {
    uint64_t op : 8;
    uint64_t error_number : 8;
    uint64_t ts_ms : 48;
    uintptr_t stub;
    uint16_t caller_lib_name_idx;
} __attribute__((packed)) bh_recorder_record_unhook_header_t;


static int bh_recorder_add_str(const char *str, size_t str_len, uint16_t *str_idx) {
    uint16_t idx = 0;
    bool ok = false;

    pthread_mutex_lock(&bh_recorder_strings.lock);

    size_t i = 0;
    while (i < bh_recorder_strings.sz) {
        bh_recorder_str_header_t  *header = (bh_recorder_str_header_t *)((uintptr_t)bh_recorder_strings.ptr + i);
        if (header->str_len == str_len) {
            void *tmp = (void *)((uintptr_t)bh_recorder_strings.ptr + i + sizeof(header->str_len));
            if (0 == memcmp(tmp, str, str_len)) {
                *str_idx = idx;
                ok = true;
                break;
            }
        }
        i += (sizeof(bh_recorder_str_header_t) + header->str_len + 1);
        idx++;
        if (idx == UINT16_MAX) {
            break;
        }
    }

    if (!ok && idx < UINT16_MAX) {
        bh_recorder_str_header_t header = {(uint16_t)str_len};
        if (0 == bh_recorder_buf_append(&bh_recorder_strings, BH_RECORDER_STRINGS_BUF_EXPAND_STEP,
                                        BH_RECORDER_STRINGS_BUG_MAX, &header, sizeof(header), str, str_len + 1)) {
            *str_idx = idx;
            ok = true;
        }
    }

    pthread_mutex_unlock(&bh_recorder_strings.lock);

    return ok ? 0 : -1;
}


static char *bh_recorder_find_str(uint64_t idx) {
    uint16_t cur_idx = 0;

    size_t i = 0;
    while (i < bh_recorder_strings.sz && cur_idx < idx) {
        bh_recorder_str_header_t  *header = (bh_recorder_str_header_t *)((uintptr_t)bh_recorder_strings.ptr + i);
        i += (sizeof(bh_recorder_str_header_t) + header->str_len + 1);
        cur_idx++;
    }
    if (cur_idx != idx) {
        return "error";
    }

    bh_recorder_str_header_t  *header = (bh_recorder_str_header_t *)((uintptr_t)bh_recorder_strings.ptr + i);
    return (char *)((uintptr_t)header + sizeof(bh_recorder_str_header_t));
}

static long bh_recorder_tz = LONG_MAX;

static uint64_t bh_recorder_get_timestamp_ms(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);

    if (LONG_MAX == bh_recorder_tz) {
        bh_recorder_tz = 0;
    }

    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
}


static size_t bh_recorder_format_timestamp_ms(uint64_t ts_ms, char *buf, size_t buf_len) {
    time_t sec = (time_t)(ts_ms / 1000);
    suseconds_t msec = (time_t)(ts_ms % 1000);

    struct tm tm;
    bh_util_localtime_r(&sec, bh_recorder_tz, &tm);

    return bh_util_snprintf(buf, buf_len, "%04d-%02d-%02dT%02d:%02d:%02d.%03ld%c%02ld:%02ld,",
                            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                            msec, bh_recorder_tz < 0 ? '-' : '+', labs(bh_recorder_tz / 3600),
                            labs(bh_recorder_tz % 3600));
}

static const char *bh_recorder_get_basename(const char *lib_name) {
    const char *p = strrchr(lib_name, '/');
    if (NULL != p && '\0' != *(p + 1)) {
        return p + 1;
    } else {
        return lib_name;
    }
}

static void bh_recorder_get_basename_by_addr(uintptr_t addr, char *lib_name, size_t lin_name_sz) {
    Dl_info info;
    if (0 == dladdr((void *)addr, &info) || NULL == info.dli_fname || '\0' == info.dli_fname[0]) {
        strlcpy(lib_name, "unknown", lin_name_sz);
    } else {
        const char *p = strrchr(info.dli_fname, '/');
        if (NULL == p || '\0' == *(p + 1)) {
            p = info.dli_fname;
        } else {
            p++;
        }
        strlcpy(lib_name, p, lin_name_sz);
    }
}

int bh_recorder_add_hook(int error_number, const char *lib_name, const char *sym_name, uintptr_t new_addr,
                         uintptr_t stub, uintptr_t caller_addr) {
    if (!bh_recorder_recordable) {
        return 0;
    }
    if (bh_recorder_error) {
        return -1;
    }

    if (NULL == lib_name) {
        lib_name = "unknown";
    } else {
        lib_name = bh_recorder_get_basename(lib_name);
    }

    size_t lib_name_len = strlen(lib_name);
    if (0 == lib_name_len || lib_name_len > BH_RECORDER_LIB_NAME_MAX) {
        return -1;
    }

    if (NULL == sym_name) {
        return -1;
    }
    size_t sym_name_len = strlen(sym_name);
    if (0 == sym_name_len || sym_name_len > BH_RECORDER_SYM_NAME_MAX) {
        return -1;
    }

    char caller_lib_name[BH_RECORDER_LIB_NAME_MAX];
    bh_recorder_get_basename_by_addr(caller_addr, caller_lib_name, sizeof(caller_lib_name));
    size_t caller_lib_name_len = strlen(caller_lib_name);

    uint16_t lib_name_idx, sym_name_idx, caller_lib_name_idx;
    if (0 != bh_recorder_add_str(lib_name, lib_name_len, &lib_name_idx)) {
        goto err;
    }
    if (0 != bh_recorder_add_str(sym_name, sym_name_len, &sym_name_idx)) {
        goto err;
    }
    if (0 != bh_recorder_add_str(caller_lib_name, caller_lib_name_len, &caller_lib_name_idx)) {
        goto err;
    }

    bh_recorder_record_hook_header_t header = {BH_RECORDER_OP_HOOK,
                                               (uint8_t)error_number,
                                               bh_recorder_get_timestamp_ms(),
                                               stub,
                                               caller_lib_name_idx,
                                               lib_name_idx,
                                               sym_name_idx,
                                               new_addr};
    pthread_mutex_lock(&bh_recorder_records.lock);
    int r = bh_recorder_buf_append(&bh_recorder_records, BH_RECORDER_RECORDS_BUG_EXPAND_STEP,
                                   BH_RECORDER_RECORDS_BUF_MAX, &header, sizeof(header), NULL, 0);
    pthread_mutex_unlock(&bh_recorder_records.lock);
    if (0 != r) {
        goto err;
    }
    return 0;
    
    err:
    bh_recorder_error = true;
    return -1;
}

int bh_recorder_add_unhook(int error_number, uintptr_t stub, uintptr_t caller_addr) {
    if (!bh_recorder_recordable) {
        return 0;
    }
    if (bh_recorder_error) {
        return -1;
    }
    
    char caller_lib_name[BH_RECORDER_LIB_NAME_MAX];
    bh_recorder_get_basename_by_addr(caller_addr, caller_lib_name, sizeof(caller_lib_name));
    size_t caller_lib_name_len = strlen(caller_lib_name);
    
    uint16_t caller_lib_name_idx;
    bh_recorder_record_unhook_header_t header;
    int r;
    if (0 != bh_recorder_add_str(caller_lib_name, caller_lib_name_len, &caller_lib_name_idx)) {
        goto err;
    }

    header = {BH_RECORDER_OP_UNHOOK, (uint8_t)error_number,
                                                 bh_recorder_get_timestamp_ms(), stub, caller_lib_name_idx};

    pthread_mutex_lock(&bh_recorder_records.lock);
    r = bh_recorder_buf_append(&bh_recorder_records, BH_RECORDER_RECORDS_BUG_EXPAND_STEP,
                                   BH_RECORDER_RECORDS_BUF_MAX, &header, sizeof(header), NULL, 0);
    pthread_mutex_unlock(&bh_recorder_records.lock);
    if (0 != r) {
        goto err;
    }
    
    err:
    bh_recorder_error = true;
    return -1;
}





