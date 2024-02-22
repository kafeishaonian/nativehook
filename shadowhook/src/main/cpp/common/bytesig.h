//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_BYTESIG_H
#define NATIVEHOOK_BYTESIG_H

#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <sys/syscall.h>
#include <unistd.h>

#define BYTESIG_TRY(...)                                                                                   \
  do {                                                                                                     \
    pid_t _bytesig_tid_ = gettid();                                                                        \
    if (0 == _bytesig_tid_) _bytesig_tid_ = (pid_t)syscall(SYS_gettid);                                    \
    sigjmp_buf _bytesig_jbuf_;                                                                             \
    int _bytesig_sigs_[] = {__VA_ARGS__};                                                                  \
    bytesig_protect(_bytesig_tid_, &_bytesig_jbuf_, _bytesig_sigs_, sizeof(_bytesig_sigs_) / sizeof(int)); \
    int _bytesig_protected_ = 1;                                                                           \
    int _bytesig_ex_ = sigsetjmp(_bytesig_jbuf_, 1);                                                       \
    if (0 == _bytesig_ex_) {
#define BYTESIG_CATCH_2(signum_, code_)                                                     \
  }                                                                                         \
  else {                                                                                    \
    bytesig_unprotect(_bytesig_tid_, _bytesig_sigs_, sizeof(_bytesig_sigs_) / sizeof(int)); \
    _bytesig_protected_ = 0;                                                                \
    int signum_ = (int)(((unsigned int)_bytesig_ex_ & 0xFF0000U) >> 16U);                   \
    int code_ = 0;                                                                          \
    if (((unsigned int)_bytesig_ex_ & 0xFF00U) > 0)                                         \
      code_ = (int)(((unsigned int)_bytesig_ex_ & 0xFF00U) >> 8U);                          \
    else if (((unsigned int)_bytesig_ex_ & 0xFFU) > 0)                                      \
      code_ = -((int)((unsigned int)_bytesig_ex_ & 0xFFU));                                 \
    (void)signum_;                                                                          \
    (void)code_;

#define BYTESIG_CATCH_1(signum_) BYTESIG_CATCH_2(signum_, _bytesig_code_)
#define BYTESIG_CATCH_0()        BYTESIG_CATCH_1(_bytesig_signum_)

#define FUNC_CHOOSER(_f1, _f2, _f3, ...)     _f3
#define FUNC_RECOMPOSER(argsWithParentheses) FUNC_CHOOSER argsWithParentheses
#define CHOOSE_FROM_ARG_COUNT(...)           FUNC_RECOMPOSER((__VA_ARGS__, BYTESIG_CATCH_2, BYTESIG_CATCH_1, ))
#define NO_ARG_EXPANDER()                    , , BYTESIG_CATCH_0
#define MACRO_CHOOSER(...)                   CHOOSE_FROM_ARG_COUNT(NO_ARG_EXPANDER __VA_ARGS__())

#define BYTESIG_CATCH(...) MACRO_CHOOSER(__VA_ARGS__)(__VA_ARGS__)

#define BYTESIG_EXIT                                                                        \
  }                                                                                         \
  if (1 == _bytesig_protected_)                                                             \
    bytesig_unprotect(_bytesig_tid_, _bytesig_sigs_, sizeof(_bytesig_sigs_) / sizeof(int)); \
  }                                                                                         \
  while (0)                                                                                 \
    ;

#ifdef __cplusplus
extern "C" {
#endif

int bytesig_init(int signum);

void bytesig_protect(pid_t tid, sigjmp_buf *jbuf, const int signums[], size_t signums_cnt);
void bytesig_unprotect(pid_t tid, const int signums[], size_t signums_cnt);

#ifdef __cplusplus
}
#endif

#endif //NATIVEHOOK_BYTESIG_H
