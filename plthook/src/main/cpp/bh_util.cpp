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

struct tm *bh_util_localtime_r(const time_t *timep, long gmtoff, struct tm *result) {
    time_t days, rem, y;
    const unsigned short int *ip;

    if (NULL == result) {
        return NULL;
    }

    result->tm_gmtoff = gmtoff;

    days = ((*timep) / BH_UTIL_SECS_PER_DAY);
    rem = ((*timep) % BH_UTIL_SECS_PER_DAY);
    rem += gmtoff;

    while (rem < 0) {
        rem += BH_UTIL_SECS_PER_DAY;
        --days;
    }
    while (rem >= BH_UTIL_SECS_PER_DAY) {
        rem -= BH_UTIL_SECS_PER_DAY;
        ++days;
    }
    result->tm_hour = (int)(rem / BH_UTIL_SECS_PER_HOUR);
    rem %= BH_UTIL_SECS_PER_HOUR;
    result->tm_min = (int)(rem / 60);
    result->tm_sec = rem % 60;
    result->tm_wday = (4 + days) % 7;
    if (result->tm_wday < 0) {
        result->tm_wday += 7;
    }
    y = 1970;

    while (days < 0 || days >= (BH_UTIL_ISLEAP(y) ? 366 : 365)) {
        time_t yg = y + days / 365 - (days % 365 < 0);
        days -= ((yg - y) * 365 + BH_UTIL_LEAPS_THRU_END_OF(yg - 1) - BH_UTIL_LEAPS_THRU_END_OF(y - 1));
        y = yg;
    }
    result->tm_year = (int)(y - 1900);
    if (result->tm_year != y - 1900) {
        errno = EOVERFLOW;
        return NULL;
    }

    result->tm_yday = (int)days;
    ip = bh_util_mon_yday[BH_UTIL_ISLEAP(y)];
    for (y = 11; days < (long int)ip[y]; --y) {
        continue;
    }
    days -= ip[y];
    result->tm_mon = (int)y;
    result->tm_mday = (int)(days + 1);
    return result;
}

static unsigned bh_util_parse_decimal(const char *format, int *ppos) {
    const char *p = format + *ppos;
    unsigned result = 0;
    for (;;) {
        int ch = *p;
        unsigned d = (unsigned)(ch - '0');
        if (d >= 10U) {
            break;
        }
        result = result * 10 + d;
        p++;
    }
    *ppos = (int)(p - format);
    return result;
}


static void bh_util_format_unsigned(char *buf, size_t buf_size, uint64_t value, int base, int caps) {
    char *p = buf;
    char *end = buf + buf_size - 1;

    while (value) {
        unsigned d = (unsigned)(value % (uint64_t)base);
        value /= (uint64_t)base;
        if (p != end) {
            char ch;
            if (d < 10) {
                ch = '0' + (char)d;
            } else {
                ch = (caps ? 'A' : 'a') + (char)(d - 10);
            }
            *p++ = ch;
        }
    }

    if (p == buf) {
        if (p != end) {
            *p++ = '0';
        }
    }
    *p = '\0';

    size_t length = (size_t)(p - buf);
    for (size_t i = 0, j = length - 1; i < j; ++i,--j) {
        char ch = buf[i];
        buf[i] = buf[j];
        buf[j] = ch;
    }
}


static void bh_util_format_integer(char *buf, size_t buf_size, uint64_t value, char conversion) {
    int is_signed = (conversion == 'd' || conversion == 'i' || conversion == 'o');
    int base = 10;
    if (conversion == 'x' || conversion == 'X') {
        base = 16;
    } else if (conversion == 'o') {
        base = 8;
    }

    int caps = (conversion == 'X');
    if (is_signed && (int64_t)(value) < 0) {
        buf[0] = '-';
        buf += 1;
        buf_size -= 1;
        value = (uint64_t)(-(int64_t)(value));
    }
    bh_util_format_unsigned(buf, buf_size, value, base, caps);
}



typedef struct {
    size_t total;
    char *pos;
    size_t avail;
} bh_util_stream_t;


static void bh_util_stream_init(bh_util_stream_t *self, char *buffer, size_t buffer_size) {
    self->total = 0;
    self->pos = buffer;
    self->avail = buffer_size;

    if (self->avail > 0) {
        self->pos[0] = '\0';
    }
}

static size_t bh_util_stream_total(bh_util_stream_t *self) {
    return self->total;
}

static void bh_util_stream_send(bh_util_stream_t *self, const char *data, int len) {
    if (len < 0) {
        len = (int) strlen(data);
    }
    self->total += (size_t)len;

    if (self->avail <= 1) {
        return;
    }

    if ((size_t)len >= self->avail) {
        len = (int)(self->avail - 1);
    }

    memcpy(self->pos, data, (size_t)len);
    self->pos += len;
    self->pos[0] = '\0';
    self->avail -= (size_t)len;
}


