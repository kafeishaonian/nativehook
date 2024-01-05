//
// Created by MOMO on 2024/1/4.
//

#ifndef NATIVEHOOK_BH_HOOK_MANAGER_H
#define NATIVEHOOK_BH_HOOK_MANAGER_H

#include <pthread.h>
#include <stdbool.h>

#include "bh_elf.h"
#include "bh_task.h"
#include "queue.h"

typedef struct bh_hook_manager bh_hook_manager_t;

bh_hook_manager_t *bh_hook_manager_create(void);

void bh_hook_manager_hook(bh_hook_manager_t *self, bh_task_t *task, bh_elf_t *caller_elf);
void bh_hook_manager_unhook(bh_hook_manager_t *self, bh_task_t *task, bh_elf_t *caller_elf);


#endif //NATIVEHOOK_BH_HOOK_MANAGER_H
