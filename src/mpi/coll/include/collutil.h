/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef COLLUTIL_H_INCLUDED
#define COLLUTIL_H_INCLUDED

/* Returns nearest thr nearest power of two of a number*/
static inline int MPIU_pof2(int number)
{
    int pof2 = 1;

    while(pof2 <= number)
        pof2 <<= 1;
    pof2 >>= 1;

    return pof2;
}

/* Returns non-zero if val is a power of two.  If ceil_pof2 is non-NULL, it sets
   *ceil_pof2 to the power of two that is just larger than or equal to val.
   That is, it rounds up to the nearest power of two. */
static inline int MPIU_is_pof2(int val, int *ceil_pof2)
{
    int pof2 = 1;

    while (pof2 < val)
        pof2 *= 2;
    if (ceil_pof2)
        *ceil_pof2 = pof2;

    if (pof2 == val)
        return 1;
    else
        return 0;
}

/* TODO with a modest amount of work in the handle allocator code we should be
 * able to encode commutativity in the handle value and greatly simplify this
 * routine */
/* returns TRUE iff the given op is commutative */
static inline int MPIR_Op_is_commutative(MPI_Op op)
{
    MPIR_Op *op_ptr;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        return TRUE;
    }
    else {
        MPIR_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            return FALSE;
        else
            return TRUE;
    }
}

/* Implements the "mirror permutation" of "bits" low-order bits of an integer "x".
 *
 * positions 76543210, bits==3 yields 76543012.
 */
ATTRIBUTE((const)) /* tells the compiler that this func only depends on its args
                      and may be optimized much more aggressively, similar to "pure" */
static inline int MPIU_Mirror_permutation(unsigned int x, int bits)
{
    /* a mask for the high order bits that should be copied as-is */
    int high_mask = ~((0x1 << bits) - 1);
    int retval = x & high_mask;
    int i;

    for (i = 0; i < bits; ++i) {
        unsigned int bitval = (x & (0x1 << i)) >> i; /* 0x1 or 0x0 */
        retval |= bitval << ((bits - i) - 1);
    }

    return retval;
}

#endif /* !defined(COLLUTIL_H_INCLUDED) */
