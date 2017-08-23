/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef LOOPUTIL_H
#define LOOPUTIL_H

#include "mpichconf.h"

/* FIXME!!!  TODO!!!  FOO!!!  DO THIS!!!  DETECT ME!!!
 *
 * Consider using MPIU_INT64_T etc. types instead of the
 * EIGHT_BYTE_BASIC_TYPE stuff, or put #defines at the top of this file
 * assigning them in a simple manner.
 *
 * Doing that might require that we create MPIU_UINT64_T types (etc),
 * because it looks like we really want to have unsigned types for the
 * various convert functions below.
 */

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#ifdef HAVE_INT64
#define uint64_t __int64
#define uint32_t __int32
#elif defined(MPIU_INT64_T)
/* FIXME: This is necessary with some compilers or compiler settings */
#define uint64_t unsigned MPIU_INT64_T
#endif

/* FIXME: Who defines __BYTE_ORDER or __BIG_ENDIAN?  They aren't part of C */
/* FIXME: The else test assumes that the byte order is little endian, whereas
   it may simply have been undetermined.  This should instead use either
   a configure-time test (for which there are macros) or a runtime test
   and not use this non-portable check */

/* Some platforms, like AIX, use BYTE_ORDER instead of __BYTE_ORDER */
#if defined(BYTE_ORDER) && !defined(__BYTE_ORDER)
#define __BYTE_ORDER BYTE_ORDER
#endif

#if defined(WORDS_BIGENDIAN)
#define BLENDIAN 0
#elif defined(WORDS_LITTLEENDIAN)
#define BLENDIAN 1
#else
#if !defined(__BYTE_ORDER) || !defined(__BIG_ENDIAN)
#error This code assumes that __BYTE_ORDER and __BIG_ENDIAN are defined
#endif
/* FIXME: "BLENDIAN" is a non-conforming name - it could conflict with some
   other definition in a non-mpich header file */
#if ((defined(_BIG_ENDIAN) && !defined(ntohl)) || (__BYTE_ORDER == __BIG_ENDIAN))
#define BLENDIAN 0 /* detected host arch byte order is big endian */
#else
#define BLENDIAN 1 /* detected host arch byte order is little endian */
#endif
#endif

/*
  set to 1: uses manual swapping routines
            for 16/32 bit data types
  set to 0: uses system provided swapping routines
            for 16/32 bit data types
*/
#define MANUAL_BYTESWAPS 1

/*
  NOTE:

  There are two 'public' calls here:

  FLOAT_convert(src, dest) -- converts floating point src into
  external32 floating point format and stores the result in dest.

  BASIC_convert(src, dest) -- converts integral type src into
  external32 integral type and stores the result in dest.

  These two macros compile to assignments on big-endian architectures.
*/

#if (MANUAL_BYTESWAPS == 0)
#include <netinet/in.h>
#endif

#define BITSIZE_OF(type)    (sizeof(type) * CHAR_BIT)

#if (MANUAL_BYTESWAPS == 1)
#define BASIC_convert32(src, dest)      \
{                                    \
    dest = (((src >> 24) & 0x000000FF) |\
            ((src >>  8) & 0x0000FF00) |\
            ((src <<  8) & 0x00FF0000) |\
            ((src << 24) & 0xFF000000));\
}
#else
#define BASIC_convert32(src, dest)      \
{                                    \
    dest = htonl((uint32_t)src);        \
}
#endif

#if (MANUAL_BYTESWAPS == 1)
#define BASIC_convert16(src, dest)  \
{                                \
    dest = (((src >> 8) & 0x00FF) | \
            ((src << 8) & 0xFF00)); \
}
#else
#define BASIC_convert16(src, dest)  \
{                                \
    dest = htons((uint16_t)src);    \
}
#endif


/* changed the argument types to be char* instead of uint64_t* because the Sun compiler
   prints out warnings that the function expects unsigned long long, but is being passed
   signed long long in mpid_ext32_segment.c. */
static inline void BASIC_convert64(char *src, char *dest)
{
    uint32_t tmp_src[2];
    uint32_t tmp_dest[2];

    tmp_src[0] = (uint32_t)(*((uint64_t *)src) >> 32);
    tmp_src[1] = (uint32_t)((*((uint64_t *)src) << 32) >> 32);

    BASIC_convert32(tmp_src[0], tmp_dest[0]);
    BASIC_convert32(tmp_src[1], tmp_dest[1]);

    *((uint64_t *)dest) = (uint64_t)tmp_dest[0];
    *((uint64_t *)dest) <<= 32;
    *((uint64_t *)dest) |= (uint64_t)tmp_dest[1];
}

static inline void BASIC_convert96(char *src, char *dest)
{
    uint32_t tmp_src[3];
    uint32_t tmp_dest[3];
    char *ptr = dest;

    tmp_src[0] = (uint32_t)(*((uint64_t *)src) >> 32);
    tmp_src[1] = (uint32_t)((*((uint64_t *)src) << 32) >> 32);
    tmp_src[2] = (uint32_t)
        (*((uint32_t *)((char *)src + sizeof(uint64_t))));

    BASIC_convert32(tmp_src[0], tmp_dest[0]);
    BASIC_convert32(tmp_src[1], tmp_dest[1]);
    BASIC_convert32(tmp_src[2], tmp_dest[2]);

    *((uint32_t *)ptr) = tmp_dest[0];
    ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = tmp_dest[1];
    ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = tmp_dest[2];
}

