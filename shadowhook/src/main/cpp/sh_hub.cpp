//
// Created by MOMO on 2024/3/25.
//

#include "sh_hub.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "queue.h"
#include "sh_log.h"
#include "sh_safe.h"
#include "sh_sig.h"
#include "sh_trampo.h"
#include "sh_util.h"
#include "shadowhook.h"
#include "tree.h"


#define SH_HUB_TRAMPO_PAGE_NAME "shadowhook-hub-trampo"
#define SH_HUB_TRAMPO_DELAY_SEC 5
#define SH_HUB_STACK_NAME       "shadowhook-hub-stack"
#define SH_HUB_STACK_SIZE       4096
#define SH_HUB_STACK_FRAME_MAX  127  // keep sizeof(sh_hub_stack_t) < 4K
#define SH_HUB_THREAD_MAX       1024
#define SH_HUB_DELAY_SEC        10

#define SH_HUB_FRAME_FLAG_NONE            ((uintptr_t)0)
#define SH_HUB_FRAME_FLAG_ALLOW_REENTRANT ((uintptr_t)(1 << 0))

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
typedef struct sh_hub_proxy {
    void *func;
    bool enabled;
    SLIST_ENTRY(sh_hub_proxy,) link;
} sh_hub_proxy_t;
#pragma clang diagnostic pop

typedef SLIST_HEAD(sh_hub_proxy_list, sh_hub_proxy,) sh_hub_proxy_list_t;

// frame in the stack
typedef struct {
    sh_hub_proxy_list_t proxies;
    uintptr_t orig_address;
    void *return_address;
    uintptr_t flags;
} sh_hub_frame_t;

// stack for each thread
typedef struct {
    size_t frames_cnt;
    sh_hub_frame_t frames[SH_HUB_STACK_FRAME_MAX];
} sh_hub_stack_t;


// hub for each target-address
struct sh_hub {
    sh_hub_proxy_list_t proxies;
    pthread_mutex_t proxies_lock;
    uintptr_t orig_address;
    uintptr_t trampo;
    time_t destroy_ts;
    LIST_ENTRY(sh_hub,) link;
};


// hub list for delayed-destroy staging
typedef LIST_HEAD(sh_hub_list, sh_hub,) sh_hub_list_t;

// global data for hub delayed-destroy staging
static sh_hub_list_t sh_hub_delayed_destroy;
static pthread_mutex_t sh_hub_delayed_destroy_lock;

// global data for trampo
static sh_trampo_mgr_t sh_hub_trampo_mgr;

// global data for stack
static pthread_key_t sh_hub_stack_tls_key;
static sh_hub_stack_t *sh_hub_stack_cache;
static uint8_t *sh_hub_stack_cache_used;


// hub trampoline template
extern void *sh_hub_trampo_template_data __attribute__((visibility("hidden")));

__attribute__((naked)) static void sh_hub_trampo_template(void) {
#if defined(__arm__)
    __asm__(
      // Save parameter registers, LR
      "push  { r0 - r3, lr }     \n"

      // Call sh_hub_push_stack()
      "ldr   r0, hub_ptr         \n"
      "mov   r1, lr              \n"
      "ldr   ip, push_stack      \n"
      "blx   ip                  \n"

      // Save the hook function's address to IP register
      "mov   ip, r0              \n"

      // Restore parameter registers, LR
      "pop   { r0 - r3, lr }     \n"

      // Call hook function
      "bx    ip                  \n"

      "sh_hub_trampo_template_data:"
      ".global sh_hub_trampo_template_data;"
      "push_stack:"
      ".word 0;"
      "hub_ptr:"
      ".word 0;");
#elif defined(__aarch64__)
    __asm__(
      // Save parameter registers, XR(X8), LR
      "stp   x0, x1, [sp, #-0xd0]!    \n"
      "stp   x2, x3, [sp, #0x10]      \n"
      "stp   x4, x5, [sp, #0x20]      \n"
      "stp   x6, x7, [sp, #0x30]      \n"
      "stp   x8, lr, [sp, #0x40]      \n"
      "stp   q0, q1, [sp, #0x50]      \n"
      "stp   q2, q3, [sp, #0x70]      \n"
      "stp   q4, q5, [sp, #0x90]      \n"
      "stp   q6, q7, [sp, #0xb0]      \n"

      // Call sh_hub_push_stack()
      "ldr   x0, hub_ptr              \n"
      "mov   x1, lr                   \n"
      "ldr   x16, push_stack          \n"
      "blr   x16                      \n"

      // Save the hook function's address to IP register
      "mov   x16, x0                  \n"

      // Restore parameter registers, XR(X8), LR
      "ldp   q6, q7, [sp, #0xb0]      \n"
      "ldp   q4, q5, [sp, #0x90]      \n"
      "ldp   q2, q3, [sp, #0x70]      \n"
      "ldp   q0, q1, [sp, #0x50]      \n"
      "ldp   x8, lr, [sp, #0x40]      \n"
      "ldp   x6, x7, [sp, #0x30]      \n"
      "ldp   x4, x5, [sp, #0x20]      \n"
      "ldp   x2, x3, [sp, #0x10]      \n"
      "ldp   x0, x1, [sp], #0xd0      \n"

      // Call hook function
      "br    x16                      \n"

      "sh_hub_trampo_template_data:"
      ".global sh_hub_trampo_template_data;"
      "push_stack:"
      ".quad 0;"
      "hub_ptr:"
      ".quad 0;");
#endif
}

