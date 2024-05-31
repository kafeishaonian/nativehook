//
// Created by MOMO on 2024/5/30.
//

#include "sh_linker.h"

#include <dlfcn.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "sh_log.h"
#include "sh_recorder.h"
#include "sh_sig.h"
#include "sh_switch.h"
#include "sh_util.h"
#include "shadowhook.h"
#include "xdl.h"

#ifndef __LP64__
#define SH_LINKER_BASENAME "linker"
#else
#define SH_LINKER_BASENAME "linker64"
#endif


#define SH_LINKER_SYM_G_DL_MUTEX        "__dl__ZL10g_dl_mutex"
#define SH_LINKER_SYM_G_DL_MUTEX_U_QPR2 "__dl_g_dl_mutex"
#define SH_LINKER_SYM_DO_DLOPEN_L       "__dl__Z9do_dlopenPKciPK17android_dlextinfo"
#define SH_LINKER_SYM_DO_DLOPEN_N       "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv"
#define SH_LINKER_SYM_DO_DLOPEN_O       "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv"

static bool sh_linker_dlopen_hooked = false;

static sh_linker_post_dlopen_t sh_linker_post_dlopen;
static void *sh_linker_post_dlopen_arg;

static pthread_mutex_t *sh_linker_g_dl_mutex;
static uintptr_t sh_linker_dlopen_addr;  // save address of dlopen(==4.x) or do_dlopen(>=5.0)
static xdl_info_t sh_linker_dlopen_dlinfo;

#if defined(__arm__) && __ANDROID_API__ < __ANDROID_API_L__
static uintptr_t sh_linker_dlfcn[6];
static const char *sh_linker_dlfcn_name[6] = {"dlopen", "dlerror", "dlsym",
                                              "dladdr", "dlclose", "dl_unwind_find_exidx"};
#endif

__attribute__((constructor)) static void sh_linker_ctor(void) {
    sh_linker_dlopen_addr = (uintptr_t)dlopen;
#if defined(__arm__) && __ANDROID_API__ < __ANDROID_API_L__
    sh_linker_dlfcn[0] = (uintptr_t)dlopen;
  sh_linker_dlfcn[1] = (uintptr_t)dlerror;
  sh_linker_dlfcn[2] = (uintptr_t)dlsym;
  sh_linker_dlfcn[3] = (uintptr_t)dladdr;
  sh_linker_dlfcn[4] = (uintptr_t)dlclose;
  sh_linker_dlfcn[5] = (uintptr_t)dl_unwind_find_exidx;
#endif
}

static void *sh_linker_get_base_address(xdl_info_t *dlinfo) {
    uintptr_t vaddr_min = UINTPTR_MAX;
    for (size_t i = 0; i < dlinfo->dlpi_phnum; ++i) {
        const ElfW(Phdr) *phdr = &(dlinfo->dlpi_phdr[i]);
        if (PT_LOAD == phdr->p_type && vaddr_min > phdr->p_vaddr) {
            vaddr_min = phdr->p_vaddr;
        }
    }

    if (UINTPTR_MAX == vaddr_min) {
        return dlinfo->dli_fbase;  // should not happen
    } else {
        return (void *) ((uintptr_t) dlinfo->dli_fbase + SH_UTIL_PAGE_START(vaddr_min));
    }
}


int sh_linker_init(void);

const char *ch_linker_match_dlfcn(uintptr_t target_address);
bool sh_linker_need_to_hook_dlopen(uintptr_t target_address);

typedef void (*sh_linker_post_dlopen_t)(void *arg);
int sh_linker_hook_dlopen(sh_linker_post_dlopen_t post_dlopen, void *post_dlopen_arg);

int sh_linker_get_dlinfo_by_address(void *address, xdl_info_t *dlinfo, char *lib_name, size_t lib_name_sz,
                                    char *sym_name, size_t sym_name_sz, bool ignore_symbol_check);

int sh_linker_get_dlinfo_by_sym_name(const char *lib_name, const char *sym_name, xdl_info_t *dlinfo,
                                     char *real_lib_name, size_t real_lib_name_sz);