static void bh_util_stream_send_repeat(bh_util_stream_t *self, char ch, int count) {
    char pad[8];
    memset(pad, ch, sizeof(pad));

    const int pad_size = (int)(sizeof(pad));
    while (count > 0) {
        int avail = count;
        if (avail > pad_size) {
            avail = pad_size;
        }
        bh_util_stream_send(self, pad, avail);
        count -= avail;
    }
}



static void bh_util_stream_vformat(bh_util_stream_t *self, const char *format, va_list args) {
    int nn = 0;

    for (;;) {
        int mm;
        int padZero = 0;
        int padLeft = 0;
        char sign = '\0';
        int width = -1;
        int prec = -1;
        size_t bytelen = sizeof(int);
        int slen;
        char buffer[32];
        char c;

        mm = nn;
        do {
            c = format[mm];
            if (c == '\0' || c == '%') {
                break;
            }
            mm++;
        } while (1);

        if (mm > nn) {
            bh_util_stream_send(self, format + nn, mm - nn);
            nn = mm;
        }

        if (c == '\0') {
            break;
        }

        nn++;

        for (;;) {
            c = format[nn++];
            if (c == '\0') {
                c = '%';
                bh_util_stream_send(self, &c, 1);
                return;
            } else if (c == '0') {
                padZero = 1;
                continue;
            } else if (c == '-') {
                padLeft = 1;
                continue;
            } else if (c == ' ' || c == '+') {
                sign = c;
                continue;
            }
            break;
        }

        if ((c >= '0' && c <= '9')) {
            nn--;
            width = (int)(bh_util_parse_decimal(format, &nn));
            c = format[nn++];
        }

        if (c == '.') {
            prec = (int)(bh_util_parse_decimal(format, &nn));
            c = format[nn++];
        }

        switch (c) {
            case 'h':
                bytelen = sizeof(short);
                if (format[nn] == 'h') {
                    bytelen = sizeof(char);
                    nn += 1;
                }
                c = format[nn++];
                break;
            case 'l':
                bytelen = sizeof(long);
                if (format[nn] == 'l') {
                    bytelen = sizeof(long long);
                    nn += 1;
                }
                c = format[nn++];
                break;
            case 'z':
                bytelen = sizeof(size_t);
                c = format[nn++];
                break;
            case 't':
                bytelen = sizeof(ptrdiff_t);
                c = format[nn++];
                break;
            default:;
        }

        const char *str = buffer;
        if (c == 's') {
            // string
            str = va_arg(args, const char *);
            if (str == NULL) {
                str = "(null)";
            }
        } else if (c == 'c') {
            // character
            // NOTE: char is promoted to int when passed through the stack
            buffer[0] = (char)(va_arg(args, int));
            buffer[1] = '\0';
        } else if (c == 'p') {
            uint64_t value = (uintptr_t)(va_arg(args, void *));
            buffer[0] = '0';
            buffer[1] = 'x';
            bh_util_format_integer(buffer + 2, sizeof(buffer) - 2, value, 'x');
        } else if (c == 'd' || c == 'i' || c == 'o' || c == 'u' || c == 'x' || c == 'X') {
            // integers - first read value from stack
            uint64_t value;
            int is_signed = (c == 'd' || c == 'i' || c == 'o');
            // NOTE: int8_t and int16_t are promoted to int when passed through the stack
            switch (bytelen) {
                case 1:
                    value = (uint8_t)(va_arg(args, int));
                    break;
                case 2:
                    value = (uint16_t)(va_arg(args, int));
                    break;
                case 4:
                    value = va_arg(args, uint32_t);
                    break;
                case 8:
                    value = va_arg(args, uint64_t);
                    break;
                default:
                    return;  // should not happen
            }

            if (is_signed) {
                int shift = (int)(64 - 8 * bytelen);
                value = (uint64_t)(((int64_t)(value << shift)) >> shift);
            }
        } else if (c == '%') {
            buffer[0] = '%';
            buffer[1] = '\0';
        } else {
            return;
        }

        slen = (int) strlen(str);
        if (sign != '\0' || prec != -1) {
            return;
        }

        if (slen < width && !padLeft) {
            char padChar = padZero ? '0' : ' ';
            bh_util_stream_send_repeat(self, padChar, width - slen);
        }

        bh_util_stream_send(self, str, slen);
        if (slen < width && padLeft) {
            char padChar = padZero ? '0' : ' ';
            bh_util_stream_send_repeat(self, padChar, width - slen);
        }
    }
}


size_t bh_util_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list args) {
    bh_util_stream_t stream;
    bh_util_stream_init(&stream, buffer, buffer_size);
    bh_util_stream_vformat(&stream, format, args);
    return bh_util_stream_total(&stream);
}

size_t bh_util_snprintf(char *buffer, size_t buffer_size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t buffer_len = bh_util_vsnprintf(buffer, buffer_size, format, args);
    va_end(args);
    return buffer_len;
}