//
// Created by MOMO on 2024/1/4.
//

#include "bh_hook.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bh_log.h"
#include "bh_task.h"

bh_hook_t *bh_hook_create(void *got_address, void *orig_func) {
    bh_hook_t *self;

    if (NULL == (self = static_cast<bh_hook_t *>(malloc(sizeof(bh_hook_t))))) {
        return NULL;
    }
    self->got_address = got_address;
    self->orig_func = orig_func;
    SLIST_INIT(&self->running_list);
    pthread_mutex_init(&self->running_list_lock, NULL);

    return self;
}

void bh_hook_destroy(bh_hook_t **self) {
    if (NULL == self || NULL == *self) {
        return;
    }

    pthread_mutex_destroy(&(*self)->running_list_lock);
    free(*self);
    *self = NULL;
}

int bh_hook_add_func(bh_hook_t *self, void *func, uint32_t task_id) {
    bh_hook_call_t *running;
    int r = PLT_HOOK_STATUS_CODE_OK;

    pthread_mutex_lock(&self->running_list_lock);

    // check repeated function
    SLIST_FOREACH(running, &self->running_list, link) {
        if (running->enabled && running->func == func) {
            r = PLT_HOOK_STATUS_CODE_REPEATED_FUNC;
            goto end;
        }
    }

    // try to re-enable an exists item
    SLIST_FOREACH(running, &self->running_list, link) {
        if (running->func == func && running->task_id == task_id) {
            if (!running->enabled) {
                __atomic_store_n((bool *)&running->enabled, true, __ATOMIC_SEQ_CST);
            }
            BH_LOG_INFO("hook chain: add(re-enable) func, GOT %" PRIxPTR ", func %" PRIxPTR,
                        (uintptr_t)self->got_address, (uintptr_t)func);
            goto end;
        }
    }

    // create new item
    if (NULL == (running = static_cast<bh_hook_call_t *>(malloc(sizeof(bh_hook_call_t))))) {
        r = PLT_HOOK_STATUS_CODE_APPEND_TRAMPO;
        goto end;
    }
    running->func = func;
    running->enabled = true;
    running->task_id = task_id;

    SLIST_NEXT(running, link) = SLIST_FIRST(&self->running_list);
    __atomic_store_n((uintptr_t *)(&SLIST_FIRST(&self->running_list)), (uintptr_t)running, __ATOMIC_RELEASE);

    BH_LOG_INFO("hook chain: add(new) func, GOT %" PRIxPTR ", func %" PRIxPTR, (uintptr_t)self->got_address,
                (uintptr_t)func);

    end:
    pthread_mutex_unlock(&self->running_list_lock);
    return r;
}

bool bh_hook_delete_func(bh_hook_t *self, void *func) {
    bool useful = false;

    pthread_mutex_lock(&self->running_list_lock);

    bh_hook_call_t *running;
    SLIST_FOREACH(running, &self->running_list, link) {
        if (running->func == func) {
            if (running->enabled) {
                __atomic_store_n((bool *)&running->enabled, false, __ATOMIC_SEQ_CST);
            }
            BH_LOG_INFO("hook chain: del func, GOT %" PRIxPTR ", func %" PRIxPTR, (uintptr_t)self->got_address,
                        (uintptr_t)func);
        }

        if (running->enabled && !useful) {
            useful = true;
        }
    }
    pthread_mutex_unlock(&self->running_list_lock);
    return useful;
}