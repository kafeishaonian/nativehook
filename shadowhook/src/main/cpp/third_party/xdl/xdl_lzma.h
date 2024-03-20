//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_XDL_LZMA_H
#define NATIVEHOOK_XDL_LZMA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int xdl_lzma_decompress(uint8_t *src, size_t src_size, uint8_t **dst, size_t *dst_size);

#ifdef __cplusplus
}
#endif
#endif //NATIVEHOOK_XDL_LZMA_H
