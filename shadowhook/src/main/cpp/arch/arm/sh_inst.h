//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_SH_INST_H
#define NATIVEHOOK_SH_INST_H

#include <stdint.h>
#include "xdl.h"

typedef struct {
    uint32_t trampo[4];
    uint8_t backup[16];
    uint16_t backup_len;
    uint16_t exit_type;
    uintptr_t exit_address;
    uint32_t exit[2];
    uintptr_t enter_address;
} sh_inst_t;

int sh_inst_hook(sh_inst_t *self, uintptr_t target_address, xdl_info_t *dlinfo, uintptr_t new_address,
                 uintptr_t *orig_address, uintptr_t *orig_address2);

int sh_inst_unhook(sh_inst_t *self, uintptr_t target_address);

#endif //NATIVEHOOK_SH_INST_H
