//
// Created by MOMO on 2023/12/25.
//

#include "bh_elf_manager.h"


#include <android/api-level.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <link.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bh_const.h"
#include "bh_dl_iterate.h"
#include "bh_dl_monitor.h"
#include "bh_elf.h"
#include "bh_linker.h"
#include "bh_util.h"
#include "tree.h"
#include "bh_log.h"


// block list
typedef struct bh_elf_manager_block {
    char *caller_path_name;
    TAILQ_ENTRY(bh_elf_manager_block, ) link;
} bh_elf_manager_block_t;
typedef TAILQ_HEAD(bh_elf_manager_block_list, bh_elf_manager_block, ) bh_elf_manager_block_list_t;

// RB-tree for ELF info (bh_elf_t)
static __inline__ int bh_elf_cmp(bh_elf_t *a, bh_elf_t *b) {
    return strcmp(a->pathname, b->pathname);
}
typedef RB_HEAD(bh_elf_tree, bh_elf) bh_elf_tree_t;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
RB_GENERATE_STATIC(bh_elf_tree, bh_elf, link, bh_elf_cmp)
#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
struct bh_elf_manager {
    bool contain_pathname;
    bool contain_basename;
    bh_elf_tree_t elfs;
    size_t elfs_cnt;
    bh_elf_list_t abandoned_elfs;
    pthread_rwlock_t elfs_lock;
    bh_elf_manager_block_list_t blocklist;
    pthread_mutex_t blocklist_lock;
};
#pragma clang diagnostic pop


bh_elf_manager_t *bh_elf_manager_create(void) {
    bh_elf_manager_t *self;
    if (NULL == (self = static_cast<bh_elf_manager_t *>(malloc(sizeof(bh_elf_manager_t))))) {
        return NULL;
    }
    self->contain_pathname = false;
    self->contain_basename = false;
    RB_INIT(&self->elfs);
    self->elfs_cnt = 0;
    TAILQ_INIT(&self->abandoned_elfs);
    pthread_rwlock_init(&self->elfs_lock, NULL);
    TAILQ_INIT(&self->blocklist);
    pthread_mutex_init(&self->blocklist_lock, NULL);

    return self;
}

int bh_elf_manager_add_ignore(bh_elf_manager_t *self, const char *caller_path_name) {
    bh_elf_manager_block_t *block;
    if (NULL == (block = static_cast<bh_elf_manager_block_t *>(calloc(1, sizeof(bh_elf_t))))) {
        return -1;
    }
    if (NULL == (block->caller_path_name = strdup(caller_path_name))) {
        free(block);
        return -1;
    }

    bh_elf_manager_block_t *tmp;
    pthread_mutex_lock(&self->blocklist_lock);
    TAILQ_FOREACH(tmp, &self->blocklist, link) {
        if (0 == strcmp(tmp->caller_path_name, caller_path_name)) {
            break;
        }
    }
    if (NULL == tmp) {
        TAILQ_INSERT_TAIL(&self->blocklist, block, link);
        block = NULL;
    }
    pthread_mutex_unlock(&self->blocklist_lock);

    if (NULL != block) {
        free(block->caller_path_name);
        free(block);
    }

    return 0;
}

static bool bh_elf_manager_check_ignore(bh_elf_manager_t *self, const char *caller_path_name) {
    bool result = true;
    pthread_mutex_lock(&self->blocklist_lock);

    bh_elf_manager_block_t *tmp;
    TAILQ_FOREACH(tmp, &self->blocklist, link) {
        if ('/' == caller_path_name[0] && '/' != tmp->caller_path_name[0]) {
            if (bh_util_ends_with(caller_path_name, tmp->caller_path_name)) {
                goto ignore;
            }
        } else if ('/' != caller_path_name[0] && '/' == tmp->caller_path_name[0]) {
            if (bh_util_ends_with(tmp->caller_path_name, caller_path_name)) {
                goto ignore;
            }
        } else {
            if (0 == strcmp(caller_path_name, tmp->caller_path_name)) {
                goto ignore;
            }
        }
    }
    result = false;

    ignore:
    pthread_mutex_unlock(&self->blocklist_lock);
    return result;
}

