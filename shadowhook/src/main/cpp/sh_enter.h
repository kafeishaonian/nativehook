//
// Created by MOMO on 2024/3/20.
//

#ifndef NATIVEHOOK_SH_ENTER_H
#define NATIVEHOOK_SH_ENTER_H

#include <stdint.h>

int sh_enter_init(void);

uintptr_t sh_enter_alloc(void);
void sh_enter_free(uintptr_t enter);

#endif //NATIVEHOOK_SH_ENTER_H
