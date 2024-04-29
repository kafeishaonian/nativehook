//
// Created by MOMO on 2024/3/20.
//

#ifndef NATIVEHOOK_SH_EXIT_H
#define NATIVEHOOK_SH_EXIT_H

#include <stddef.h>
#include <stdint.h>

#include "xdl.h"

void sh_exit_init(void);

int sh_exit_alloc(uintptr_t *exit_addr, uint16_t *exit_type, uintptr_t pc, xdl_info_t *dlinfo, uint8_t *exit,
                  size_t exit_len, size_t range_low, size_t range_high);
int sh_exit_free(uintptr_t exit_addr, uint16_t exit_type, uint8_t *exit, size_t exit_len);

#endif //NATIVEHOOK_SH_EXIT_H
