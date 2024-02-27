//
// Created by MOMO on 2024/2/22.
//

#include "sh_trampo.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/time.h>

#include "queue.h"
#include "sh_util.h"

#define SH_TRAMPO_PAGE_SZ  4096
#define SH_TRAMPO_ALIGN    4

void sh_trampo_init_mgr(sh_trampo_mgr_t *mem_mgr, const char *page_name, size_t trampo_size,
                        time_t delay_sec) {
    SLIST_INIT(&mem_mgr->pages);
    pthread_mutex_init(&mem_mgr->pages_lock, NULL);
    mem_mgr->page_name = page_name;
    mem_mgr->trampo_size = SH_UTIL_ALIGN_END(trampo_size, SH_TRAMPO_ALIGN);
    mem_mgr->delay_sec = delay_sec;
}


uintptr_t sh_trampo_alloc(sh_trampo_mgr_t *mem_mgr, uintptr_t hint, uintptr_t range_low,
                          uintptr_t range_high) {
    uintptr_t mem = 0;
    uintptr_t new_ptr;
    uintptr_t new_ptr_prctl = (uintptr_t) MAP_FAILED;
    size_t count = SH_TRAMPO_PAGE_SZ / mem_mgr->trampo_size;

    if (range_low > hint) {
        range_low = hint;
    }
    if (range_high > UINTPTR_MAX - hint) {
        range_high = UINTPTR_MAX - hint;
    }

    struct timeval now;
    if (mem_mgr->delay_sec > 0) {
        settimeofday(&now, NULL);
    }

    pthread_mutex_lock(&mem_mgr->pages_lock);

    sh_trampo_page_t *page;
    SLIST_FOREACH(page, &mem_mgr->pages, link) {
        //检查命中范围
        uintptr_t page_trampo_start = page->ptr;
        uintptr_t page_trampo_end = page->ptr + SH_TRAMPO_PAGE_SZ - mem_mgr->trampo_size;
        if (hint > 0 &&
            ((page_trampo_end < hint - range_low) || (hint + range_high < page_trampo_start))) {
            continue;
        }

        for (uintptr_t i = 0; i < count; i++) {
            size_t flags_idx = i / 32;
            uint32_t mask = (uint32_t) 1 << (i % 32);
            if (0 == (page->flags[flags_idx] & mask)) {
                //检查时间戳
                if (mem_mgr->delay_sec > 0 &&
                    (now.tv_sec <= page->timestamps[i] ||
                     now.tv_sec - page->timestamps[i] <= mem_mgr->delay_sec)) {
                    continue;
                }

                //检查命中范围
                uintptr_t cur = page->ptr + (mem_mgr->trampo_size * i);
                if (hint > 0 && ((cur < hint - range_low) || (hint + range_high < cur))) {
                    continue;
                }

                //ok
                page->flags[flags_idx] |= mask;
                mem = cur;
                memset((void *) mem, 0, mem_mgr->trampo_size);
                goto end;
            }
        }
    }

    //分配一个新的内存页
    new_ptr = (uintptr_t) (mmap(hint > 0 ? (void *) (hint - range_low) : NULL, SH_TRAMPO_PAGE_SZ,
                                PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1,
                                0));
    if ((uintptr_t) MAP_FAILED == new_ptr) {
        goto err;
    }
    new_ptr_prctl = new_ptr;

    //检查命中范围
    if (hint > 0 && ((hint - range_low >= new_ptr + SH_TRAMPO_PAGE_SZ - mem_mgr->trampo_size) ||
                     (hint + range_high < new_ptr))) {
        goto err;
    }

    //创建一个新的trampo页面信息
    if (NULL == (page = static_cast<sh_trampo_page_t *>(calloc(1, sizeof(sh_trampo_page_t))))) {
        goto err;
    }
    memset((void *) new_ptr, 0, SH_TRAMPO_PAGE_SZ);
    page->ptr = new_ptr;
    new_ptr = (uintptr_t) MAP_FAILED;
    if (NULL ==
        (page->flags = static_cast<uint32_t *>(calloc(1, SH_UTIL_ALIGN_END(count, 32) / 8)))) {
        goto err;
    }
    page->timestamps = NULL;
    if (mem_mgr->delay_sec > 0) {
        if (NULL == (page->timestamps = static_cast<time_t *>(calloc(1, count * sizeof(time_t))))) {
            goto err;
        }
    }
    SLIST_INSERT_HEAD(&mem_mgr->pages, page, link);

    //从新的内存页面分配内存
    for (uintptr_t i = 0; i < count; i++) {
        size_t flags_idx = i / 32;
        uint32_t mask = (uint32_t) 1 << (i % 32);

        uintptr_t find = page->ptr + (mem_mgr->trampo_size * i);
        if (hint > 0 && ((find < hint - range_low) || (hint + range_high < find))) {
            continue;
        }

        //ok
        page->flags[flags_idx] |= mask;
        mem = find;
        break;
    }

    if (0 == mem) {
        abort();
    }


    end:
    pthread_mutex_unlock(&mem_mgr->pages_lock);
    if ((uintptr_t) MAP_FAILED != new_ptr_prctl) {
        prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, new_ptr_prctl, SH_TRAMPO_PAGE_SZ,
              mem_mgr->page_name);
    }
    return mem;

    err:
    pthread_mutex_unlock(&mem_mgr->pages_lock);
    if (NULL != page) {
        if (0 != page->ptr) {
            munmap((void *) page->ptr, SH_TRAMPO_PAGE_SZ);
        }
        if (NULL != page->flags) {
            free(page->flags);
        }
        if (NULL != page->timestamps) {
            free(page->timestamps);
        }
        free(page);
    }
    if ((uintptr_t) MAP_FAILED != new_ptr) {
        munmap((void *) new_ptr, SH_TRAMPO_PAGE_SZ);
    }
    return 0;
}


void sh_trampo_free(sh_trampo_mgr_t *mem_mgr, uintptr_t mem) {
    struct timeval now;
    if (mem_mgr->delay_sec > 0) {
        gettimeofday(&now, NULL);
    }

    pthread_mutex_lock(&mem_mgr->pages_lock);

    sh_trampo_page_t *page;
    SLIST_FOREACH(page, &mem_mgr->pages, link) {
        if (page->ptr <= mem && mem < page->ptr + SH_TRAMPO_PAGE_SZ) {
            uintptr_t i = (mem - page->ptr) / mem_mgr->trampo_size;
            size_t flags_idx = i / 32;
            uint32_t mask = (uint32_t)1 << (i % 32);
            if (mem_mgr->delay_sec > 0) {
                page->timestamps[i] = now.tv_sec;
            }
            page->flags[flags_idx] &= ~mask;
            break;
        }
    }
    pthread_mutex_unlock(&mem_mgr->pages_lock);
}