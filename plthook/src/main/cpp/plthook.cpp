//
// Created by MOMO on 2023/12/19.
//

#include "include/plthook.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bh_recorder.h"

const char *plt_hook_get_version(void) {
    return PLT_HOOK_VERSION;
}

int plt_hook_init(int mode, bool debug) {

    return 0;
}


plt_hook_stub_t plt_hook_hook_single(const char *caller_path_name, const char *callee_path_name,
                                     const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                     void *hooked_arg) {

}


