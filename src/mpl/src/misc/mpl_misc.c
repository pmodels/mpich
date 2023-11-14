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

/* encodes src data into hex characters */
int MPL_hex_encode(const void *src, int src_size, char *dest, int dest_size, int *encoded_size)
{
    if (dest_size < src_size * 2 + 1) {
        return MPL_ERR_FAIL;
    }

    const char *srcp = src;

    for (int i = 0; i < src_size; i++) {
        snprintf(dest, 3, "%02X", (unsigned char) *srcp);
        srcp++;
        dest += 2;
    }
    *dest = '\0';

    *encoded_size = src_size * 2 + 1;
    return MPL_SUCCESS;
}

/* return the size of the binary encoded in the src string */
int MPL_hex_decode_len(const char *src)
{
    int n = 0;
    while (isxdigit(src[0]) && isxdigit(src[1])) {
        src += 2;
        n++;
    }

    return n;
}

/* decodes hex encoded string in src into original binary data in dest */
int MPL_hex_decode(const char *src, void *dest, int dest_size, int *decoded_size)
{
    char *destp = dest;

    int n = 0;
    while (isxdigit(src[0]) && isxdigit(src[1])) {
        *destp = (char) (hex(src[0]) << 4) + hex(src[1]);
        src += 2;
        destp++;

        n++;
        if (n >= dest_size) {
            return MPL_ERR_FAIL;
        }
    }

    *decoded_size = n;
    return MPL_SUCCESS;
}
