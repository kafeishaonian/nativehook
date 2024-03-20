//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_XDL_LINKER_H
#define NATIVEHOOK_XDL_LINKER_H

#ifdef __cplusplus
extern "C" {
#endif

void xdl_linker_lock(void);
void xdl_linker_unlock(void);

void *xdl_linker_force_dlopen(const char *filename);

#ifdef __cplusplus
}
#endif

#endif //NATIVEHOOK_XDL_LINKER_H
