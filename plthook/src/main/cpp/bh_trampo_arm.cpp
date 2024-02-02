//
// Created by MOMO on 2024/1/5.
//

#if defined(__arm__)

#include "bh_trampo.h"

__attribute__((naked)) void bh_trampo_template(void) {
    __asm__(// Save caller-saved registers
            "push  { r0 - r3, lr }     \n"

            // Call bh_trampo_push_stack()
            "ldr   r0, .L_hook_ptr     \n"
            "mov   r1, lr              \n"
            "ldr   ip, .L_push_stack   \n"
            "blx   ip                  \n"

            // Save the hook function's address to IP register
            "mov   ip, r0              \n"

            // Restore caller-saved registers
            "pop   { r0 - r3, lr }     \n"

            // Call hook function
            "bx    ip                  \n"

            "bh_trampo_data:"
            ".global bh_trampo_data;"
            ".L_push_stack:"
            ".word 0;"
            ".L_hook_ptr:"
            ".word 0;");
}

#else
typedef int make_iso_happy;
#endif
