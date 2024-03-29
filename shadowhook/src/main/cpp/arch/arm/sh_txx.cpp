//
// Created by MOMO on 2024/3/19.
//

#include "sh_txx.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "sh_log.h"
#include "sh_util.h"

bool sh_txx_is_address_need_fix(uintptr_t address, sh_txx_rewrite_info_t *rinfo) {
    return (rinfo->start_address <= address && address < rinfo->end_address);
}

uintptr_t sh_txx_fix_address(uintptr_t address, sh_txx_rewrite_info_t *rinfo) {
    bool is_thumb = SH_UTIL_IS_THUMB(address);

    if (is_thumb) {
        address = SH_UTIL_CLEAR_BIT0(address);
    }

    if (rinfo->start_address <= address && address < rinfo->end_address) {
        uintptr_t cursor_address = rinfo->start_address;
        size_t offset = 0;
        for (size_t i = 0; i < rinfo->inst_lens_cnt; ++i) {
            if (cursor_address >= address) {
                break;
            }
            cursor_address += 2;
            offset += rinfo->inst_lens[i];
        }
        uintptr_t fixed_address = (uintptr_t) rinfo->buffer + offset;
        if (is_thumb) {
            fixed_address = SH_UTIL_SET_BIT0(fixed_address);
        }
        SH_LOG_INFO("txx rewrite: fix addr %" PRIxPTR " -> %" PRIxPTR, address, fixed_address);
        return fixed_address;
    }

    if (is_thumb) {
        address = SH_UTIL_SET_BIT0(address);
        return address;
    }
}