static void *sh_hub_trampo_template_start(void) {
#if defined(__arm__) && defined(__thumb__)
    return (void *)((uintptr_t)&sh_hub_trampo_template - 1);
#else
    return (void *) &sh_hub_trampo_template;
#endif
}

static sh_hub_stack_t *sh_hub_stack_create(void) {
    for (size_t i = 0; i < SH_HUB_THREAD_MAX; ++i) {
        uint8_t *used = &(sh_hub_stack_cache_used[i]);
        if (0 == *used) {
            uint8_t expected = 0;
            if (__atomic_compare_exchange_n(used, &expected, 1, false, __ATOMIC_ACQUIRE,
                                            __ATOMIC_RELAXED)) {
                sh_hub_stack_t *stack = &(sh_hub_stack_cache[i]);
                stack->frames_cnt = 0;
                SH_LOG_DEBUG("hub: get stack from global cache[%zu] %p", i, (void *) stack);
                return stack;  // OK
            }
        }
    }

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    void *buf = sh_safe_mmap(NULL, SH_HUB_STACK_SIZE, prot, flags, -1, 0);
    if (MAP_FAILED == buf) {
        return NULL;
    }
    sh_safe_prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (unsigned long) buf, SH_HUB_STACK_SIZE,
                  (unsigned long) SH_HUB_STACK_NAME);
    sh_hub_stack_t *stack = (sh_hub_stack_t *) buf;
    stack->frames_cnt = 0;
    return stack;
}

static void sh_hub_stack_destroy(void *buf) {
    if (NULL == buf) {
        return;
    }

    if ((uintptr_t) sh_hub_stack_cache <= (uintptr_t) buf &&
        (uintptr_t) buf <
        ((uintptr_t) sh_hub_stack_cache + SH_HUB_THREAD_MAX * sizeof(sh_hub_stack_t))) {

        size_t i = ((uintptr_t) buf - (uintptr_t) sh_hub_stack_cache) / sizeof(sh_hub_stack_t);
        uint8_t *used = &(sh_hub_stack_cache_used[i]);
        if (1 != *used) {
            abort();
        }
        __atomic_store_n(used, 0, __ATOMIC_RELEASE);
        SH_LOG_DEBUG("hub: return stack to global cache[%zu] %p", i, buf);
    } else {
        munmap(buf, SH_HUB_STACK_SIZE);
    }
}

int sh_hub_init(void) {
    LIST_INIT(&sh_hub_delayed_destroy);
    pthread_mutex_init(&sh_hub_delayed_destroy_lock, NULL);

    // init TLS key
    if (__predict_false(0 != pthread_key_create(&sh_hub_stack_tls_key, sh_hub_stack_destroy))) {
        return -1;
    }

    // init hub's stack cache
    if (__predict_false(NULL == (sh_hub_stack_cache = static_cast<sh_hub_stack_t *>(malloc(
            SH_HUB_THREAD_MAX * sizeof(sh_hub_stack_t)))))) {
        return -1;
    }
    if (__predict_false(NULL == (sh_hub_stack_cache_used = static_cast<uint8_t *>(calloc(
            SH_HUB_THREAD_MAX, sizeof(uint8_t)))))) {
        return -1;
    }

    // init hub's trampoline manager
    size_t code_size = (uintptr_t) (&sh_hub_trampo_template_data) -
                       (uintptr_t) (sh_hub_trampo_template_start());
    size_t data_size = sizeof(void *) + sizeof(void *);
    sh_trampo_init_mgr(&sh_hub_trampo_mgr, SH_HUB_TRAMPO_PAGE_NAME, code_size + data_size,
                       SH_HUB_TRAMPO_DELAY_SEC);

    return 0;
}

