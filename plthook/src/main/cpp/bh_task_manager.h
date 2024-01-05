//
// Created by MOMO on 2024/1/4.
//

#ifndef NATIVEHOOK_BH_TASK_MANAGER_H
#define NATIVEHOOK_BH_TASK_MANAGER_H

#include "bh_task.h"

typedef struct bh_task_manager bh_task_manager_t;

bh_task_manager_t *bh_task_manager_create(void);

void bh_task_manager_add(bh_task_manager_t *self, bh_task_t *task);
void bh_task_manager_delete(bh_task_manager_t *self, bh_task_t *task);

void bh_task_manager_hook(bh_task_manager_t *self, bh_task_t *task);
int bh_task_manager_unhook(bh_task_manager_t *self, bh_task_t *task);

#endif //NATIVEHOOK_BH_TASK_MANAGER_H
