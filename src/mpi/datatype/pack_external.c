/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Pack_external */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Pack_external = PMPI_Pack_external
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Pack_external  MPI_Pack_external
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Pack_external as PMPI_Pack_external
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Pack_external(const char datarep[], const void *inbuf, int incount,
                      MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint * position)
    __attribute__ ((weak, alias("PMPI_Pack_external")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Pack_external
#define MPI_Pack_external PMPI_Pack_external

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Pack_external
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
   MPI_Pack_external - Packs a datatype into contiguous memory, using the
     external32 format

Input Parameters:
+ datarep - data representation (string)
. inbuf - input buffer start (choice)
. incount - number of input data items (integer)
. datatype - datatype of each input data item (handle)
- outsize - output buffer size, in bytes (address integer)

Output Parameters:
. outbuf - output buffer start (choice)

Input/Output Parameters:
. position - current position in buffer, in bytes (address integer)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
.N MPI_ERR_COUNT
@*/
int MPI_Pack_external(const char datarep[],
                      const void *inbuf,
                      int incount,
                      MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint * position)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint first, last;

    MPIR_Segment *segp;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_PACK_EXTERNAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_PACK_EXTERNAL);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COUNT(incount, mpi_errno);
            MPIR_ERRTEST_COUNT(outsize, mpi_errno);
            /* NOTE: inbuf could be null (MPI_BOTTOM) */
            if (incount > 0) {
                MPIR_ERRTEST_ARGNULL(outbuf, "output buffer", mpi_errno);
            }
            MPIR_ERRTEST_ARGNULL(position, "position", mpi_errno);

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ... */
    if (incount == 0) {
        goto fn_exit;
    }

    segp = MPIR_Segment_alloc();
    /* --BEGIN ERROR HANDLING-- */
    if (segp == NULL) {
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         FCNAME,
                                         __LINE__,
                                         MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPIR_Segment");
        goto fn_fail;
    }
    /* --END ERROR HANDLING-- */
    mpi_errno = MPIR_Segment_init(inbuf, incount, datatype, segp);
    if (mpi_errno != MPI_SUCCESS)
        goto fn_fail;

    /* NOTE: the use of buffer values and positions in MPI_Pack_external and
     * in MPIR_Segment_pack_external are quite different.  See code or docs
     * or something.
     */
    first = 0;
    last = SEGMENT_IGNORE_LAST;

    /* Ensure that pointer increment fits in a pointer */
    MPIR_Ensure_Aint_fits_in_pointer((MPIR_VOID_PTR_CAST_TO_MPI_AINT outbuf) + *position);

    MPIR_Segment_pack_external32(segp, first, &last, (void *) ((char *) outbuf + *position));

    *position += last;

    MPIR_Segment_free(segp);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_PACK_EXTERNAL);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_pack_external", "**mpi_pack_external %s %p %d %D %p %d %p",
                                 datarep, inbuf, incount, datatype, outbuf, outsize, position);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
