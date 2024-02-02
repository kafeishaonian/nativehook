//
// Created by MOMO on 2024/1/5.
//

#if defined(__i386__)

#include "bh_trampo.h"

__attribute__((naked)) void bh_trampo_template(void) {
    __asm__(
            "pushl  %ebp             \n"
            "movl   %esp, %ebp       \n"

            // the second param for bh_trampo_push_stack(): return address
            "pushl  4(%ebp)          \n"

            // the first param for bh_trampo_push_stack(): .L_hook_ptr
            "call   .L_pic_trampo    \n"
            ".L_pic_trampo:          \n"
            "popl   %ecx             \n"
            "addl   $(.L_hook_ptr - .L_pic_trampo), %ecx\n"
            "movl   (%ecx), %eax     \n"
            "pushl  %eax             \n"

            // Call bh_trampo_push_stack()
            "addl   $(.L_push_stack - .L_hook_ptr), %ecx\n"
            "movl   (%ecx), %eax     \n"
            "call   *%eax            \n"

            "movl   %ebp, %esp       \n"
            "popl   %ebp             \n"

            // Call hook function
            "jmp    *%eax\n"

            "bh_trampo_data:"
            ".global bh_trampo_data;"
            ".L_push_stack:"
            ".word 0; .word 0;"
            ".L_hook_ptr:"
            ".word 0; .word 0;");
}

#else
typedef int make_iso_happy;
#endif