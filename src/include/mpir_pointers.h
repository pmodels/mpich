/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_POINTERS_H_INCLUDED
#define MPIR_POINTERS_H_INCLUDED

#include "mpi.h"
#include "mpichconf.h"
#include "mpichconfconst.h"
#include "mpir_assert.h"
#include "mpl.h"


/* This test is lame.  Should eventually include cookie test
   and in-range addresses */
#define MPIR_Valid_ptr_class(kind,ptr,errclass,err) \
  {if (!(ptr)) { err = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, errclass, \
                                             "**nullptrtype", "**nullptrtype %s", #kind ); } }

#define MPIR_Info_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Info,ptr,MPI_ERR_INFO,err)
/* Check not only for a null pointer but for an invalid communicator,
   such as one that has been freed.  Let's try the ref_count as the test
   for now */
/* ticket #1441: check (refcount<=0) to cover the case of 0, an "over-free" of
 * -1 or similar, and the 0xecec... case when --enable-g=mem is used */
#define MPIR_Comm_valid_ptr(ptr,err,ignore_rev) {     \
     MPIR_Valid_ptr_class(Comm,ptr,MPI_ERR_COMM,err); \
     if ((ptr) && MPIR_Object_get_ref(ptr) <= 0) {    \
         MPIR_ERR_SET(err,MPI_ERR_COMM,"**comm");     \
         ptr = 0;                                     \
     } else if ((ptr) && (ptr)->revoked && !(ignore_rev)) {        \
         MPIR_ERR_SET(err,MPIX_ERR_REVOKED,"**comm"); \
     }                                                \
}
#define MPIR_Win_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Win,ptr,MPI_ERR_WIN,err)
#define MPIR_Group_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Group,ptr,MPI_ERR_GROUP,err)
#define MPIR_Op_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Op,ptr,MPI_ERR_OP,err)
#define MPIR_Errhandler_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Errhandler,ptr,MPI_ERR_ARG,err)
#define MPIR_Request_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Request,ptr,MPI_ERR_REQUEST,err)
#define MPII_Keyval_valid_ptr(ptr,err) MPIR_Valid_ptr_class(Keyval,ptr,MPI_ERR_KEYVAL,err)


/* Assigns (src_) to (dst_), checking that (src_) fits in (dst_) without
 * truncation.
 *
 * When fiddling with this macro, please keep C's overly complicated integer
 * promotion/truncation/conversion rules in mind.  A discussion of these issues
 * can be found in Chapter 5 of "Secure Coding in C and C++" by Robert Seacord.
 */
#define MPIR_Assign_trunc(dst_,src_,dst_type_)                                         \
    do {                                                                               \
        /* will catch some of the cases if the expr_inttype macros aren't available */ \
        MPIR_Assert((src_) == (dst_type_)(src_));                                      \
        dst_ = (dst_type_)(src_);                                                      \
    } while (0)

/*
 * Ensure an MPI_Aint value fits into a signed int.
 * Useful for detecting overflow when MPI_Aint is larger than an int.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#define MPIR_Ensure_Aint_fits_in_int(aint) \
  MPIR_Assert((aint) == (MPI_Aint)(int)(aint));

/*
 * Ensure an MPI_Aint value fits into a pointer.
 * Useful for detecting overflow when MPI_Aint is larger than a pointer.
 *
 * \param[in]  aint  Variable of type MPI_Aint
 */
#ifndef SIZEOF_PTR_IS_AINT
#define MPIR_Ensure_Aint_fits_in_pointer(aint) \
  MPIR_Assert((aint) == (MPI_Aint)(uintptr_t) MPIR_AINT_CAST_TO_VOID_PTR(aint));
#else
#define MPIR_Ensure_Aint_fits_in_pointer(aint) do {} while(0)
#endif

#endif /* MPIR_POINTERS_H_INCLUDED */
