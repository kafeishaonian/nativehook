//
// Created by MOMO on 2024/3/25.
//

#ifndef NATIVEHOOK_SH_HUB_H
#define NATIVEHOOK_SH_HUB_H

#include <stdbool.h>
#include <stdint.h>

int sh_hub_init(void);

typedef struct sh_hub sh_hub_t;

sh_hub_t *sh_hub_create(uintptr_t target_address, uintptr_t *trampo);

void sh_hub_destroy(sh_hub_t *self, bool with_delay);

uintptr_t sh_hub_get_orig_address(sh_hub_t *self);
uintptr_t *sh_hub_get_orig_address_address(sh_hub_t *self);

int sh_hub_add_proxy(sh_hub_t *self, uintptr_t func);
int sh_hub_delete_proxy(sh_hub_t * self, uintptr_t func, bool *have_enabled_proxy);

void *sh_hub_get_prev_func(void *func);
void sh_hub_pop_stack(void *return_address);
void sh_hub_allow_reentrant(void *return_address);
void sh_hub_disallow_reentrant(void *return_address);
void *sh_hub_get_return_address(void);



#endif //NATIVEHOOK_SH_HUB_H
