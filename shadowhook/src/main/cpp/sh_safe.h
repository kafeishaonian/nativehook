//
// Created by MOMO on 2024/3/26.
//

#ifndef NATIVEHOOK_SH_SAFE_H
#define NATIVEHOOK_SH_SAFE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

int sh_safe_init(void);
uintptr_t *sh_safe_get_orig_address_address(uintptr_t target_address);

void *sh_safe_pthread_getspecific(pthread_key_t key);
int sh_safe_pthread_setspecific(pthread_key_t key, const void *value);
void sh_safe_abort(void);

void *sh_safe_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int sh_safe_prctl(int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5);

#endif //NATIVEHOOK_SH_SAFE_H
