/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>

#if defined(MPL_HAVE_AVX)

#ifdef MPL_USE_MEMORY_TRACING
#undef malloc
#undef free
#endif

#include <string.h>
#include <immintrin.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"

void MPL_Memcpy_stream_avx(void *dest, const void *src, size_t n)
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

    /* Copy 256 bytes at a time by unrolling a series of 32-byte streaming copies. */
    while (n >= 256) {
        __m256i ymm0 = _mm256_loadu_si256((void const *) (s + (32 * 0)));
        __m256i ymm1 = _mm256_loadu_si256((void const *) (s + (32 * 1)));
        __m256i ymm2 = _mm256_loadu_si256((void const *) (s + (32 * 2)));
        __m256i ymm3 = _mm256_loadu_si256((void const *) (s + (32 * 3)));
        __m256i ymm4 = _mm256_loadu_si256((void const *) (s + (32 * 4)));
        __m256i ymm5 = _mm256_loadu_si256((void const *) (s + (32 * 5)));
        __m256i ymm6 = _mm256_loadu_si256((void const *) (s + (32 * 6)));
        __m256i ymm7 = _mm256_loadu_si256((void const *) (s + (32 * 7)));
        _mm256_stream_si256((void *) (d + (32 * 0)), ymm0);
        _mm256_stream_si256((void *) (d + (32 * 1)), ymm1);
        _mm256_stream_si256((void *) (d + (32 * 2)), ymm2);
        _mm256_stream_si256((void *) (d + (32 * 3)), ymm3);
        _mm256_stream_si256((void *) (d + (32 * 4)), ymm4);
        _mm256_stream_si256((void *) (d + (32 * 5)), ymm5);
        _mm256_stream_si256((void *) (d + (32 * 6)), ymm6);
        _mm256_stream_si256((void *) (d + (32 * 7)), ymm7);
        d += 256;
        s += 256;
        n -= 256;
    }

    /* Once there are fewer than 256 bytes left to be copied, copy a chunk of 128 and 64 bytes (if
     * applicable). */
    if (n >= 128) {
        __m256i ymm0 = _mm256_loadu_si256((void const *) (s + (32 * 0)));
        __m256i ymm1 = _mm256_loadu_si256((void const *) (s + (32 * 1)));
        __m256i ymm2 = _mm256_loadu_si256((void const *) (s + (32 * 2)));
        __m256i ymm3 = _mm256_loadu_si256((void const *) (s + (32 * 3)));
        _mm256_stream_si256((void *) (d + (32 * 0)), ymm0);
        _mm256_stream_si256((void *) (d + (32 * 1)), ymm1);
        _mm256_stream_si256((void *) (d + (32 * 2)), ymm2);
        _mm256_stream_si256((void *) (d + (32 * 3)), ymm3);
        d += 128;
        s += 128;
        n -= 128;
    }

    if (n >= 64) {
        __m256i ymm0 = _mm256_loadu_si256((void const *) (s + (32 * 0)));
        __m256i ymm1 = _mm256_loadu_si256((void const *) (s + (32 * 1)));
        _mm256_stream_si256((void *) (d + (32 * 0)), ymm0);
        _mm256_stream_si256((void *) (d + (32 * 1)), ymm1);
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

void MPL_Memcpy_stream_dev_avx(void *dest, const void *src, size_t n)
{
    char *d = (char *) dest;
    const char *s = (const char *) src;

    /* fallback to MPL_Memcpy_stream if not 64-byte aligned */
    if (((uintptr_t) s) & 63 || ((uintptr_t) d) & 63) {
        MPL_Memcpy_stream_avx(d, s, n);
        return;
    }

    while (n >= 32) {
        _mm256_storeu_si256((__m256i *) d, _mm256_stream_load_si256((void const *) s));
        d += 32;
        s += 32;
        n -= 32;
    }
    if (n & 16) {
        _mm_stream_si128((__m128i *) d, _mm_stream_load_si128((void *) s));
        d += 16;
        s += 16;
        n -= 16;
    }
    if (n & 8) {
        *(int64_t *) d = *(int64_t *) s;
        d += 8;
        s += 8;
        n -= 8;
    }
    if (n & 4) {
        *(int *) d = *(int *) s;
        d += 4;
        s += 4;
        n -= 4;
    }
    if (n & 2) {
        *(int16_t *) d = *(int16_t *) s;
        d += 2;
        s += 2;
        n -= 2;
    }
    if (n == 1) {
        *(char *) d = *(char *) s;
    }
    _mm_sfence();
}

#pragma GCC diagnostic pop

#else

void MPL_Memcpy_stream_avx(void *dest, const void *src, size_t n)
{
    /* stub function, should not reach here */
    assert(0);
}

void MPL_Memcpy_stream_dev_avx(void *dest, const void *src, size_t n)
{
    assert(0);
}

#endif
