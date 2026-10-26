/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPL_FLOAT_H_INCLUDED
#define MPL_FLOAT_H_INCLUDED

#include "mplconfig.h"

/* *INDENT-ON* */
#if defined(__cplusplus)
extern "C" {
#endif
/* *INDENT-OFF* */

/* Software conversion between the native long double and IEEE 754 binary128
 * in big-endian byte order (e.g. the MPI external32 representation of long
 * double), for compilers without a native binary128 type. It is available,
 * and MPL_HAVE_LDOUBLE_BINARY128_CONV is defined, when long double is
 *
 *   - IEEE 754 binary64 (MPL_LDBL_MANT_DIG == 53), e.g. macOS arm64, or
 *   - x87 80-bit extended precision (MPL_LDBL_MANT_DIG == 64 on x86), e.g.
 *     macOS x86_64.
 *
 * Widening to binary128 is exact. Narrowing from binary128 rounds to nearest,
 * ties to even, overflows to infinity and underflows to subnormals or zero.
 * NaNs keep their sign and the leading bits of their payload, and become
 * quiet NaNs, as with a hardware conversion. Neither buffer needs to be
 * aligned. */

#if defined(MPL_LDBL_MANT_DIG) && \
    (MPL_LDBL_MANT_DIG == 53 || \
     (MPL_LDBL_MANT_DIG == 64 && (defined(__x86_64__) || defined(__i386__) || \
                                  defined(_M_X64) || defined(_M_IX86))))
#define MPL_HAVE_LDOUBLE_BINARY128_CONV 1

/* src: a native long double; dest: 16 bytes of big-endian binary128 */
void MPL_ldouble_to_binary128_be(const void *src, void *dest);

/* src: 16 bytes of big-endian binary128; dest: a native long double */
void MPL_binary128_be_to_ldouble(const void *src, void *dest);
#endif

/* *INDENT-ON* */
#if defined(__cplusplus)
}
#endif
/* *INDENT-OFF* */

#endif /* MPL_FLOAT_H_INCLUDED */
