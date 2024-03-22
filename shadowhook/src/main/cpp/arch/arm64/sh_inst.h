//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_SH_INST_H
#define NATIVEHOOK_SH_INST_H

#include <stdbool.h>
#include <stdint.h>

#include "xdl.h"

typedef struct {
    uint32_t trampo[4];   // align 16 // length == backup_len
    uint8_t backup[16];   // align 16
    uint32_t backup_len;  // == 4 or 16
    uint32_t exit_type;
    uintptr_t exit_address;
    uint32_t exit[4];
    uintptr_t enter_address;
} sh_inst_t;

int sh_inst_hook(sh_inst_t *self, uintptr_t target_addr, xdl_info_t *dlinfo, uintptr_t new_addr,
                 uintptr_t *orig_addr, uintptr_t *orig_addr2);

int sh_inst_unhook(sh_inst_t *self, uintptr_t target_addr);


#endif //NATIVEHOOK_SH_INST_H
