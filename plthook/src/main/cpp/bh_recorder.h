//
// Created by MOMO on 2023/12/27.
//

#ifndef NATIVEHOOK_BH_RECORDER_H
#define NATIVEHOOK_BH_RECORDER_H

#include <stdbool.h>
#include <stdint.h>

bool bh_recorder_get_recordable(void );
void bh_recorder_set_recordable(bool recordable);

int bh_recorder_add_hook(int error_number, const char *lib_name, const char *sym_name, uintptr_t new_addr,
                         uintptr_t stub, uintptr_t caller_addr);
int bh_recorder_add_unhook(int error_number, uintptr_t stub, uintptr_t caller_addr);

char *bh_recorder_get(uint32_t item_flags);
void bh_recorder_dump(int fd, uint32_t item_flags);

#endif //NATIVEHOOK_BH_RECORDER_H
