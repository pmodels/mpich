/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>


#if defined (MPL_HAVE_SYS_SYSINFO_H)
#include <sys/sysinfo.h>
#endif

#if defined (MPL_HAVE_UNISTD_H)
#include <unistd.h>
#endif

int MPL_get_nprocs(void)
{
#if defined (MPL_HAVE_GET_NPROCS)
    return get_nprocs();
#elif defined (MPL_HAVE_DECL__SC_NPROCESSORS_ONLN) && MPL_HAVE_DECL__SC_NPROCESSORS_ONLN
    int count = sysconf(_SC_NPROCESSORS_ONLN);
    return (count > 0) ? count : 1;
#else
    /* Neither MPL_HAVE_GET_NPROCS nor MPL_HAVE_DECL__SC_NPROCESSORS_ONLN are defined.
     * Should not reach here. */
    assert(0);
    return 1;
#endif
}

/* Simple hex encoding binary as hexadecimal string. For example,
 * a binary with 4 bytes, 0x12, 0x34, 0x56, 0x78, will be encoded
 * as ascii string "12345678". The encoded string will have string
 * length of exactly double the binary size plus a terminating "NUL".
 */

static int hex(unsigned char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else {
        assert(0);
        return -1;
    }
}

static char to_hex(unsigned char num)
{
    if (num < 10) {
        return '0' + num;
    } else {
        return 'A' + num - 10;
    }
}

/* encodes src data into hex characters */

int MPL_hex_encode(const void *src, int src_size, char *dest, int dest_size, int *encoded_size)
{
    const unsigned char *srcp = src;
    *encoded_size = 0;
    int i = 0;
    while (i < src_size) {
        if (srcp[i] == 0 && i + 1 < src_size && srcp[i] == srcp[i + 1]) {
            /* two or more consecutive 0's, encode into [#] */
            int cnt = 0;
            for (int j = i; j < src_size; j++) {
                if (srcp[j] == 0) {
                    cnt++;
                } else {
                    break;
                }
            }
            int ret = snprintf(dest, dest_size, "[%d]", cnt);
            if (ret < 0) {
                return MPL_ERR_FAIL;
            }
            dest += ret;
            dest_size -= ret;
            *encoded_size += ret;

            i += cnt;
        } else {
            /* 2 hex digits */
            if (dest_size < 3) {
                return MPL_ERR_FAIL;
            }
            dest[0] = to_hex(srcp[i] >> 4);
            dest[1] = to_hex(srcp[i] & 0xf);

            dest += 2;
            dest_size -= 2;
            *encoded_size += 2;

            i++;
        }
    }
    *dest = '\0';

    return MPL_SUCCESS;
}

/* return the size of the binary encoded in the src string */
int MPL_hex_decode_len(const char *src)
{
    int n = 0;
    while (true) {
        if (src[0] == '[') {
            /* [cnt] for consecutive 0's */
            src++;
            int cnt = atoi(src);
            n += cnt;

            while (isdigit(*src)) {
                src++;
            }
            if (*src == ']') {
                src++;
            } else {
                /* error */
                return 0;
            }
        } else if (isxdigit(src[0]) && isxdigit(src[1])) {
            /* 2 hex digits for a single byte */
            n++;
            src += 2;
        } else {
            break;
        }
    }

    return n;
}

/* decodes hex encoded string in src into original binary data in dest */
int MPL_hex_decode(const char *src, void *dest, int dest_size, int *decoded_size)
{
    unsigned char *destp = dest;

    int n = 0;
    while (true) {
        if (src[0] == '[') {
            /* [cnt] for consecutive 0's */
            src++;
            int cnt = atoi(src);
            n += cnt;
            if (n > dest_size) {
                return MPL_ERR_FAIL;
            }
            if (dest_size >= cnt) {
                for (int i = 0; i < cnt; i++) {
                    destp[i] = 0;
                }
            }
            destp += cnt;

            while (isdigit(*src)) {
                src++;
            }
            if (*src == ']') {
                src++;
            } else {
                /* error */
                return 0;
            }
        } else if (isxdigit(src[0]) && isxdigit(src[1])) {
            /* 2 hex digits for a single byte */
            *destp = (char) (hex(src[0]) << 4) + hex(src[1]);
            src += 2;
            destp++;

            n++;
            if (n > dest_size) {
                return MPL_ERR_FAIL;
            }
        } else {
            break;
        }
    }

    *decoded_size = n;
    return MPL_SUCCESS;
}
