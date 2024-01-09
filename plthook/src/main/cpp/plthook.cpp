//
// Created by MOMO on 2023/12/19.
//

#include "include/plthook.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bh_core.h"
#include "bh_recorder.h"

const char *plt_hook_get_version(void) {
    return PLT_HOOK_VERSION;
}

int plt_hook_init(int mode, bool debug) {
    return bh_core_init(mode, debug);
}


plt_hook_stub_t plt_hook_hook_single(const char *caller_path_name, const char *callee_path_name,
                                     const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                     void *hooked_arg) {

    const void *caller_address = __builtin_return_address(0);
    return bh_core_hook_single(caller_path_name, callee_path_name, sym_name, new_func, hooked, hooked_arg,
                               (uintptr_t)caller_address);
}

plt_hook_stub_t plt_hook_hook_partial(plt_hook_caller_allow_filter_t caller_allow_filter,
                                      void *caller_allow_filter_arg, const char *callee_path_name,
                                      const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                      void *hooked_arg) {

    const void *caller_address = __builtin_return_address(0);
    return bh_core_hook_partial(caller_allow_filter, caller_allow_filter_arg, callee_path_name, sym_name,
                                new_func, hooked, hooked_arg, (uintptr_t)caller_address);
}

plt_hook_stub_t plt_hook_hook_all(const char *callee_path_name, const char *sym_name, void *new_func,
                                  plt_hook_hooked_t hooked, void *hooked_arg) {

    const void *caller_address = __builtin_return_address(0);
    return bh_core_hook_all(callee_path_name, sym_name, new_func, hooked, hooked_arg, (uintptr_t)caller_address);
}


int plt_hook_unhook(plt_hook_stub_t stub) {
    const void *caller_address = __builtin_return_address(0);
    return bh_core_unhook(stub, (uintptr_t)caller_address);
}


int plt_hook_add_ignore(const char *caller_path_name) {
    return bh_core_add_ignore(caller_path_name);
}

bool plt_hook_get_debug(void) {
    return bh_core_get_debug();
}

void plt_hook_set_debug(bool debug) {
    bh_core_set_debug(debug);
}

bool plt_hook_get_recordable(void) {
    return bh_core_get_recordable();
}

void plt_hook_set_recordable(bool recordable) {
    bh_core_set_recordable(recordable);
}

char *plt_hook_get_records(uint32_t item_flags) {
    return bh_recorder_get(item_flags);
}


void plt_hook_dump_records(int fd, uint32_t item_flags) {
    bh_recorder_dump(fd, item_flags);
}

void *plt_hook_get_prev_func(void *func) {
    if (__predict_false(PLT_HOOK_MODE_MANUAL == bh_core_get_mode())) {
        abort();
    }
    return bh_core_get_prev_func(func);
}

void *plt_hook_get_return_address(void) {
    if (__predict_false(PLT_HOOK_MODE_MANUAL == bh_core_get_mode())) {
        abort();
    }
    return bh_core_get_return_address();
}

void plt_hook_pop_stack(void *return_address) {
    if (__predict_false(PLT_HOOK_MODE_MANUAL == bh_core_get_mode())) {
        abort();
    }
    bh_core_pop_stack(return_address);
}

int plt_hook_get_mode(void) {
    return bh_core_get_mode();
}

void plt_hook_add_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data) {
    bh_core_add_dlopen_callback(pre, post, data);
}

void plt_hook_delete_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data) {
    bh_core_delete_dlopen_callback(pre, post, data);
}

