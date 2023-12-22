//
// Created by MOMO on 2023/12/21.
//

#include "bh_sleb128.h"

void bh_sleb128_decoder_init(bh_sleb128_decoder_t *self, uint8_t *data, size_t data_sz) {
    self->cur = data;
    self->end = data + data_sz;
}


int bh_sleb128_decoder_next(bh_sleb128_decoder_t *self, size_t *ret) {
    size_t value = 0;
    static const size_t size = 8 * sizeof(value);
    size_t shift = 0;
    uint8_t byte;

    do {
        if (self->cur >= self->end) {
            return -1;
        }
        byte = *(self->cur)++;
        value |= ((size_t) (byte & 127) << shift);
        shift +=7;
    } while (byte & 128);

    if (shift < size && (byte & 64)) {
        value |= -((size_t) (1) << shift);
    }

    *ret = value;
    return 0;
}