/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_STATUS_H_INCLUDED
#define MPIR_STATUS_H_INCLUDED

/* We use bits from the "count_lo" and "count_hi_and_cancelled" fields
 * to represent the 'count' and 'cancelled' objects.  The LSB of the
 * "count_hi_and_cancelled" field represents the 'cancelled' object.
 * The 'count' object is split between the "count_lo" and
 * "count_hi_and_cancelled" fields, with the lower order bits going
 * into the "count_lo" field, and the higher order bits goin into the
 * "count_hi_and_cancelled" field.  This gives us 2N-1 bits for the
 * 'count' object, where N is the size of int.  However, the value
 * returned to the user is bounded by the definition on MPI_Count. */
/* NOTE: The below code assumes that the count value is never
 * negative.  For negative values, right-shifting can have weird
 * implementation specific consequences. */
#define MPIR_STATUS_SET_COUNT(status_, count_)                          \
    {                                                                   \
        (status_).count_lo = ((int) count_);                            \
        (status_).count_hi_and_cancelled &= 1;                          \
        (status_).count_hi_and_cancelled |= (int) (((MPIR_Ucount) count_) >> (8 * SIZEOF_INT) << 1); \
    }

#define MPIR_STATUS_GET_COUNT(status_)                                  \
    ((MPI_Count) ((((MPIR_Ucount) (((unsigned int) (status_).count_hi_and_cancelled) >> 1)) << (8 * SIZEOF_INT)) + (unsigned int) (status_).count_lo))

#define MPIR_STATUS_SET_CANCEL_BIT(status_, cancelled_) \
    {                                                   \
        (status_).count_hi_and_cancelled &= ~1;         \
        (status_).count_hi_and_cancelled |= cancelled_; \
    }

#define MPIR_STATUS_GET_CANCEL_BIT(status_) ((status_).count_hi_and_cancelled & 1)

/* Do not set MPI_ERROR (only set if ERR_IN_STATUS is returned */
#define MPIR_Status_set_empty(status_)                          \
    {                                                           \
        if ((status_) != MPI_STATUS_IGNORE)                     \
        {                                                       \
            (status_)->MPI_SOURCE = MPI_ANY_SOURCE;             \
            (status_)->MPI_TAG = MPI_ANY_TAG;                   \
            MPIR_STATUS_SET_COUNT(*(status_), 0);               \
            MPIR_STATUS_SET_CANCEL_BIT(*(status_), FALSE);      \
        }                                                       \
    }
/* See MPI 1.1, section 3.11, Null Processes */
/* Do not set MPI_ERROR (only set if ERR_IN_STATUS is returned */
#define MPIR_Status_set_procnull(status_)                       \
    {                                                           \
        if ((status_) != MPI_STATUS_IGNORE)                     \
        {                                                       \
            (status_)->MPI_SOURCE = MPI_PROC_NULL;              \
            (status_)->MPI_TAG = MPI_ANY_TAG;                   \
            MPIR_STATUS_SET_COUNT(*(status_), 0);               \
            MPIR_STATUS_SET_CANCEL_BIT(*(status_), FALSE);      \
        }                                                       \
    }

#endif /* MPIR_STATUS_H_INCLUDED */
