//
// Created by MOMO on 2023/12/21.
//

#include "bh_util.h"

#define PAGE_START(address) ((address) & (uintptr_t) PAGE_MASK)

int bh_util_set_address_protect(void *address, int protect) {
    uintptr_t start_address = PAGE_START((uintptr_t) address);
    uintptr_t end_address = PAGE_START((uintptr_t) address + sizeof(uintptr_t) -1) + PAGE_SIZE;
    size_t size = end_address - start_address;

    if (0 != mprotect((void *) start_address, size, protect)) {
        return -1;
    }
    return 0;
}

int bh_util_set_protect(void *start, void *end, int protect){
    uintptr_t start_address = PAGE_START((uintptr_t) start);
    uintptr_t end_address = PAGE_START((uintptr_t) end - 1) + PAGE_SIZE;
    size_t size = end_address - start_address;

    if (0 != mprotect((void *) start_address, size, protect)) {
        return -1;
    }
    return 0;
}

bool bh_util_starts_with(const char *str, const char *start) {
    while (*str && *str == *start) {
        str++;
        start++;
    }
    return '\0' == *start;
}

bool bh_util_ends_with(const char *str, const char *ending) {
    size_t str_len = strlen(str);
    size_t ending_len = strlen(ending);

    if (ending_len > str_len) {
        return 0;
    }

    return 0 == strcmp(str + (str_len - ending_len), ending) ? true : false;
}


size_t bh_util_trim_ending(char *start) {
    char *end = start + strlen(start);
    while (start < end && isspace((int)(*(end - 1)))) {
        end--;
        *end = '\0';
    }
    return (size_t)(end - start);
}

static int bh_util_get_api_level_from_build_prop(void) {
    char buf[128];
    int api_level = -1;

    FILE *fp = fopen("/system/build.prop", "r");
    if (NULL == fp) {
        goto end;
    }

    while (fgets(buf, sizeof(buf), fp)) {
        if (bh_util_starts_with(buf, "ro.build.version.sdk=")) {
            api_level = atoi(buf + 21);
            break;
        }
    }
    fclose(fp);

    end:
    return (api_level > 0) ? api_level : -1;
}

int bh_util_get_api_level(void) {
    static int bh_util_api_level = -1;

    if (bh_util_api_level < 0) {
        int api_level = android_get_device_api_level();
        if (api_level < 0) {
            api_level = bh_util_get_api_level_from_build_prop();
        }
        if (api_level < __ANDROID_API_J__) {
            api_level = __ANDROID_API_J__;
        }

        __atomic_store_n(&bh_util_api_level, api_level, __ATOMIC_SEQ_CST);
    }
    return bh_util_api_level;
}

int bh_util_write(int fd, const char *buf, size_t buf_len) {
    if (fd < 0) {
        return -1;
    }

    const char *ptr = buf;
    size_t nleft = buf_len;

    while (nleft > 0) {
        errno = 0;
        ssize_t nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) {
            if (nwritten < 0 && errno == EINTR) {
                nwritten = 0;
            } else {
                return -1;
            }
        }

        nleft -= (size_t)nwritten;
        ptr += nwritten;
    }

    return 0;
}

#define BH_UTIL_ISLEAP(year)((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))

#define BH_UTIL_SECS_PER_HOUR           (60 * 60)
#define BH_UTIL_SECS_PER_DAY            (BH_UTIL_SECS_PER_HOUR * 24)
#define BH_UTIL_DIV(a, b)               ((a) / (b) - ((a) % (b) < 0))
#define BH_UTIL_LEAPS_THRU_END_OF(y)    (BH_UTIL_DIV(y, 4) - BH_UTIL_DIV(y, 100) + BH_UTIL_DIV(y, 400))

static const unsigned short int bh_util_mon_yday[2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
};

//struct tm *bh_util_localtime_r(const time_t *timep, long gmtoff, struct tm *result) {
//    time_t days, rem, y;
//    const unsigned short int *ip;
//}