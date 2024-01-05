//
// Created by MOMO on 2024/1/4.
//

#ifndef NATIVEHOOK_BH_CORE_H
#define NATIVEHOOK_BH_CORE_H

#include <stdbool.h>

#include "bh_elf_manager.h"
#include "bh_hook_manager.h"
#include "bh_task_manager.h"
#include "plthook.h"


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
typedef struct {
    int init_status;
    int mode;
    bh_task_manager_t *task_mgr;
    bh_hook_manager_t *hook_mgr;
    bh_elf_manager_t *elf_mgr;
} bh_core_t;
#pragma clang diagnostic pop

bh_core_t *bh_core_global(void);

int bh_core_init(int mode, bool debug);

plt_hook_stub_t bh_core_hook_single(const char *caller_path_name, const char *callee_path_name,
                                    const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                    void *hooked_arg, uintptr_t caller_address);

plt_hook_stub_t bh_core_hook_partial(plt_hook_caller_allow_filter_t caller_allow_filter,
                                     void *caller_allow_filter_arg, const char *callee_path_name,
                                     const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                     void *hooked_arg, uintptr_t caller_address);

plt_hook_stub_t bh_core_hook_all(const char *callee_path_name, const char *sym_name, void *new_func,
                                 plt_hook_hooked_t hooked, void *hooked_arg, uintptr_t caller_address);

int bh_core_unhook(plt_hook_stub_t stub, uintptr_t caller_address);

int bh_core_add_ignore(const char *caller_path_name);

bool bh_core_get_debug(void);
void bh_core_set_debug(bool debug);
bool bh_core_get_recordable(void);
void bh_core_set_recordable(bool recordable);

void *bh_core_get_prev_func(void *func);

void *bh_core_get_return_address(void);

void bh_core_pop_stack(void *return_address);

int bh_core_get_mode(void);

void bh_core_add_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data);

void bh_core_delete_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data);


#endif //NATIVEHOOK_BH_CORE_H
