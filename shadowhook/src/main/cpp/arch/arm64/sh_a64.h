//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_SH_A64_H
#define NATIVEHOOK_SH_A64_H

#include <stddef.h>
#include <stdint.h>

#include "sh_inst.h"

typedef struct {
    uintptr_t start_addr;
    uintptr_t end_addr;
    uint32_t *buf;
    size_t buf_offset;
    size_t inst_lens[4];
    size_t inst_lens_cnt;
} sh_a64_rewrite_info_t;

size_t sh_a64_get_rewrite_inst_len(uint32_t inst);

size_t sh_a64_rewrite(uint32_t *buf, uint32_t inst, uintptr_t pc, sh_a64_rewrite_info_t *rinfo);

size_t sh_a64_absolute_jump_with_br(uint32_t *buf, uintptr_t addr);

size_t sh_a64_absolute_jump_with_ret(uint32_t *buf, uintptr_t addr);

size_t sh_a64_relative_jump(uint32_t *buf, uintptr_t addr, uintptr_t pc);

#endif //NATIVEHOOK_SH_A64_H
