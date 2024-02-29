//
// Created by MOMO on 2024/2/26.
//

#ifndef NATIVEHOOK_SH_ERRNO_H
#define NATIVEHOOK_SH_ERRNO_H

#include "sh_log.h"

#define SH_ERRNO_SET_RET_ERRNUM(errnum) SH_ERRNO_SET_RET((errnum), (errnum))
#define SH_ERRNO_SET_RET_FAIL(errnum)   SH_ERRNO_SET_RET((errnum), -1)
#define SH_ERRNO_SET_RET_NULL(errnum)   SH_ERRNO_SET_RET((errnum), NULL)
#define SH_ERRNO_SET_RET(errnum, ret) \
  do {                                \
    sh_errno_set((errnum));           \
    return (ret);                     \
  } while (0)


int sh_errno_init(void);

void sh_errno_reset(void);

void sh_errno_set(int error_number);

int sh_errno_get(void);

const char *sh_errno_to_errmsg(int error_number);

#endif //NATIVEHOOK_SH_ERRNO_H