static inline void BASIC_convert128(char *src, char *dest)
{
    uint64_t tmp_src[2];
    uint64_t tmp_dest[2];
    char *ptr = dest;

    tmp_src[0] = *((uint64_t *)src);
    tmp_src[1] = *((uint64_t *)((char *)src + sizeof(uint64_t)));

    BASIC_convert64((char *) &tmp_src[0], (char *) &tmp_dest[0]);
    BASIC_convert64((char *) &tmp_src[1], (char *) &tmp_dest[1]);

    *((uint64_t *)ptr) = tmp_dest[0];
    ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = tmp_dest[1];
}

#if (BLENDIAN == 1)
#define BASIC_convert(src, dest)               \
{                                           \
    register int type_byte_size = sizeof(src); \
    switch(type_byte_size)                     \
    {                                          \
        case 1:                                \
            dest = src;                        \
            break;                             \
        case 2:                                \
            BASIC_convert16(src, dest);        \
            break;                             \
        case 4:                                \
            BASIC_convert32(src, dest);        \
            break;                             \
        case 8:                                \
            BASIC_convert64((char *)&src,  \
                            (char *)&dest);\
            break;                             \
    }                                          \
}

/*
  http://www.mpi-forum.org/docs/mpi-20-html/node200.htm

  When converting a larger size integer to a smaller size integer,
  only the less significant bytes are moved. Care must be taken to
  preserve the sign bit value. This allows no conversion errors if the
  data range is within the range of the smaller size integer. ( End of
  advice to implementors.)
*/
#define BASIC_mixed_convert(src, dest)
#else
#define BASIC_convert(src, dest)               \
        { dest = src; }
#define BASIC_mixed_convert(src, dest)         \
        { dest = src; }
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
  * big endian float word order
  struct le_ieee754_double_precision
  {
  unsigned int mantissa0:20;
  unsigned int exponent:11;
  unsigned int sign_neg:1;
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

  ---------------------
  96 bit floating point
  ---------------------
  * big endian byte order
  struct be_ieee854_double_extended
  {
  unsigned int negative:1;
  unsigned int exponent:15;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * big endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  unsigned int mantissa0:32;
  unsigned int mantissa1:32;
  };

  * little endian byte order
  * little endian float word order
  struct le_ieee854_double_extended
  {
  unsigned int mantissa1:32;
  unsigned int mantissa0:32;
  unsigned int exponent:15;
  unsigned int negative:1;
  unsigned int empty:16;
  };
  ---------------------

  128 bit floating point implementation notes
  ===========================================

  "A 128-bit long double number consists of an ordered pair of
  64-bit double-precision numbers. The first member of the
  ordered pair contains the high-order part of the number, and
  the second member contains the low-order part. The value of the
  long double quantity is the sum of the two 64-bit numbers."

  From http://nscp.upenn.edu/aix4.3html/aixprggd/genprogc/128bit_long_double_floating-point_datatype.htm
  [ as of 09/04/2003 ]
*/

#if (BLENDIAN == 1)
#define FLOAT_convert(src, dest)              \
{                                          \
    register int type_byte_size = sizeof(src);\
    switch(type_byte_size)                    \
    {                                         \
        case 4:                               \
        {                                     \
           long d;                            \
           BASIC_convert32((long)src, d);     \
           dest = (float)d;                   \
        }                                     \
        break;                                \
        case 8:                               \
        {                                     \
           BASIC_convert64((char *)&src,  \
                           (char *)&dest);\
        }                                     \
        break;                                \
        case 12:                              \
        {                                     \
           BASIC_convert96((char *)&src,      \
                           (char *)&dest);    \
        }                                     \
        break;                                \
        case 16:                              \
        {                                     \
           BASIC_convert128((char *)&src,     \
                            (char *)&dest);   \
        }                                     \
        break;                                \
    }                                         \
}
#else
#define FLOAT_convert(src, dest)              \
        { dest = src; }
#endif

#ifdef HAVE_INT16_T
#define TWO_BYTE_BASIC_TYPE int16_t
#else
#if (SIZEOF_SHORT == 2)
#define TWO_BYTE_BASIC_TYPE short
#else
#error "Cannot detect a basic type that is 2 bytes long"
#endif
#endif /* HAVE_INT16_T */

#ifdef HAVE_INT32_T
#define FOUR_BYTE_BASIC_TYPE int32_t
#else
#if (SIZEOF_INT == 4)
#define FOUR_BYTE_BASIC_TYPE int
#elif (SIZEOF_LONG == 4)
#define FOUR_BYTE_BASIC_TYPE long
#else
#error "Cannot detect a basic type that is 4 bytes long"
#endif
#endif /* HAVE_INT32_T */

#ifdef HAVE_INT64_T
#define EIGHT_BYTE_BASIC_TYPE int64_t
#else
#ifdef HAVE_INT64
#define EIGHT_BYTE_BASIC_TYPE __int64
#elif (SIZEOF_LONG_LONG == 8)
#define EIGHT_BYTE_BASIC_TYPE long long
#else
#error "Cannot detect a basic type that is 8 bytes long"
#endif
#endif /* HAVE_INT64_T */

#if (SIZEOF_FLOAT == 4)
#define FOUR_BYTE_FLOAT_TYPE float
#else
#error "Cannot detect a float type that is 4 bytes long"
#endif

#if (SIZEOF_DOUBLE == 8)
#define EIGHT_BYTE_FLOAT_TYPE double
#else
#error "Cannot detect a float type that is 8 bytes long"
#endif

#endif /* LOOPUTIL_H */
