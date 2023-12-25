//
// Created by MOMO on 2023/12/25.
//

#ifndef NATIVEHOOK_BH_ELF_MANAGER_H
#define NATIVEHOOK_BH_ELF_MANAGER_H

#include <stdbool.h>

#include "bh_elf.h"

typedef struct bh_elf_manager bh_elf_manager_t;

bh_elf_manager_t *bh_elf_manager_create(void);

int bh_elf_manager_add_ignore(bh_elf_manager_t *self, const char *caller_path_name);

typedef void (*bh_elf_manager_post_add_cb_t)(bh_elf_t *elf, void *arg);
void bh_elf_manager_refresh(bh_elf_manager_t *self, bool sync_clean, bh_elf_manager_post_add_cb_t cb, void *cb_arg);

typedef bool (*bh_elf_manager_iterate_cb_t)(bh_elf_t *elf, void *arg);
void bh_elf_manager_iterate(bh_elf_manager_t *self, bh_elf_manager_iterate_cb_t cb, void *cb_arg);

bh_elf_t *bh_elf_manager_find_elf(bh_elf_manager_t *self, const char *pathname);

void *bh_elf_manager_find_export_address(bh_elf_manager_t *self, const char *pathname, const char *sym_name);


#endif //NATIVEHOOK_BH_ELF_MANAGER_H
