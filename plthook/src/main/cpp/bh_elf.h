//
// Created by MOMO on 2023/12/21.
//

#ifndef NATIVEHOOK_BH_ELF_H
#define NATIVEHOOK_BH_ELF_H

#include <elf.h>
#include <link.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "queue.h"
#include "tree.h"

#if defined(__LP64__)
typedef ElfW(Rela) Elf_Reloc;
#else
typedef ElfW(Rel) Elf_Reloc;
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

//程序头列表
typedef struct bh_elf_ph{
    uintptr_t start;
    uintptr_t end;
    int protect;
    SLIST_ENTRY(bh_elf_ph, ) link;
} bh_elf_ph_t;

typedef SLIST_HEAD(bh_elf_ph_list, bh_elf_ph, ) bh_elf_ph_list_t;

// ELF信息
typedef struct bh_elf {
    bool exist;
    pthread_mutex_t hook_lock;
    bool error;
#ifdef __LP64__
    bool cfi_hooked;
    bool cfi_hooked_ok;
    pthread_mutex_t cfi_hook_lock;
#endif

    const char *pathname;
    uintptr_t load_bias;
    const ElfW(Phdr) *dlpi_phdr;
    size_t dlpi_phnum;
    bool dyn_parsed;
    pthread_mutex_t dyn_parse_lock;

    const Elf_Reloc *rel_plt;  //.rel.plt / .rela.plt  函数重定位表
    size_t rel_plt_cnt;

    const Elf_Reloc *rel_dyn; //.rel.dyn / .rela.dyn 数据重定位表
    size_t rel_dyn_cnt;

    uint8_t *rel_dyn_aps2;  //.rel.dyn / .rela.dyn 数据重定位表。APS2格式
    size_t rel_dyn_aps2_sz;

    ElfW(Sym) *dynsym;   //.dynsym (dynamic symbol-table, symbol-index -> string-table's offset)
    const char *dynstr;  //.dynstr (dynamic string-table)

    //.hash (SYSV hash for string-table)
    struct {
        const uint32_t *buckets;
        uint32_t buckets_cnt;
        const uint32_t *chains;
        uint32_t chains_cnt;
    } sysv_hash;

    //.gnu.hash (GNU hash for string-table)
    struct {
        const uint32_t *buckets;
        uint32_t buckets_cnt;
        const uint32_t *chains;
        uint32_t symoffset;
        const ElfW(Addr) *bloom;
        uint32_t bloom_cnt;
        uint32_t bloom_shift;
    } gnu_hash;

    RB_ENTRY(bh_elf) link;
    TAILQ_ENTRY(bh_elf, ) link_list;
} bh_elf_t;

typedef TAILQ_HEAD(bh_elf_list, bh_elf, ) bh_elf_list_t;

#pragma clang diagnostic pop

bh_elf_t *bh_elf_create(struct dl_phdr_info *info);
void bh_elf_destroy(bh_elf_t **self);

bool bh_elf_is_match(bh_elf_t *self, const char *name);

bool bh_elf_get_error(bh_elf_t *self);
void bh_elf_set_error(bh_elf_t *self, bool error);

#ifdef __LP64__
void bh_elf_cfi_hook_lock(bh_elf_t *self);
void bh_elf_cfi_hook_unlock(bh_elf_t *self);
#endif

void bh_elf_hook_lock(bh_elf_t *self);
void bh_elf_hook_unlock(bh_elf_t *self);

void bh_elf_set_exist(bh_elf_t *self);
void bh_elf_unset_exist(bh_elf_t *self);
bool bh_elf_is_exist(bh_elf_t *self);

//通过地址获取保护信息
int bh_elf_get_protect_by_address(bh_elf_t *self, void *address);

//根据符号名称查找导出函数符号信息
//signal-safe
ElfW(Sym) *bh_elf_find_export_func_symbol_by_symbol_name(bh_elf_t *self, const char *sym_name);

//通过符号名称形式.rel查找导入函数地址的地址列表。rel.PLT和 rel.dyn
size_t bh_elf_find_import_func_address_by_symbol_name(bh_elf_t *self, const char *sym_name, void **address_array,
                                                      size_t address_array_cap);

//通过被调用地址表单查找导入函数地址的地址列表.rel.PLT和 rel.dyn
size_t bh_elf_find_import_func_address_by_callee_address(bh_elf_t *self, void *target_address, void **address_array,
                                                         size_t address_array_cap);

//根据符号名查找导出函数地址(等于dlsym())
void *bh_elf_find_export_func_address_by_symbol_name(bh_elf_t *self, const char *sym_name);


#endif //NATIVEHOOK_BH_ELF_H
