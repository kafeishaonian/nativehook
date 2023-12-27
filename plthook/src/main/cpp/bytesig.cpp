//
// Created by MOMO on 2023/12/20.
//

#include "bytesig.h"

#include <dlfcn.h>
#include <cerrno>
#include <pthread.h>
#include <csetjmp>
#include <csignal>
#include <cstddef>
#include <cstdbool>
#include <cstdlib>
#include <string>
#include <sys/syscall.h>
#include <unistd.h>

#define BYTESIG_DEBUG 0

#if BYTESIG_DEBUG
#include <android/log.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#define BYTRSIG_LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "bytesig_tag", fmt, ##__VA_ARGS__)
#pragma clang diagnostic pop
#else
#define BYTESIG_LOG(fmt, ...)
#endif

typedef enum {
    BYTESIG_STATUS_UNAVAILABLE = 0,
    BYTESIG_STATUS_SIG32 = 1,  // use sigset_t
    BYTESIG_STATUS_SIG64 = 2   // use sigset64_t
} bytesig_status_t;

static bytesig_status_t bytesig_status = BYTESIG_STATUS_UNAVAILABLE;

extern __attribute((weak)) int sigfillset64(sigset64_t *);
extern __attribute((weak)) int sigemptyset64(sigset64_t *);
extern __attribute((weak)) int sigaddset64(sigset64_t *, int);
extern __attribute((weak)) int sigismember64(const sigset64_t *, int);

typedef int (*bytesig_sigaction64_t)(int, const struct sigaction64 *, struct sigaction64 *);
typedef int (*bytesig_sigaction_t)(int, const struct sigaction *, struct sigaction *);
typedef int (*bytesig_sigprocmask64_t)(int, const sigset64_t *, sigset64_t *);
typedef int (*bytesig_sigprocmask_t)(int, const sigset_t *, sigset_t *);

static void *bytesig_sigaction;
static void *bytesig_sigprocmask;


__attribute__((constructor)) static void bytesig_ctor(void) {
    void *libc = dlopen("libc.so", RTLD_LOCAL);
    if (__predict_false(NULL == libc)) {
        return;
    }

    if (__predict_true(NULL != sigfillset64 && NULL != sigemptyset64 && NULL != sigaddset64 &&
            NULL !=sigismember64)) {
        if (__predict_true(NULL != (bytesig_sigaction = dlsym(libc, "sigaction64")) &&
                NULL != (bytesig_sigprocmask = dlsym(libc, "sigprocmask64")))) {
            bytesig_status = BYTESIG_STATUS_SIG64;
            goto end;
        }
    }
    if (__predict_true(NULL != (bytesig_sigaction = dlsym(libc, "sigaction")) &&
            NULL != (bytesig_sigprocmask = dlsym(libc, "sigprocmask")))) {
        bytesig_status = BYTESIG_STATUS_SIG32;
    }

    end:
    dlclose(libc);
}

#define BYTESIG_PROTECTED_THREADS_MAX 256

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"

typedef struct {
    pid_t tids[BYTESIG_PROTECTED_THREADS_MAX];
    sigjmp_buf *jbufs[BYTESIG_PROTECTED_THREADS_MAX];
    union {
        struct sigaction64 prev_action64;
        struct sigaction prev_action;
    };
} bytesig_signal_t;

#pragma clang diagnostic pop

static bytesig_signal_t *bytesig_signal_array[__SIGRTMIN];

static void bytesig_sigorset64(sigset64_t *dest, sigset64_t *left, sigset64_t *right) {
    sigemptyset64(dest);
    for (size_t i = 1; i < sizeof(sigset64_t) * CHAR_BIT; i++) {
        if (sigismember64(left, (int)i) == 1 || sigismember64(right, (int)i) == 1) {
            sigaddset64(dest, (int)i);
        }
    }
}

