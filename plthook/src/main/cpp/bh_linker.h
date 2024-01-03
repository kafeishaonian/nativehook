//
// Created by MOMO on 2024/1/2.
//

#ifndef NATIVEHOOK_BH_LINKER_H
#define NATIVEHOOK_BH_LINKER_H


#include <pthread.h>
#include <stdbool.h>


typedef void *(*bh_linker_dlopen_ext_t)(const char *, int, const void *, void *);
extern bh_linker_dlopen_ext_t bh_linker_dlopen_ext;

typedef void *(*bh_linker_do_dlopen_t)(const char *, int, const void *, void *);
extern bh_linker_do_dlopen_t bh_linker_do_dlopen;

typedef char *(*bh_linker_get_error_buffer_t)(void);
extern bh_linker_get_error_buffer_t bh_linker_get_error_buffer;

typedef void (*bh_linker_bionic_format_dlerror_t)(const char *, const char *);
extern bh_linker_bionic_format_dlerror_t bh_linker_bionic_format_dlerror;

int bh_linker_init(void);

void bh_linker_lock(void);
void bh_linker_unlock(void);
bool bh_linker_is_in_lock(void);
void bh_linker_add_lock_count(void);
void bh_linker_sub_lock_count(void);


#endif //NATIVEHOOK_BH_LINKER_H
