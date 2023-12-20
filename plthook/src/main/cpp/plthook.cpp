//
// Created by MOMO on 2023/12/19.
//

#include "include/plthook.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


const char *plt_hook_get_version(void) {
    return PLT_HOOK_VERSION;
}

int plt_hook_init(int mode, bool debug) {

}

