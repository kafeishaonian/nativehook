//
// Created by MOMO on 2024/2/21.
//

#ifndef NATIVEHOOK_SH_SIG_H
#define NATIVEHOOK_SH_SIG_H

#include "sh_config.h"
#include "bytesig.h"

#define SH_SIG_TRY    BYTESIG_TRY
#define SH_SIG_CATCH  BYTESIG_CATCH
#define SH_SIG_EXIT   BYTESIG_EXIT


#define SH_SIG_TRY(...) \
  do {                  \
    if (0 == 0) {
#define SH_SIG_CATCH(...) \
  }                       \
  else {
#define SH_SIG_EXIT \
  }                 \
  }                 \
  while (0)         \
    ;


#endif //NATIVEHOOK_SH_SIG_H
