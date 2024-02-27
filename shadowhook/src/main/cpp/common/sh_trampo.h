//
// Created by MOMO on 2024/2/22.
//

#ifndef NATIVEHOOK_SH_TRAMPO_H
#define NATIVEHOOK_SH_TRAMPO_H

#include <stddef.h>
#include <stdint.h>
#include <sys/time.h>

#include "queue.h"

typedef struct sh_trampo_page {
    uintptr_t ptr;
    uint32_t *flags;
    time_t *timestamps;
    SLIST_ENTRY(sh_trampo_page,) link;
} sh_trampo_page_t;
typedef SLIST_HEAD(sh_trampo_page_list, sh_trampo_page,) sh_trampo_page_list_t;

typedef struct sh_trampo_mgr {
    sh_trampo_page_list_t pages;
    pthread_mutex_t pages_lock;
    const char *page_name;
    size_t trampo_size;
    time_t delay_sec;
} sh_trampo_mgr_t;

void sh_trampo_init_mgr(sh_trampo_mgr_t *mem_mgr, const char *page_name, size_t trampo_size,
                        time_t delay_sec);

uintptr_t sh_trampo_alloc(sh_trampo_mgr_t *mem_mgr, uintptr_t hint, uintptr_t low_offset,
                          uintptr_t high_offset);

void sh_trampo_free(sh_trampo_mgr_t *mem_mgr, uintptr_t mem);


#endif //NATIVEHOOK_SH_TRAMPO_H
