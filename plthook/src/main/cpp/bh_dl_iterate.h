//
// Created by MOMO on 2024/1/2.
//

#ifndef NATIVEHOOK_BH_DL_ITERATE_H
#define NATIVEHOOK_BH_DL_ITERATE_H

#include <link.h>
#include <stddef.h>

int bh_dl_iterate(int (*callback)(struct dl_phdr_info *, size_t, void *), void *data);


#endif //NATIVEHOOK_BH_DL_ITERATE_H