static int bh_elf_manager_iterate_cb(struct dl_phdr_info *info, size_t size, void *arg) {
    (void)size;

    uintptr_t *pkg = (uintptr_t *)arg;
    bh_elf_manager_t *self = (bh_elf_manager_t *)*pkg++;
    bh_elf_list_t *new_elfs = (bh_elf_list_t *)*pkg;

    if (bh_util_ends_with(info->dlpi_name, BH_CONST_BASENAME_PLT_HOOK)) {
        return 0;
    }
    if (bh_elf_manager_check_ignore(self, info->dlpi_name)) {
        return 0;
    }

    bh_elf_t elf_key = {.pathname = info->dlpi_name};
    bh_elf_t *elf = RB_FIND(bh_elf_tree, &self->elfs, &elf_key);
    if (NULL == elf) {
        if (NULL != (elf = bh_elf_create(info))) {
            RB_INSERT(bh_elf_tree, &self->elfs, elf);
            self->elfs_cnt++;

            if (NULL != new_elfs) {
                TAILQ_INSERT_TAIL(new_elfs, elf, link_list);
            }

            if ((!self->contain_pathname) && ('/' == info->dlpi_name[0])) {
                self->contain_pathname = true;
            }
            if ((!self->contain_basename) && ('/' != info->dlpi_name[0])) {
                self->contain_basename = true;
            }

            BH_LOG_INFO("ELF manager: add %" BH_UTIL_PRIxADDR " %s", elf->load_bias, elf->pathname);
        }
    }

    if (NULL != elf) {
        bh_elf_set_exist(elf);
    }

    return 0;
}

void bh_elf_manager_refresh(bh_elf_manager_t *self, bool sync_clean, bh_elf_manager_post_add_cb_t cb,
                            void *cb_arg) {

    bh_elf_list_t new_elfs = TAILQ_HEAD_INITIALIZER(new_elfs);

    uintptr_t pkg[2];
    pkg[0] = (uintptr_t)self;
    pkg[1] = (NULL == cb ? (uintptr_t)NULL : (uintptr_t)(&new_elfs));

    if (0 != pthread_rwlock_wrlock(&self->elfs_lock)) {
        return;
    }

    bh_dl_iterate(bh_elf_manager_iterate_cb, (void *)pkg);

    bh_elf_t *elf, *elf_tmp;
    RB_FOREACH_SAFE(elf, bh_elf_tree, &self->elfs, elf_tmp) {
        if (!bh_elf_is_exist(elf)) {
            RB_REMOVE(bh_elf_tree, &self->elfs, elf);
            self->elfs_cnt--;
            TAILQ_INSERT_TAIL(&self->abandoned_elfs, elf, link_list);
        } else {
            bh_elf_unset_exist(elf);
        }
    }

    if (sync_clean) {
        TAILQ_FOREACH_SAFE(elf, &self->abandoned_elfs, link_list, elf_tmp) {
            TAILQ_REMOVE(&self->abandoned_elfs, elf, link_list);
            bh_elf_destroy(&elf);
        }
    }

    pthread_rwlock_unlock(&self->elfs_lock);

    if (NULL != cb) {
        TAILQ_FOREACH_SAFE(elf, &new_elfs, link_list, elf_tmp) {
            TAILQ_REMOVE(&new_elfs, elf, link_list);
            cb(elf, cb_arg);
        }
    }
}

void bh_elf_manager_iterate(bh_elf_manager_t *self, bh_elf_manager_iterate_cb_t cb, void *cb_arg) {
    if (0 == self->elfs_cnt) {
        return;
    }

    bh_elf_t **copy_elfs = NULL;
    size_t copy_elfs_cnt = 0;
    pthread_rwlock_rdlock(&self->elfs_lock);
    if (self->elfs_cnt > 0) {
        if (NULL != (copy_elfs = static_cast<bh_elf_t **>(malloc(
                sizeof(bh_elf_t *) * self->elfs_cnt)))) {
            copy_elfs_cnt = self->elfs_cnt;
            size_t i = 0;
            bh_elf_t *elf;
            RB_FOREACH(elf, bh_elf_tree, &self->elfs) {
                copy_elfs[i++] = elf;
            }
        }
    }
    pthread_rwlock_unlock(&self->elfs_lock);

    if (NULL != copy_elfs) {
        bool cb_next = true;
        for (size_t i = 0; i < copy_elfs_cnt; ++i) {
            if (cb_next) {
                cb_next = cb(copy_elfs[i], cb_arg);
            }
        }
        free(copy_elfs);
    }
}

bh_elf_t *bh_elf_manager_find_elf(bh_elf_manager_t *self, const char *pathname) {
    bh_elf_t *elf = NULL;

    pthread_rwlock_rdlock(&self->elfs_lock);

    if (('/' == pathname[0] && self->contain_pathname && !self->contain_basename) ||
        ('/' != pathname[0] && self->contain_basename && !self->contain_pathname)) {
        bh_elf_t elf_key = {.pathname = pathname};
        elf = RB_FIND(bh_elf_tree, &self->elfs, &elf_key);
    } else {
        RB_FOREACH(elf, bh_elf_tree, &self->elfs) {
            if (bh_elf_is_match(elf, pathname)) break;
        }
    }

    pthread_rwlock_unlock(&self->elfs_lock);
    return elf;
}

void *bh_elf_manager_find_export_address(bh_elf_manager_t *self, const char *pathname, const char *sym_name) {
    bh_elf_t *elf = bh_elf_manager_find_elf(self, pathname);
    if (NULL == elf) return NULL;

    return bh_elf_find_export_func_address_by_symbol_name(elf, sym_name);
}