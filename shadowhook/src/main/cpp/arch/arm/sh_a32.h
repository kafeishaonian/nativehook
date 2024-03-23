//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_SH_A32_H
#define NATIVEHOOK_SH_A32_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t overwrite_start_addr;
    uintptr_t overwrite_end_addr;
    uint32_t *rewrite_buf;
    size_t rewrite_buf_offset;
    size_t rewrite_inst_lens[2];
    size_t rewrite_inst_lens_cnt;
} sh_a32_rewrite_info_t;

size_t sh_a32_get_rewrite_inst_len(uint32_t inst);
size_t sh_a32_rewrite(uint32_t *buf, uint32_t inst, uintptr_t pc, sh_a32_rewrite_info_t *rinfo);

size_t sh_a32_absolute_jump(uint32_t *buf, uintptr_t addr);
size_t sh_a32_relative_jump(uint32_t *buf, uintptr_t addr, uintptr_t pc);


#endif //NATIVEHOOK_SH_A32_H
