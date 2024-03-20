//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_XDL_ITERATE_H
#define NATIVEHOOK_XDL_ITERATE_H

#include <link.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*xdl_iterate_phdr_cb_t)(struct dl_phdr_info *info, size_t size, void *arg);
int xdl_iterate_phdr_impl(xdl_iterate_phdr_cb_t cb, void *cb_arg, int flags);

int xdl_iterate_get_full_pathname(uintptr_t base, char *buf, size_t buf_len);

#ifdef __cplusplus
}
#endif



#endif //NATIVEHOOK_XDL_ITERATE_H
