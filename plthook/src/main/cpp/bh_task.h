//
// Created by MOMO on 2024/1/4.
//

#ifndef NATIVEHOOK_BH_TASK_H
#define NATIVEHOOK_BH_TASK_H

#include <stdint.h>

#include "bh_elf.h"
#include "bh_hook.h"
#include "plthook.h"
#include "queue.h"

typedef enum {
    BH_TASK_TYPE_SINGLE = 0,
    BH_TASK_TYPE_ALL,
    BH_TASK_TYPE_PARTIAL
} bh_task_type_t;

typedef enum {
    BH_TASK_STATUS_UNFINISHED = 0,
    BH_TASK_STATUS_FINISHED,
    BH_TASK_STATUS_LONGTERM,
    BH_TASK_STATUS_UNHOOKING,
} bh_task_status_t;


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

typedef struct bh_task {
    // unique id
    uint32_t id;
    bh_task_type_t type;
    bh_task_status_t status;

    //caller
    char *caller_path_name;
    plt_hook_caller_allow_filter_t caller_allow_filter;
    void *caller_allow_filter_arg;

    //callee
    char *callee_path_name;
    void *callee_address;

    //symbol
    char *sym_name;

    //new function address;
    void *new_func;

    //callback
    plt_hook_hooked_t hooked;
    void *hooked_arg;

    //single type
    int hook_status_code;
    // manual mode
    void *manual_orig_func;

    TAILQ_ENTRY(bh_task, ) link;
} bh_task_t;
#pragma clang diagnostic pop

bh_task_t *bh_task_create_single(const char *caller_path_name, const char *callee_path_name,
                                 const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                 void *hooked_arg);

bh_task_t *bh_task_create_partial(plt_hook_caller_allow_filter_t caller_allow_filter,
                                  void *caller_allow_filter_arg, const char *callee_path_name,
                                  const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                  void *hooked_arg);


bh_task_t *bh_task_create_all(const char *callee_path_name, const char *sym_name, void *new_func,
                              plt_hook_hooked_t hooked, void *hooked_arg);


void bh_task_destroy(bh_task_t **self);

void bh_task_hook(bh_task_t *self);
void bh_task_hook_elf(bh_task_t *self, bh_elf_t *elf);
int bh_task_unhook(bh_task_t *self);

void bh_task_set_manual_orig_func(bh_task_t *self, void *orig_func);
void *bh_task_get_manual_orig_func(bh_task_t *self);

void bh_task_hooked(bh_task_t *self, int status_code, const char *caller_path_name, void *orig_func);



#endif //NATIVEHOOK_BH_TASK_H
