//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_SH_UTIL_H
#define NATIVEHOOK_SH_UTIL_H

#include <android/api-level.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifndef __ANDROID_API_U__
#define __ANDROID_API_U__ 34
#endif


#define SH_UTIL_ALIGN_START(x, align) ((uintptr_t)(x) & ~((uintptr_t)(align)-1))
#define SH_UTIL_ALIGN_END(x, align)   (((uintptr_t)(x) + (uintptr_t)(align) - 1) & ~((uintptr_t)(align) - 1))

#define SH_UTIL_PAGE_START(x) SH_UTIL_ALIGN_START(x, 0x1000)
#define SH_UTIL_PAGE_END(x)   SH_UTIL_ALIGN_END(x, 0x1000)

#define SH_UTIL_IS_THUMB(address)    ((address)&1u)
#define SH_UTIL_CLEAR_BIT0(address)  ((address)&0xFFFFFFFE)
#define SH_UTIL_SET_BIT0(address)     ((address) | 1u)

#define SH_UTIL_ALIGN_4(pc) ((pc)&0xFFFFFFFC)
#define SH_UTIL_SIGN_EXTEND_32(n, len) \
  ((SH_UTIL_GET_BIT_32(n, len - 1) > 0) ? ((n) | (0xFFFFFFFF << (len))) : n)
#define SH_UTIL_SIGN_EXTEND_64(n, len) \
  ((SH_UTIL_GET_BIT_64(n, len - 1) > 0) ? ((n) | (0xFFFFFFFFFFFFFFFF << (len))) : n)

#define SH_UTIL_GET_BIT_16(n, idx)        ((uint16_t)((n) << (15u - (idx))) >> 15u)
#define SH_UTIL_GET_BITS_16(n, high, low) ((uint16_t)((n) << (15u - (high))) >> (15u - (high) + (low)))
#define SH_UTIL_GET_BIT_32(n, idx)        ((uint32_t)((n) << (31u - (idx))) >> 31u)
#define SH_UTIL_GET_BITS_32(n, high, low) ((uint32_t)((n) << (31u - (high))) >> (31u - (high) + (low)))
#define SH_UTIL_GET_BIT_64(n, idx)        ((uint64_t)((n) << (63u - (idx))) >> 63u)
#define SH_UTIL_GET_BITS_64(n, high, low) ((uint64_t)((n) << (63u - (high))) >> (63u - (high) + (low)))

#define SH_UTIL_TEMP_FAILURE_RETRY(exp)    \
  ({                                       \
    __typeof__(exp) _rc;                   \
    do {                                   \
      errno = 0;                           \
      _rc = (exp);                         \
    } while (_rc == -1 && errno == EINTR); \
    _rc;                                   \
  })

#define SH_UTIL_MAX(a, b)   \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#define SH_UTIL_MIN(a, b)   \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })


int sh_util_mprotect(uintptr_t address, size_t len, int prot);

void sh_util_clear_cache(uintptr_t address, size_t len);

bool sh_util_is_thumb32(uintptr_t target_address);

uint32_t sh_util_arm_expand_imm(uint32_t opcode);

int sh_util_write_inst(uintptr_t target_address, void *inst, size_t inst_len);

int sh_util_get_api_level(void);

int sh_util_write(int fd, const char *buf, size_t buf_len);

struct tm *sh_util_localtime_r(const time_t *timep, long gmtoff, struct tm *result);

size_t sh_util_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list args);
size_t sh_util_snprintf(char *buffer, size_t buffer_size, const char *format, ...);


#endif //NATIVEHOOK_SH_UTIL_H
