/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_POINTER_H_INCLUDED)
#define MPIU_POINTER_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"
#include "mpichconfconst.h"
#include "mpidbg.h"
#include "mpiassert.h"

/* Assigns (src_) to (dst_), checking that (src_) fits in (dst_) without
 * truncation.
 *
 * When fiddling with this macro, please keep C's overly complicated integer
 * promotion/truncation/conversion rules in mind.  A discussion of these issues
 * can be found in Chapter 5 of "Secure Coding in C and C++" by Robert Seacord.
 */
#define MPIU_Assign_trunc(dst_,src_,dst_type_)                                         \
    do {                                                                               \
        /* will catch some of the cases if the expr_inttype macros aren't available */ \
        MPIU_Assert((src_) == (dst_type_)(src_));                                      \
        dst_ = (dst_type_)(src_);                                                      \
    } while (0)

/*
 * Ensure an MPI_Aint value fits into a signed int.
 * Useful for detecting overflow when MPI_Aint is larger than an int.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPIU_Ensure_Aint_fits_in_int(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(int)(aint));

/*
 * Ensure an MPI_Aint value fits into an unsigned int.
 * Useful for detecting overflow when MPI_Aint is larger than an
 * unsigned int.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPIU_Ensure_Aint_fits_in_uint(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(unsigned int)(aint));

/*
 * Ensure an MPI_Aint value fits into a pointer.
 * Useful for detecting overflow when MPI_Aint is larger than a pointer.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPIU_Ensure_Aint_fits_in_pointer(aint) \
  MPIU_Assert((aint) == (MPI_Aint)(MPIU_Upint) MPIU_AINT_CAST_TO_VOID_PTR(aint));


#endif /* !defined(MPIU_POINTER_H_INCLUDED) */
