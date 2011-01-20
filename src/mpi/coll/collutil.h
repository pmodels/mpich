/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef COLLUTIL_H_INCLUDED
#define COLLUTIL_H_INCLUDED

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
    MPID_Op *op_ptr;

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        return TRUE;
    }
    else {
        MPID_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPID_OP_USER_NONCOMMUTE)
            return FALSE;
        else
            return TRUE;
    }
}

#endif /* !defined(COLLUTIL_H_INCLUDED) */