static void *sh_hub_push_stack(sh_hub_t *self, void *return_address) {
    sh_hub_stack_t *stack = (sh_hub_stack_t *) sh_safe_pthread_getspecific(sh_hub_stack_tls_key);

    bool recursive = false;
    if (__predict_false(NULL == stack)) {
        if (__predict_false(NULL == (stack = sh_hub_stack_create()))) {
            goto end;
        }
        sh_safe_pthread_setspecific(sh_hub_stack_tls_key, (void *) stack);
    }

    // check whether a recursive call occurred
    for (size_t i = stack->frames_cnt; i > 0; i--) {
        sh_hub_frame_t *frame = &stack->frames[i - 1];
        if (0 == (frame->flags & SH_HUB_FRAME_FLAG_ALLOW_REENTRANT) &&
            (frame->orig_address == self->orig_address)) {
            // recursive call found
            recursive = true;
            break;
        }
    }

    // find and return the first enabled proxy's function in the proxy-list
    // (does not include the original function)
    if (!recursive) {
        sh_hub_proxy_t *proxy;
        SLIST_FOREACH(proxy, &self->proxies, link) {
            if (proxy->enabled) {
                // push a new frame for the current proxy
                if (stack->frames_cnt >= SH_HUB_STACK_FRAME_MAX) goto end;
                stack->frames_cnt++;
                SH_LOG_DEBUG("hub: frames_cnt++ = %zu", stack->frames_cnt);
                sh_hub_frame_t *frame = &stack->frames[stack->frames_cnt - 1];
                frame->proxies = self->proxies;
                frame->orig_address = self->orig_address;
                frame->return_address = return_address;
                frame->flags = SH_HUB_FRAME_FLAG_NONE;

                // return the first enabled proxy's function
                SH_LOG_DEBUG("hub: push_stack() return first enabled proxy %p", proxy->func);
                return proxy->func;
            }
        }
    }

    end:
    SH_LOG_DEBUG("hub: push_stack() return orig_addr %p", (void *) self->orig_addr);
    return (void *) self->orig_address;
}


sh_hub_t *sh_hub_create(uintptr_t target_address, uintptr_t *trampo) {
    size_t code_size = (uintptr_t) (&sh_hub_trampo_template_data) -
                       (uintptr_t) (sh_hub_trampo_template_start());
    size_t data_size = sizeof(void *) + sizeof(void *);

    sh_hub_t *self = static_cast<sh_hub_t *>(malloc(sizeof(sh_hub_t)));
    if (NULL == self) return NULL;
    SLIST_INIT(&self->proxies);
    pthread_mutex_init(&self->proxies_lock, NULL);
    self->orig_address = 0;

    // alloc memory for trampoline
    if (0 == (self->trampo = sh_trampo_alloc(&sh_hub_trampo_mgr, 0, 0, 0))) {
        free(self);
        return NULL;
    }
    
    // fill in code
    SH_SIG_TRY(SIGSEGV, SIGBUS)
            {
                memcpy((void *) self->trampo, sh_hub_trampo_template_start(), code_size);
            }
        SH_SIG_CATCH()
            {
                sh_trampo_free(&sh_hub_trampo_mgr, self->trampo);
                free(self);
                SH_LOG_WARN("hub: fill in code crashed");
                return NULL;
            }
            SH_SIG_EXIT

    // file in data
    void **data = (void **) (self->trampo + code_size);
    *data++ = (void *) sh_hub_push_stack;
    *data = (void *) self;

    // clear CPU cache
    sh_util_clear_cache(self->trampo, code_size + data_size);

#if defined(__arm__) && defined(__thumb__)
    *trampo = self->trampo + 1;
#else
    *trampo = self->trampo;
#endif

    SH_LOG_INFO(
            "hub: create trampo for target_addr %" PRIxPTR " at %" PRIxPTR ", size %zu + %zu = %zu",
            target_address, *trampo, code_size, data_size, code_size + data_size);
    return self;
}

static void sh_hub_destroy_inner(sh_hub_t *self) {
    pthread_mutex_destroy(&self->proxies_lock);

    if (0 != self->trampo) {
        sh_trampo_free(&sh_hub_trampo_mgr, self->trampo);
    }

    while (!SLIST_EMPTY(&self->proxies)) {
        sh_hub_proxy_t *proxy = SLIST_FIRST(&self->proxies);
        SLIST_REMOVE_HEAD(&self->proxies, link);
        free(proxy);
    }

    free(self);
}

void sh_hub_destroy(sh_hub_t *self, bool with_delay) {
    if (SHADOWHOOK_IS_SHARED_MODE) {
        struct timeval now;
    }
}

uintptr_t sh_hub_get_orig_address(sh_hub_t *self);

uintptr_t *sh_hub_get_orig_address_address(sh_hub_t *self);

int sh_hub_add_proxy(sh_hub_t *self, uintptr_t func);

int sh_hub_delete_proxy(sh_hub_t *self, uintptr_t func, bool *have_enabled_proxy);

void *sh_hub_get_prev_func(void *func);

void sh_hub_pop_stack(void *return_address) {
    sh_hub_stack_t *stack = (sh_hub_stack_t *) sh_safe_pthread_getspecific(sh_hub_stack_tls_key);
    if (0 == stack->frames_cnt) {
        return;
    }
    sh_hub_frame_t *frame = &stack->frames[stack->frames_cnt - 1];

    if (frame->return_address == return_address) {
        stack->frames_cnt--;
        SH_LOG_DEBUG("hub: frames_cnt-- = %zu", stack->frames_cnt);
    }
}

void sh_hub_allow_reentrant(void *return_address);

void sh_hub_disallow_reentrant(void *return_address);

void *sh_hub_get_return_address(void);

