//
// Created by MOMO on 2023/12/25.
//

#include "bh_cfi.h"

#if defined(__aarch64__)

#include <dlfcn.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "bh_util.h"
#include "bytesig.h"

#define BH_CFI_LIB_DL         "libdl.so"
#define BH_CFI_SLOWPATH       "__cfi_slowpath"
#define BH_CFI_SLOWPATH_DIAG  "__cfi_slowpath_diag"
#define BH_CFI_ARM64_RET_INST 0xd65f03c0


static void *bh_cfi_slowpath = NULL;
static void *bh_cfi_slowpath_diag = NULL;

__attribute__((constructor)) static void bh_cfi_ctor(void) {
    void *handle = dlopen(BH_CFI_LIB_DL, RTLD_NOW);
    if (NULL != handle) {
        bh_cfi_slowpath = dlsym(handle, BH_CFI_SLOWPATH);
        bh_cfi_slowpath_diag = dlsym(handle, BH_CFI_SLOWPATH_DIAG);
        dlclose(handle);
    }
}


int bh_cfi_disable_slowpath(void) {
    if (bh_util_get_api_level() < __ANDROID_API_O__) {
        return 0;
    }

    if (NULL == bh_cfi_slowpath || NULL == bh_cfi_slowpath_diag) {
        return -1;
    }

    void *start = bh_cfi_slowpath <= bh_cfi_slowpath_diag ? bh_cfi_slowpath : bh_cfi_slowpath_diag;
    void *end = bh_cfi_slowpath <= bh_cfi_slowpath_diag ? bh_cfi_slowpath_diag : bh_cfi_slowpath;

    if (0 != bh_util_set_protect(start, (void *)((uintptr_t)end + sizeof(uint32_t)), PROT_READ | PROT_WRITE | PROT_EXEC)) {
        return -1;
    }

    BYTESIG_TRY(SIGSEGV, SIGBUS) {
        *((uint32_t *)bh_cfi_slowpath) = BH_CFI_ARM64_RET_INST;
        *((uint32_t *)bh_cfi_slowpath_diag) = BH_CFI_ARM64_RET_INST;
    }

    BYTESIG_CATCH() {
        return -1;
    }

    BYTESIG_EXIT

    __builtin___clear_cache(static_cast<char *>(start),
                            static_cast<char *>((void *) ((size_t) end + sizeof(uint32_t))));

    return 0;
}
#else

int bh_cfi_disable_slowpath(void) {
    return 0;
}

#endif