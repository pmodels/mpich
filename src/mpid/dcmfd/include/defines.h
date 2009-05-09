/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file include/defines.h
 * \brief ???
 */

#ifndef _DEFINES_H_
#define _DEFINES_H_

#include "mpido_properties.h"

#define MPIDO_ISPOF2(x)   (!(((x) - 1) & (x)) ? 1 : 0)
#define MPIDO_ISEVEN(x)   (((x) % 2) ? 0 : 1)

/* denotes the end of arguments in variadic functions */
#define MPIDO_END_ARGS -1

typedef int MPIDO_Embedded_Info_Mask;

#define MPIDO_INFO_TOTAL_BITS (8 * sizeof (MPIDO_Embedded_Info_Mask))
#define MPIDO_INFO_ELMT(d)    ((d) / MPIDO_INFO_TOTAL_BITS)
#define MPIDO_INFO_MASK(d)                                                     \
        ((MPIDO_Embedded_Info_Mask) 1 << ((d) % MPIDO_INFO_TOTAL_BITS))

/* embedded_info_set of informative bits */
typedef struct
{
  MPIDO_Embedded_Info_Mask info_bits[4]; /* initially 128 bits */
} MPIDO_Embedded_Info_Set;

# define MPIDO_INFO_BITS(set) ((set) -> info_bits)

/* unset all bits in the embedded_info_set */
#define MPIDO_INFO_ZERO(s)                                                     \
  do                                                                          \
  {                                                                           \
    unsigned int __i, __j;                                                    \
    MPIDO_Embedded_Info_Set * __arr = (s);                                     \
    __j = sizeof(MPIDO_Embedded_Info_Set) / sizeof(MPIDO_Embedded_Info_Mask);   \
    for (__i = 0; __i < __j; ++__i)                                           \
      MPIDO_INFO_BITS (__arr)[__i] = 0;                                        \
  } while (0)

#define MPIDO_INFO_OR(s, d)                                                    \
  do                                                                          \
  {                                                                           \
    unsigned int __i, __j;                                                    \
    MPIDO_Embedded_Info_Set * __src = (s);                                     \
    MPIDO_Embedded_Info_Set * __dst = (d);                                     \
    __j = sizeof(MPIDO_Embedded_Info_Set) / sizeof(MPIDO_Embedded_Info_Mask);   \
    for (__i = 0; __i < __j; ++__i)                                           \
      MPIDO_INFO_BITS (__dst)[__i] |= MPIDO_INFO_BITS (__src)[__i];             \
  } while (0)



/* sets a bit in the embedded_info_set */
#define MPIDO_INFO_SET(s, d)                                                   \
        (MPIDO_INFO_BITS (s)[MPIDO_INFO_ELMT(d)] |= MPIDO_INFO_MASK(d))

/* unsets a bit in the embedded_info_set */
#define MPIDO_INFO_UNSET(s, d)                                                 \
        (MPIDO_INFO_BITS (s)[MPIDO_INFO_ELMT(d)] &= ~MPIDO_INFO_MASK(d))

/* checks a bit in the embedded_info_set */
#define MPIDO_INFO_ISSET(s, d)                                                 \
        ((MPIDO_INFO_BITS (s)[MPIDO_INFO_ELMT(d)] & MPIDO_INFO_MASK(d)) != 0)

/* this sets multiple bits in one call */
extern void MPIDO_MSET_INFO(MPIDO_Embedded_Info_Set * set, ...);

/* this sees if bits pattern in s are in d */
extern int MPIDO_INFO_MET(MPIDO_Embedded_Info_Set *s, MPIDO_Embedded_Info_Set *d);

enum {
  MPIDO_TREE_SUPPORT        =  0,
  MPIDO_TORUS_SUPPORT       =  1,
  MPIDO_TREE_MIN_SUPPORT    =  2,
  MPIDO_NOT_SUPPORTED       = -1,
  MPIDO_SUPPORT_NOT_NEEDED  = -2
};

#endif
