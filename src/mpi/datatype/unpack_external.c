/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpack_external */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpack_external = PMPI_Unpack_external
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpack_external  MPI_Unpack_external
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpack_external as PMPI_Unpack_external
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize,
                        MPI_Aint * position, void *outbuf, int outcount, MPI_Datatype datatype)
    __attribute__ ((weak, alias("PMPI_Unpack_external")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpack_external
#define MPI_Unpack_external PMPI_Unpack_external

#endif

/*@
   MPI_Unpack_external - Unpack a buffer (packed with MPI_Pack_external)
   according to a datatype into contiguous memory

Input Parameters:
+ datarep - data representation (string)
. inbuf - input buffer start (choice)
. insize - input buffer size, in bytes (address integer)
. outcount - number of output data items (integer)
. datatype - datatype of output data item (handle)

Input/Output Parameters:
. position - current position in buffer, in bytes (address integer)

Output Parameters:
. outbuf - output buffer start (choice)

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_TYPE
.N MPI_ERR_ARG
@*/
int MPI_Unpack_external(const char datarep[],
                        const void *inbuf,
                        MPI_Aint insize,
                        MPI_Aint * position, void *outbuf, int outcount, MPI_Datatype datatype)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_UNPACK_EXTERNAL);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_UNPACK_EXTERNAL);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (insize > 0) {
                MPIR_ERRTEST_ARGNULL(inbuf, "input buffer", mpi_errno);
            }
            /* NOTE: outbuf could be MPI_BOTTOM; don't test for NULL */
            MPIR_ERRTEST_COUNT(insize, mpi_errno);
            MPIR_ERRTEST_COUNT(outcount, mpi_errno);

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (datatype != MPI_DATATYPE_NULL && !HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            /* If datatye_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    if (insize == 0) {
        goto fn_exit;
    }

    MPI_Aint actual_unpack_bytes;
    mpi_errno = MPIR_Typerep_unpack_external((void *) ((char *) inbuf + *position),
                                             outbuf, outcount, datatype, &actual_unpack_bytes);
    if (mpi_errno)
        goto fn_fail;
    *position += actual_unpack_bytes;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_UNPACK_EXTERNAL);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_unpack_external",
                                 "**mpi_unpack_external %s %p %d %p %p %d %D", datarep, inbuf,
                                 insize, position, outbuf, outcount, datatype);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(NULL, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
