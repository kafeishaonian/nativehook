//
// Created by MOMO on 2024/5/30.
//

#ifndef NATIVEHOOK_SH_LINKER_H
#define NATIVEHOOK_SH_LINKER_H


#include <stdbool.h>
#include <stdint.h>


#include "xdl.h"

int sh_linker_init(void);

const char *ch_linker_match_dlfcn(uintptr_t target_address);
bool sh_linker_need_to_hook_dlopen(uintptr_t target_address);

typedef void (*sh_linker_post_dlopen_t)(void *arg);
int sh_linker_hook_dlopen(sh_linker_post_dlopen_t post_dlopen, void *post_dlopen_arg);

int sh_linker_get_dlinfo_by_address(void *address, xdl_info_t *dlinfo, char *lib_name, size_t lib_name_sz,
                                    char *sym_name, size_t sym_name_sz, bool ignore_symbol_check);

int sh_linker_get_dlinfo_by_sym_name(const char *lib_name, const char *sym_name, xdl_info_t *dlinfo,
                                     char *real_lib_name, size_t real_lib_name_sz);


#endif //NATIVEHOOK_SH_LINKER_H
