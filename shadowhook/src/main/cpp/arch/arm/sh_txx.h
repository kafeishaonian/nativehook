//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_SH_TXX_H
#define NATIVEHOOK_SH_TXX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uintptr_t start_address;
    uintptr_t end_address;
    uint16_t *buffer;
    size_t buffer_offset;
    size_t inst_lens[13];
    size_t inst_lens_cnt;
} sh_txx_rewrite_info_t;

bool sh_txx_is_address_need_fix(uintptr_t address, sh_txx_rewrite_info_t *rinfo);

uintptr_t sh_txx_fix_address(uintptr_t address, sh_txx_rewrite_info_t *rinfo);


#endif //NATIVEHOOK_SH_TXX_H
