//
// Created by MOMO on 2024/3/19.
//

#ifndef NATIVEHOOK_XDL_H
#define NATIVEHOOK_XDL_H

#include <dlfcn.h>
#include <link.h>
#include_next "stddef.h"

extern "C" {
typedef struct {
    // same as Dl_info:
    const char *dli_fname;  // Pathname of shared object that contains address.
    void *dli_fbase;        // Address at which shared object is loaded.
    const char *dli_sname;  // Name of nearest symbol with address lower than addr.
    void *dli_saddr;        // Exact address of symbol named in dli_sname.
    // added by xDL:
    size_t dli_ssize;             // Symbol size of nearest symbol with address lower than addr.
    const ElfW(Phdr) *dlpi_phdr;  // Pointer to array of ELF program headers for this object.
    size_t dlpi_phnum;            // Number of items in dlpi_phdr.
} xdl_info_t;

//
// Default value for flags in both xdl_open() and xdl_iterate_phdr().
//
#define XDL_DEFAULT 0x00

//
// Enhanced dlopen() / dlclose() / dlsym().
//
#define XDL_TRY_FORCE_LOAD    0x01
#define XDL_ALWAYS_FORCE_LOAD 0x02
void *xdl_open(const char *filename, int flags);
void *xdl_close(void *handle);
void *xdl_sym(void *handle, const char *symbol, size_t *symbol_size);
void *xdl_dsym(void *handle, const char *symbol, size_t *symbol_size);

//
// Enhanced dladdr().
//
int xdl_addr(void *addr, xdl_info_t *info, void **cache);
void xdl_addr_clean(void **cache);

//
// Enhanced dl_iterate_phdr().
//
#define XDL_FULL_PATHNAME 0x01
int xdl_iterate_phdr(int (*callback)(struct dl_phdr_info *, size_t, void *), void *data, int flags);

//
// Custom dlinfo().
//
#define XDL_DI_DLINFO 1  // type of info: xdl_info_t
int xdl_info(void *handle, int request, void *info);


};

#endif //NATIVEHOOK_XDL_H
