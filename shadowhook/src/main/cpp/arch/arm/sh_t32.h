//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_SH_T32_H
#define NATIVEHOOK_SH_T32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sh_txx.h"

size_t sh_t32_get_rewrite_inst_len(uint16_t high_inst, uint16_t low_inst);
size_t sh_t32_rewrite(uint16_t *buffer, uint16_t high_inst, uint16_t low_inst, uintptr_t pc,
                      sh_txx_rewrite_info_t *rinfo);

size_t sh_t32_absolute_jump(uint16_t *buf, bool is_align4, uintptr_t address);
size_t sh_t32_relative_jump(uint16_t *buf, uintptr_t address, uintptr_t pc);

#endif //NATIVEHOOK_SH_T32_H
