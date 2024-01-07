//
// Created by MOMO on 2024/1/5.
//

#ifndef NATIVEHOOK_BH_DL_MONITOR_H
#define NATIVEHOOK_BH_DL_MONITOR_H

#include <stdbool.h>

#include "plthook.h"

typedef void (*bh_dl_monitor_post_dlopen_t)(void *arg);
void bh_dl_monitor_set_post_dlopen(bh_dl_monitor_post_dlopen_t cb, void *cb_arg);

typedef void (*bh_dl_monitor_post_dlclose_t)(bool sync_refresh, void *arg);
void bh_dl_monitor_set_post_dlclose(bh_dl_monitor_post_dlclose_t cb, void *cb_arg);

void bh_dl_monitor_add_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data);
void bh_dl_monitor_delete_dlopen_callback(plt_hook_pre_dlopen_t pre, plt_hook_post_dlopen_t post, void *data);

bool bh_dl_monitor_is_initing(void);
int bh_dl_monitor_init(void);
void bh_dl_monitor_uninit(void);

void bh_dl_monitor_dlclose_rdlock(void);
void bh_dl_monitor_dlclose_unlock(void);


#endif //NATIVEHOOK_BH_DL_MONITOR_H
