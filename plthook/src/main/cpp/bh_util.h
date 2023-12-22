//
// Created by MOMO on 2023/12/21.
//

#ifndef NATIVEHOOK_BH_UTIL_H
#define NATIVEHOOK_BH_UTIL_H


#pragma once
#include <android/api-level.h>
#include <ctype.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <link.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#if defined(__LP64__)
#define BH_UTIL_PRIxADDR "016" PRIxPTR
#else
#define BH_UTIL_PRIxADDR "08" PRIxPTR
#endif

#define BH_UTIL_TEMP_FAILURE_RETRY(exp)    \
  ({                                       \
    __typeof__(exp) _rc;                   \
    do {                                   \
      errno = 0;                           \
      _rc = (exp);                         \
    } while (_rc == -1 && errno == EINTR); \
    _rc;                                   \
  })



int bh_util_set_address_protect(void *address, int protect);
int bh_util_set_protect(void *start, void *end, int protect);

bool bh_util_starts_with(const char *str, const char *start);
bool bh_util_ends_with(const char *str, const char *ending);

size_t bh_util_trim_ending(char *start);

int bh_util_get_api_level(void);

int bh_util_write(int fd, const char *buf, size_t buf_len);

struct tm *bh_util_localtime_r(const time_t *timep, long gmtoff, struct tm *result);

size_t bh_util_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list args);
size_t bh_util_snprintf(char *buffer, size_t buffer_size, const char *format, ...);



#endif //NATIVEHOOK_BH_UTIL_H
