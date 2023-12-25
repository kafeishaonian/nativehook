//
// Created by MOMO on 2023/12/25.
//

#ifndef NATIVEHOOK_BH_CONST_H
#define NATIVEHOOK_BH_CONST_H

#ifndef __LP64__
#define BH_CONST_PATHNAME_LINKER       "/system/bin/linker"
#define BH_CONST_BASENAME_LINKER       "linker"
#define BH_CONST_BASENAME_APP_PROCESS  "app_process32"
#else
#define BH_CONST_PATHNAME_LINKER      "/system/bin/linker64"
#define BH_CONST_BASENAME_LINKER      "linker64"
#define BH_CONST_BASENAME_APP_PROCESS "app_process64"
#endif

#define BH_CONST_BASENAME_DL           "libdl.so"
#define BH_CONST_BASENAME_PLT_HOOK     "libplthook.so"

#define BH_CONST_SYM_DLCLOSE                   "dlclose"
#define BH_CONST_SYM_LOADER_DLCLOSE            "__loader_dlclose"
#define BH_CONST_SYM_DLOPEN                    "dlopen"
#define BH_CONST_SYM_ANDROID_DLOPEN_EXT        "android_dlopen_ext"
#define BH_CONST_SYM_DLOPEN_EXT                "__dl__ZL10dlopen_extPKciPK17android_dlextinfoPv"
#define BH_CONST_SYM_LOADER_DLOPEN             "__loader_dlopen"
#define BH_CONST_SYM_LOADER_ANDROID_DLOPEN_EXT "__loader_android_dlopen_ext"
#define BH_CONST_SYM_DO_DLOPEN                 "__dl__Z9do_dlopenPKciPK17android_dlextinfoPv"
#define BH_CONST_SYM_G_DL_MUTEX                "__dl__ZL10g_dl_mutex"
#define BH_CONST_SYM_LINKER_GET_ERROR_BUFFER   "__dl__Z23linker_get_error_bufferv"
#define BH_CONST_SYM_BIONIC_FORMAT_DLERROR     "__dl__ZL23__bionic_format_dlerrorPKcS0_"
#define BH_CONST_SYM_CFI_SLOWPATH              "__cfi_slowpath"
#define BH_CONST_SYM_CFI_SLOWPATH_DIAG         "__cfi_slowpath_diag"


#endif //NATIVEHOOK_BH_CONST_H
