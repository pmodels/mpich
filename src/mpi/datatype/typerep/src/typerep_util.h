/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef TYPEREP_UTIL_H_INCLUDED
#define TYPEREP_UTIL_H_INCLUDED

/* This header contains macros and routines to facilitate basic datatype conversions,
 * e.g. convert integers and real numbers with different sizes and endians. It is mainly
 * used to support packing and unpacking "external32" datarep.
 */

#include "mpichconf.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

/* Relying on the common predefined macros */

#if defined(WORDS_BIGENDIAN)
#define BLENDIAN 0

#elif defined(WORDS_LITTLEENDIAN)
#define BLENDIAN 1

#else

#ifndef __BYTE_ORDER
#if defined(BYTE_ORDER) /* e.g. AIX */
#define __BYTE_ORDER BYTE_ORDER
#elif defined(__BYTE_ORDER__)   /* e.g. clang */
#define __BYTE_ORDER __BYTE_ORDER__
#endif
#endif /* __BYTE_ORDER */

#ifndef __BIG_ENDIAN
#if defined(_BIG_ENDIAN)
#define __BIG_ENDIAN _BIG_ENDIAN
#elif define(__BIG_ENDIAN__)
#define __BIG_ENDIAN __BIG_ENDIAN__
#endif
#endif /* __BIG_ENDIAN */

#if !defined(__BYTE_ORDER) || !defined(__BIG_ENDIAN)
#error This code assumes that __BYTE_ORDER and __BIG_ENDIAN are defined
#endif

#if defined(_BIG_ENDIAN) || (__BYTE_ORDER == __BIG_ENDIAN)
#define BLENDIAN 0      /* detected host arch byte order is big endian */
#else
#define BLENDIAN 1      /* detected host arch byte order is little endian */
#endif

#endif

/*
  NOTE:

  There are two 'public' calls here:

  FLOAT_convert(src, dest) -- converts floating point src into
  external32 floating point format and stores the result in dest.

  BASIC_convert(const void *src, void *dest, int size) -- converts integral
  type src into external32 integral type and stores the result in dest.

  These two macros compile to assignments on big-endian architectures.
*/

#define BASIC_convert16(src, dest)  \
{                                \
    dest = ((((src) >> 8) & 0x00FF) | \
            (((src) << 8) & 0xFF00)); \
}

#define BASIC_convert32(src, dest)      \
{                                    \
    dest = ((((src) >> 24) & 0x000000FF) |\
            (((src) >>  8) & 0x0000FF00) |\
            (((src) <<  8) & 0x00FF0000) |\
            (((src) << 24) & 0xFF000000));\
}

#define BASIC_convert64(src, dest)   \
{                                    \
    dest = ((((src) >> 56) & 0x00000000000000FFLL) |\
            (((src) >> 40) & 0x000000000000FF00LL) |\
            (((src) >> 24) & 0x0000000000FF0000LL) |\
            (((src) >>  8) & 0x00000000FF000000LL) |\
            (((src) <<  8) & 0x000000FF00000000LL) |\
            (((src) << 24) & 0x0000FF0000000000LL) |\
            (((src) << 40) & 0x00FF000000000000LL) |\
            (((src) << 56) & 0xFF00000000000000LL));\
}

static inline void BASIC_convert96(const char *src, char *dest)
{
    uint32_t tmp_src[3];
    uint32_t tmp_dest[3];

    tmp_src[0] = (uint32_t) (*((uint64_t *) src) >> 32);
    tmp_src[1] = (uint32_t) ((*((uint64_t *) src) << 32) >> 32);
    tmp_src[2] = (uint32_t)
        (*((uint32_t *) ((char *) src + sizeof(uint64_t))));

    BASIC_convert32(tmp_src[0], tmp_dest[0]);
    BASIC_convert32(tmp_src[1], tmp_dest[1]);
    BASIC_convert32(tmp_src[2], tmp_dest[2]);

    ((uint32_t *) dest)[2] = tmp_dest[0];
    ((uint32_t *) dest)[1] = tmp_dest[1];
    ((uint32_t *) dest)[0] = tmp_dest[2];
}

static inline void BASIC_convert128(const char *src, char *dest)
{
    uint64_t tmp_src[2];
    uint64_t tmp_dest[2];

    tmp_src[0] = *((uint64_t *) src);
    tmp_src[1] = *((uint64_t *) ((char *) src + sizeof(uint64_t)));

    BASIC_convert64(tmp_src[0], tmp_dest[0]);
    BASIC_convert64(tmp_src[1], tmp_dest[1]);

    ((uint64_t *) dest)[1] = tmp_dest[0];
    ((uint64_t *) dest)[0] = tmp_dest[1];
}

#if (BLENDIAN == 1)
static inline void BASIC_copyto(const void *src, void *dest, int size)
{
    for (int i = 0; i < size; i++) {
        ((char *) dest)[i] = ((const char *) src)[size - 1 - i];
    }
}

/* Note: use `uint` to suppress warnings on left-shift */
static inline void BASIC_convert(const void *src, void *dest, int size)
{
    switch (size) {
        case 1:
            *(uint8_t *) dest = *(uint8_t *) src;
            break;
        case 2:
            BASIC_convert16(*(uint16_t *) src, *(uint16_t *) dest);
            break;
        case 4:
            BASIC_convert32(*(uint32_t *) src, *(uint32_t *) dest);
            break;
        case 8:
            BASIC_convert64(*(uint64_t *) src, *(uint64_t *) dest);
            break;
        default:
            BASIC_copyto(src, dest, size);
    }
}

#else
static inline void BASIC_copyto(const void *src, void *dest, int size)
{
    memcpy(dest, src, (size_t) size);
}

/* FIXME: we may need use memcpy to allow no-alignment */
static inline void BASIC_convert(const void *src, void *dest, int size)
{
    switch (size) {
        case 1:
            *(int8_t *) dest = *(int8_t *) src;
            break;
        case 2:
            *(int16_t *) dest = *(int16_t *) src;
            break;
        case 4:
            *(int32_t *) dest = *(int32_t *) src;
            break;
        case 8:
            *(int64_t *) dest = *(int64_t *) src;
            break;
        default:
            BASIC_copyto(src, dest, size);
    }
}

#endif

/*
  Notes on the IEEE floating point format
  ---------------------------------------

  external32 for floating point types is big-endian IEEE format.

  ---------------------
  32 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_single_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:8;
  unsigned int mantissa:23;
  };

  * little endian byte order
  struct le_ieee754_single_precision
  {
  unsigned int mantissa:23;
  unsigned int exponent:8;
  unsigned int sign_neg:1;
  };
  ---------------------

  ---------------------
  64 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee754_double_precision
  {
  unsigned int sign_neg:1;
  unsigned int exponent:11;
  unsigned int mantissa0:20;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
  };
  ---------------------

  128 bit floating point implementation notes
  ===========================================

  For the IEEE ``Double Extended'' formats, MPI specifies a Format
  Width of 16 bytes, with 15 exponent bits, bias = +16383,
  112 fraction bits, and an encoding analogous to the ``Double'' format.

*/

MPI_Aint MPII_Typerep_get_basic_size_external32(MPI_Datatype el_type);
bool MPII_Typerep_basic_type_is_complex(MPI_Datatype el_type);
bool MPII_Typerep_basic_type_is_unsigned(MPI_Datatype el_type);

#endif /* TYPEREP_UTIL_H_INCLUDED */
