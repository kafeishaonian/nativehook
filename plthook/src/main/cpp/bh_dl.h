//
// Created by MOMO on 2023/12/26.
//

#ifndef NATIVEHOOK_BH_DL_H
#define NATIVEHOOK_BH_DL_H


void *bh_dl_open_linker(void);
void bh_dl_close(void *handle);
void *bh_dl_dsym(void *handle, const char *symbol);


#endif //NATIVEHOOK_BH_DL_H