static void bytesig_sigorset(sigset_t *dest, sigset_t *left, sigset_t *right) {
    sigemptyset(dest);
    for (size_t i = 1; i < sizeof(sigset_t) * CHAR_BIT; i++) {
        if (sigismember(left, (int)i) == 1 || sigismember(right, (int) i) == 1) {
            sigaddset(dest, (int) i);
        }
    }
}

__attribute__((noinline)) static void bytesig_handler_internal(int signum, siginfo_t *siginfo, void *context) {
    bytesig_signal_t *sig = bytesig_signal_array[signum];

    pid_t tid = gettid();
    if (__predict_false(0 == tid)) {
        tid = (pid_t) syscall(SYS_gettid);
    }
    for (size_t i = 0; i < BYTESIG_PROTECTED_THREADS_MAX; ++i) {
        if (__predict_false(tid == __atomic_load_n(&(sig->tids[i]), __ATOMIC_RELAXED))) {
            BYTESIG_LOG("siglongjmp signal %d (code %d) at %zu", signum, siginfo->si_code, i);

            unsigned int ret_signum = (((unsigned int)signum & 0xFFU) << 16U);
            unsigned int ret_code = 0U;
            if (siginfo->si_code > 0) {
                ret_code = (((unsigned int)(siginfo->si_code) & 0xFFU) << 8U);
            } else if (siginfo->si_code < 0) {
                ret_code = (unsigned int)(-(siginfo->si_code)) & 0xFFU;
            }
            int ret_val = (int)(ret_signum | ret_code);

            siglongjmp(*(__atomic_load_n(&(sig->jbufs[i]), __ATOMIC_RELAXED)), ret_val);
        }
    }

#define SET_THREAD_SIGNAL_MASK(suffix)                                                              \
    do {                                                                                            \
        sigset##suffix##_t prev_mask;                                                               \
        bytesig_sigorset##suffix(&prev_mask, &(((ucontext_t *)context)->uc_sigmask##suffix),        \
                                 &(sig->prev_action##suffix.sa_mask));                              \
        if (0 == ((unsigned int)(sig->prev_action##suffix.sa_flags) & (unsigned int)SA_NODEFER)) {  \
            sigaddset##suffix(&prev_mask, signum);                                                  \
        }                                                                                           \
        sigaddset##suffix(&prev_mask, SIGPIPE);                                                     \
        sigaddset##suffix(&prev_mask, SIGUSR1);                                                     \
        sigaddset##suffix(&prev_mask, SIGQUIT);                                                     \
        ((bytesig_sigprocmask##suffix##_t)bytesig_sigprocmask)(SIG_SETMASK, &prev_mask, NULL);      \
    } while (0)

    if (BYTESIG_STATUS_SIG64 == bytesig_status) {
        SET_THREAD_SIGNAL_MASK(64);
    } else {
        SET_THREAD_SIGNAL_MASK();
    }
}

static void bytesig_handler(int signum, siginfo_t *siginfo, void *context) {
    bytesig_handler_internal(signum, siginfo, context);

#define CALL_PREVIOUS_SIGNAL_HANDLER(suffix)                                                        \
    do {                                                                                            \
        if (__predict_true(sig->prev_action##suffix.sa_flags & SA_SIGINFO)){                        \
            sig->prev_action##suffix.sa_sigaction(signum, siginfo, context);                        \
        } else if (SIG_DFL != sig->prev_action##suffix.sa_handler &&                                \
                 SIG_IGN != sig->prev_action##suffix.sa_handler) {                                  \
            sig->prev_action##suffix.sa_handler(signum);                                            \
        }                                                                                           \
    } while (0)
    bytesig_signal_t *sig = bytesig_signal_array[signum];
    if (BYTESIG_STATUS_SIG64 == bytesig_status) {
        CALL_PREVIOUS_SIGNAL_HANDLER(64);
    } else {
        CALL_PREVIOUS_SIGNAL_HANDLER();
    }
}

int bytesig_init(int signum) {
    if (__predict_false(signum <= 0 || signum >= __SIGRTMIN || signum == SIGKILL || signum == SIGSTOP)) {
        return -1;
    }
    if (__predict_false(BYTESIG_STATUS_UNAVAILABLE == bytesig_status)) {
        return -1;
    }
    if (__predict_false(NULL != bytesig_signal_array[signum])) {
        return -1;
    }

    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&lock);
    int ret = -1;
    bytesig_signal_t *sig;
    if (__predict_false(NULL != bytesig_signal_array[signum])) {
        goto end;
    }

    sig = static_cast<bytesig_signal_t *>(calloc(1, sizeof(bytesig_signal_t)));
    if (__predict_false(NULL == sig)) {
        goto end;
    }

#define SA_EXPOSE_TAGBITS 0x00000800

#define REGISTER_SIGNAL_HANDLER(suffix)                                                                         \
    do {                                                                                                        \
        struct sigaction##suffix act;                                                                           \
        memset(&act, 0, sizeof(struct sigaction##suffix));                                                      \
        sigfillset##suffix(&act.sa_mask);                                                                       \
        act.sa_sigaction = bytesig_handler;                                                                     \
        act.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART | SA_EXPOSE_TAGBITS;                                \
        if (__predict_false(                                                                                    \
                0 !=                                                                                            \
                ((bytesig_sigaction##suffix##_t)bytesig_sigaction)(signum, &act, &sig->prev_action##suffix))) { \
          free(sig);                                                                                            \
          goto end;                                                                                             \
        }                                                                                                       \
    } while (0)

    if (BYTESIG_STATUS_SIG64 == bytesig_status)
        REGISTER_SIGNAL_HANDLER(64);
    else
        REGISTER_SIGNAL_HANDLER();

    end:
    pthread_mutex_unlock(&lock);
    return ret;
}

void bytesig_protect(pid_t tid, sigjmp_buf *jbuf, const int signums[], size_t signums_cnt) {
    for (size_t i = 0; i < signums_cnt; ++i) {
        int signum = signums[i];
        if (__predict_false(signum <= 0 || signum >= __SIGRTMIN || signum == SIGKILL || signum == SIGSTOP)) {
            continue;
        }

        bytesig_signal_t *sig = bytesig_signal_array[signum];
        if (__predict_false(NULL == sig)){
            continue;
        }

        bool repeated = false;
        for (size_t j = 0; j < BYTESIG_PROTECTED_THREADS_MAX; ++j) {
            if (__predict_false(tid == sig->tids[j])){
                repeated = true;
                break;
            }
        }

        if (__predict_false(repeated)){
            continue;
        }

        size_t j = 0;
        while (1) {
            if (0 == sig->tids[j]){
                pid_t expected = 0;
                if (__atomic_compare_exchange_n(&sig->tids[j], &expected, tid, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
                    sig->jbufs[j] = jbuf;
                    BYTESIG_LOG("protect_start signal %d at %zu", signum, j);
                    break;
                }
            }
            j++;
            if (__predict_false(BYTESIG_PROTECTED_THREADS_MAX == j)) {
                j = 0;
            }
        }
    }
}


void bytesig_unprotect(pid_t tid, const int signums[], size_t signums_cnt) {
    for (size_t i = 0; i < signums_cnt; i++) {
        int signum = signums[i];
        if (__predict_false(signum <= 0 || signum >= __SIGRTMIN || signum == SIGKILL || signum == SIGSTOP)) {
            continue;
        }

        bytesig_signal_t *sig = bytesig_signal_array[signum];
        if (__predict_false(NULL == sig)) {
            continue;
        }

        for (size_t j = 0; j < BYTESIG_PROTECTED_THREADS_MAX; j++) {
            if (tid == sig->tids[j]){
                sig->jbufs[j] = NULL;
                __atomic_store_n(&sig->tids[j], 0, __ATOMIC_RELEASE);
                BYTESIG_LOG("protect_end signal %d at %zu", signum, j);
                break;
            }
        }
    }
}
