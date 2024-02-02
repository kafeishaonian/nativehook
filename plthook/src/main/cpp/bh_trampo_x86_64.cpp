//
// Created by MOMO on 2024/1/5.
//

#if defined(__x86_64__)

#include "bh_trampo.h"

__attribute__((naked)) void bh_trampo_template(void) {
  __asm__(
      "pushq   %rbp                      \n"
      "movq    %rsp, %rbp                \n"

      // Save caller-saved registers
      "subq    $192,  %rsp               \n"
      "movupd  %xmm0, 176(%rsp)          \n"
      "movupd  %xmm1, 160(%rsp)          \n"
      "movupd  %xmm2, 144(%rsp)          \n"
      "movupd  %xmm3, 128(%rsp)          \n"
      "movupd  %xmm4, 112(%rsp)          \n"
      "movupd  %xmm5,  96(%rsp)          \n"
      "movupd  %xmm6,  80(%rsp)          \n"
      "movupd  %xmm7,  64(%rsp)          \n"
      "movq    %rax,   56(%rsp)          \n"
      "movq    %rdi,   48(%rsp)          \n"
      "movq    %rsi,   40(%rsp)          \n"
      "movq    %rdx,   32(%rsp)          \n"
      "movq    %rcx,   24(%rsp)          \n"
      "movq    %r8,    16(%rsp)          \n"
      "movq    %r9,     8(%rsp)          \n"
      "movq    %r10,     (%rsp)          \n"

      // Call bh_trampo_push_stack()
      "movq    .L_hook_ptr(%rip), %rdi   \n"
      "movq    8(%rbp), %rsi             \n"
      "call    *.L_push_stack(%rip)      \n"

      // Save the hook function's address to IP register
      "movq    %rax, %r11                \n"

      // Restore caller-saved registers
      "movupd  176(%rsp), %xmm0          \n"
      "movupd  160(%rsp), %xmm1          \n"
      "movupd  144(%rsp), %xmm2          \n"
      "movupd  128(%rsp), %xmm3          \n"
      "movupd  112(%rsp), %xmm4          \n"
      "movupd   96(%rsp), %xmm5          \n"
      "movupd   80(%rsp), %xmm6          \n"
      "movupd   64(%rsp), %xmm7          \n"
      "movq     56(%rsp), %rax           \n"
      "movq     48(%rsp), %rdi           \n"
      "movq     40(%rsp), %rsi           \n"
      "movq     32(%rsp), %rdx           \n"
      "movq     24(%rsp), %rcx           \n"
      "movq     16(%rsp), %r8            \n"
      "movq      8(%rsp), %r9            \n"
      "movq       (%rsp), %r10           \n"
      "addq    $192,      %rsp           \n"

      "movq    %rbp, %rsp                \n"
      "popq    %rbp                      \n"

      // Call hook function
      "jmp     *%r11                     \n"

      "bh_trampo_data:"
      ".global bh_trampo_data;"
      ".L_push_stack:"
      ".quad 0;"
      ".L_hook_ptr:"
      ".quad 0;");
}

#else
typedef int make_iso_happy;
#endif