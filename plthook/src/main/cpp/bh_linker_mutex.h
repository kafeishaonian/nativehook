//
// Created by MOMO on 2023/12/27.
//

#ifndef NATIVEHOOK_BH_LINKER_MUTEX_H
#define NATIVEHOOK_BH_LINKER_MUTEX_H

#include <pthread.h>

pthread_mutex_t *bh_linker_mutex_get_by_stack(void);


#endif //NATIVEHOOK_BH_LINKER_MUTEX_H
