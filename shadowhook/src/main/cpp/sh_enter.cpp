//
// Created by MOMO on 2024/3/20.
//

#include "sh_enter.h"

#include <stdint.h>

#include "sh_trampo.h"

#define SH_ENTER_PAGE_NAME "shadowhook-enter"
#define SH_ENTER_SZ        256
#define SH_ENTER_DELAY_SEC 10

static sh_trampo_mgr_t sh_enter_trampo_mgr;

int sh_enter_init(void) {
    sh_trampo_init_mgr(&sh_enter_trampo_mgr, SH_ENTER_PAGE_NAME, SH_ENTER_SZ, SH_ENTER_DELAY_SEC);
    return 0;
}

uintptr_t sh_enter_alloc(void) {
    return sh_trampo_alloc(&sh_enter_trampo_mgr, 0, 0, 0);
}

void sh_enter_free(uintptr_t enter) {
    sh_trampo_free(&sh_enter_trampo_mgr, enter);
}


