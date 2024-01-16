//
// Created by MOMO on 2024/1/4.
//

#include "bh_task_manager.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "bh_core.h"
#include "bh_dl_monitor.h"
#include "bh_elf_manager.h"
#include "bh_log.h"
#include "bh_task.h"
#include "queue.h"

typedef TAILQ_HEAD(bh_task_queue, bh_task, ) bh_task_queue_t;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct bh_task_manager {
    bh_task_queue_t tasks;
    pthread_rwlock_t lock;
};
#pragma clang diagnostic pop



bh_task_manager_t *bh_task_manager_create(void) {
    bh_task_manager_t *self = static_cast<bh_task_manager_t *>(malloc(sizeof(bh_task_manager_t)));
    if (NULL == self) {
        return NULL;
    }

    TAILQ_INIT(&self->tasks);
    pthread_rwlock_init(&self->lock, NULL);
    return self;
}

void bh_task_manager_add(bh_task_manager_t *self, bh_task_t *task) {
    pthread_rwlock_wrlock(&self->lock);
    TAILQ_INSERT_TAIL(&self->tasks, task, link);
    pthread_rwlock_unlock(&self->lock);
}

void bh_task_manager_delete(bh_task_manager_t *self, bh_task_t *task) {
    pthread_rwlock_wrlock(&self->lock);
    TAILQ_REMOVE(&self->tasks, task, link);
    pthread_rwlock_unlock(&self->lock);
}


void bh_task_manager_hook(bh_task_manager_t *self, bh_task_t *task) {

}

int bh_task_manager_unhook(bh_task_manager_t *self, bh_task_t *task) {

}