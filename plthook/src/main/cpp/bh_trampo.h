//
// Created by MOMO on 2024/1/2.
//

#ifndef NATIVEHOOK_BH_TRAMPO_H
#define NATIVEHOOK_BH_TRAMPO_H

#include "bh_hook.h"

extern void *bh_trampo_data;

void bh_trampo_template(void);

int bh_trampo_init(void);

void *bh_trampo_create(bh_hook_t *hook);

void *bh_trampo_get_prev_func(void *func);

void bh_trampo_pop_stack(void *return_address);

void *bh_trampo_get_return_address(void);


#endif //NATIVEHOOK_BH_TRAMPO_H
