//
// Created by MOMO on 2023/12/21.
//

#ifndef NATIVEHOOK_BH_SLEB128_H
#define NATIVEHOOK_BH_SLEB128_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *cur;
    uint8_t *end;
} bh_sleb128_decoder_t;


void bh_sleb128_decoder_init(bh_sleb128_decoder_t *self, uint8_t *data, size_t data_sz);
int bh_sleb128_decoder_next(bh_sleb128_decoder_t *self, size_t *ret);

#endif //NATIVEHOOK_BH_SLEB128_H
