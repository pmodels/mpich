/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

            if (!HANDLE_IS_BUILTIN(datatype)) {
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

    MPI_Aint actual_pack_bytes;
    mpi_errno =
        MPIR_Typerep_pack_external(inbuf, incount, datatype, (void *) ((char *) outbuf + *position),
                                   &actual_pack_bytes);
    if (mpi_errno)
        goto fn_fail;
    *position += actual_pack_bytes;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_PACK_EXTERNAL);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_pack_external", "**mpi_pack_external %s %p %d %D %p %d %p",
                                 datarep, inbuf, incount, datatype, outbuf, outsize, position);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(0, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
