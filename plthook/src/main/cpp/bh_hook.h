//
// Created by MOMO on 2024/1/4.
//

#ifndef NATIVEHOOK_BH_HOOK_H
#define NATIVEHOOK_BH_HOOK_H

#include <pthread.h>
#include <stdbool.h>

#include "queue.h"
#include "tree.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

typedef struct bh_hook_call {
    void *func;
    bool enabled;
    uint32_t task_id;
    SLIST_ENTRY(bh_hook_call, ) link;
} bh_hook_call_t;
typedef SLIST_HEAD(bh_hook_call_list, bh_hook_call, ) bh_hook_call_list_t;

typedef struct bh_hook {
    void *got_address;
    void *orig_func;
    bh_hook_call_list_t running_list;
    pthread_mutex_t running_list_lock;
    RB_ENTRY(bh_hook) link;
} bh_hook_t;

#pragma clang diagnostic pop

bh_hook_t *bh_hook_create(void *got_address, void *orig_func);
void bh_hook_destroy(bh_hook_t **self);

int bh_hook_add_func(bh_hook_t *self, void *func, uint32_t task_id);
bool bh_hook_delete_func(bh_hook_t *self, void *func);


#endif //NATIVEHOOK_BH_HOOK_H
