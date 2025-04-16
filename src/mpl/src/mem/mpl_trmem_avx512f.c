/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>

#if defined(MPL_HAVE_AVX512F)

#include <string.h>
#include <immintrin.h>

void MPL_Memcpy_stream_avx512f(void *dest, const void *src, size_t n)
{
    /* Anything less than 256 bytes is not worth optimizing */
    if (n <= 256) {
        memcpy(dest, src, n);
        return;
    }

    char *d = (char *) dest;
    const char *s = (const char *) src;

    /* Copy the first 63 bytes or less (if the address isn't 64-byte aligned) using a regular memcpy
     * to make the rest faster */
    if (((uintptr_t) d) & 63) {
        const uintptr_t t = 64 - (((uintptr_t) d) & 63);
        memcpy(d, s, t);
        d += t;
        s += t;
        n -= t;
    }

    /* Copy 512 bytes at a time by unrolling a series of 32-byte streaming copies. */
    while (n >= 512) {
        __m512i zmm0 = _mm512_loadu_si512((__m512i const *) (s + (64 * 0)));
        __m512i zmm1 = _mm512_loadu_si512((__m512i const *) (s + (64 * 1)));
        __m512i zmm2 = _mm512_loadu_si512((__m512i const *) (s + (64 * 2)));
        __m512i zmm3 = _mm512_loadu_si512((__m512i const *) (s + (64 * 3)));
        __m512i zmm4 = _mm512_loadu_si512((__m512i const *) (s + (64 * 4)));
        __m512i zmm5 = _mm512_loadu_si512((__m512i const *) (s + (64 * 5)));
        __m512i zmm6 = _mm512_loadu_si512((__m512i const *) (s + (64 * 6)));
        __m512i zmm7 = _mm512_loadu_si512((__m512i const *) (s + (64 * 7)));
        _mm512_stream_si512((__m512i *) (d + (64 * 0)), zmm0);
        _mm512_stream_si512((__m512i *) (d + (64 * 1)), zmm1);
        _mm512_stream_si512((__m512i *) (d + (64 * 2)), zmm2);
        _mm512_stream_si512((__m512i *) (d + (64 * 3)), zmm3);
        _mm512_stream_si512((__m512i *) (d + (64 * 4)), zmm4);
        _mm512_stream_si512((__m512i *) (d + (64 * 5)), zmm5);
        _mm512_stream_si512((__m512i *) (d + (64 * 6)), zmm6);
        _mm512_stream_si512((__m512i *) (d + (64 * 7)), zmm7);
        d += 512;
        s += 512;
        n -= 512;
    }
    /* Copy 256 bytes at a time by unrolling a series of 32-byte streaming copies. */
    while (n >= 256) {
        __m512i zmm0 = _mm512_loadu_si512((__m512i const *) (s + (64 * 0)));
        __m512i zmm1 = _mm512_loadu_si512((__m512i const *) (s + (64 * 1)));
        __m512i zmm2 = _mm512_loadu_si512((__m512i const *) (s + (64 * 2)));
        __m512i zmm3 = _mm512_loadu_si512((__m512i const *) (s + (64 * 3)));
        _mm512_stream_si512((__m512i *) (d + (64 * 0)), zmm0);
        _mm512_stream_si512((__m512i *) (d + (64 * 1)), zmm1);
        _mm512_stream_si512((__m512i *) (d + (64 * 2)), zmm2);
        _mm512_stream_si512((__m512i *) (d + (64 * 3)), zmm3);
        d += 256;
        s += 256;
        n -= 256;
    }

    /* Once there are fewer than 256 bytes left to be copied, copy a chunk of 128 and 64 bytes (if
     * applicable). */
    if (n >= 128) {
        __m512i zmm0 = _mm512_loadu_si512((__m512i const *) (s + (64 * 0)));
        __m512i zmm1 = _mm512_loadu_si512((__m512i const *) (s + (64 * 1)));
        _mm512_stream_si512((__m512i *) (d + (64 * 0)), zmm0);
        _mm512_stream_si512((__m512i *) (d + (64 * 1)), zmm1);
        d += 128;
        s += 128;
        n -= 128;
    }

    if (n >= 64) {
        __m512i zmm0 = _mm512_loadu_si512((__m512i const *) (s + (64 * 0)));
        _mm512_stream_si512((__m512i *) (d + (64 * 0)), zmm0);
        d += 64;
        s += 64;
        n -= 64;
    }

    /* If there is any data left, copy it using a regular memcpy */
    if (n > 0) {
        memcpy(d, s, (n & 63));
    }

    /* A memory fence is required after the streaming stores above. */
    _mm_sfence();
}

#else

void MPL_Memcpy_stream_avx512f(void *dest, const void *src, size_t n)
{
    /* stub function, should not reach here */
    assert(0);
}

#endif
