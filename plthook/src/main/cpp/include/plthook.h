//
// Created by MOMO on 2023/12/19.
//

#ifndef NATIVEHOOK_PLTHOOK_H
#define NATIVEHOOK_PLTHOOK_H

#include <stdbool.h>
#include <stdint.h>

#define PLT_HOOK_VERSION "1.0.9"

#define PLT_HOOK_STATUS_CODE_OK                  0
#define PLT_HOOK_STATUS_CODE_UNINIT              1
#define PLT_HOOK_STATUS_CODE_INITERR_INVALID_ARG 2
#define PLT_HOOK_STATUS_CODE_INITERR_SYM         3
#define PLT_HOOK_STATUS_CODE_INITERR_TASK        4
#define PLT_HOOK_STATUS_CODE_INITERR_HOOK        5
#define PLT_HOOK_STATUS_CODE_INITERR_ELF         6
#define PLT_HOOK_STATUS_CODE_INITERR_ELF_REFR    7
#define PLT_HOOK_STATUS_CODE_INITERR_TRAMPO      8
#define PLT_HOOK_STATUS_CODE_INITERR_SIG         9
#define PLT_HOOK_STATUS_CODE_INITERR_DLMTR       10
#define PLT_HOOK_STATUS_CODE_INVALID_ARG         11
#define PLT_HOOK_STATUS_CODE_UNMATCH_ORIG_FUNC   12
#define PLT_HOOK_STATUS_CODE_NOSYM               13
#define PLT_HOOK_STATUS_CODE_GET_PROT            14
#define PLT_HOOK_STATUS_CODE_SET_PROT            15
#define PLT_HOOK_STATUS_CODE_SET_GOT             16
#define PLT_HOOK_STATUS_CODE_NEW_TRAMPO          17
#define PLT_HOOK_STATUS_CODE_APPEND_TRAMPO       18
#define PLT_HOOK_STATUS_CODE_GOT_VERIFY          19
#define PLT_HOOK_STATUS_CODE_REPEATED_FUNC       20
#define PLT_HOOK_STATUS_CODE_READ_ELF            21
#define PLT_HOOK_STATUS_CODE_CFI_HOOK_FAILED     22
#define PLT_HOOK_STATUS_CODE_ORIG_ADDR           23
#define PLT_HOOK_STATUS_CODE_INITERR_CFI         24
#define PLT_HOOK_STATUS_CODE_IGNORE              25
#define PLT_HOOK_STATUS_CODE_MAX                 255

#define PLT_HOOK_MODE_AUTOMATIC 0
#define PLT_HOOK_MODE_MANUAL    1


#ifdef __cplusplus
extern "C" {
#endif

    const char *plt_hook_get_version(void);

    typedef void *plt_hook_stub_t;

    typedef void (*plt_hook_hooked_t)(plt_hook_stub_t task_stub, int status_code, const char * caller_path_name,
            const char *sym_name, void *new_func, void *prev_func, void *arg);

    typedef bool (*plt_hook_caller_allow_filter_t)(const char *caller_path_name, void *arg);

    int plt_hook_init(int mode, bool debug);

    plt_hook_stub_t plt_hook_hook_single(const char *caller_path_name, const char *callee_path_name,
                                         const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                         void *hooked_arg);

    plt_hook_stub_t plt_hook_hook_partial(plt_hook_caller_allow_filter_t caller_allow_filter,
                                          void *caller_allow_filter_arg, const char *callee_path_name,
                                          const char *sym_name, void *new_func, plt_hook_hooked_t hooked,
                                          void *hooked_arg);

    plt_hook_stub_t plt_hook_hook_all(const char *callee_path_name, const char *sym_name, void *new_func,
                                      plt_hook_hooked_t hooked, void *hooked_arg);

    int plt_hook_unhook(plt_hook_stub_t stub);

    int plt_hook_add_ignore(const char *caller_path_name);

    int plt_hook_get_mode(void);
    bool plt_hook_get_debug(void);
    void plt_hook_set_debug(bool debug);
    bool plt_hook_get_recordable(void);
    void plt_hook_set_recordable(bool recordable);

#define PLT_HOOK_RECORD_ITEM_ALL             0xFF  // 0b11111111
#define PLT_HOOK_RECORD_ITEM_TIMESTAMP       (1 << 0)
#define PLT_HOOK_RECORD_ITEM_CALLER_LIB_NAME (1 << 1)
#define PLT_HOOK_RECORD_ITEM_OP              (1 << 2)
#define PLT_HOOK_RECORD_ITEM_LIB_NAME        (1 << 3)
#define PLT_HOOK_RECORD_ITEM_SYM_NAME        (1 << 4)
#define PLT_HOOK_RECORD_ITEM_NEW_ADDR        (1 << 5)
#define PLT_HOOK_RECORD_ITEM_ERRNO           (1 << 6)
#define PLT_HOOK_RECORD_ITEM_STUB            (1 << 7)

};



#endif //NATIVEHOOK_PLTHOOK_H
