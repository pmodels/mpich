/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpl.h"
#include <assert.h>

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#ifdef MPL_HAVE_ZE

#if defined(MPL_HAVE_AVX512F)

#ifdef MPL_HAVE_X86INTRIN_H
/* must come before mpl_trmem.h */
#include <x86intrin.h>
#endif

void MPL_gpu_fast_memcpy_avx512f(void *src, void *dest, size_t size)
{
    char *d = (char *) dest;
    const char *s = (const char *) src;
    size_t n = size;

    while (n >= 64) {
        _mm512_stream_si512((__m512i *) d, _mm512_stream_load_si512((__m512i const *) s));
        d += 64;
        s += 64;
        n -= 64;
    }
    if (n & 32) {
        _mm256_stream_si256((__m256i *) d, _mm256_stream_load_si256((__m256i const *) s));
        d += 32;
        s += 32;
        n -= 32;
    }
    if (n & 16) {
        _mm_stream_si128((__m128i *) d, _mm_stream_load_si128((__m128i const *) s));
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

#else

void MPL_gpu_fast_memcpy_avx512f(void *src, void *dest, size_t size)
{
    assert(0);
}

#endif

#endif /* MPL_HAVE_ZE */
