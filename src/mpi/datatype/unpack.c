/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Unpack */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Unpack = PMPI_Unpack
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Unpack  MPI_Unpack
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Unpack as PMPI_Unpack
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount,
               MPI_Datatype datatype, MPI_Comm comm) __attribute__ ((weak, alias("PMPI_Unpack")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Unpack
#define MPI_Unpack PMPI_Unpack
#endif

/*@
    MPI_Unpack - Unpack a buffer according to a datatype into contiguous memory

Input Parameters:
+ inbuf - input buffer start (choice)
. insize - size of input buffer, in bytes (integer)
. outcount - number of items to be unpacked (integer)
. datatype - datatype of each output data item (handle)
- comm - communicator for packed message (handle)

Output Parameters:
. outbuf - output buffer start (choice)

Inout/Output Parameters:
. position - current position in bytes (integer)


.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_ARG

.seealso: MPI_Pack, MPI_Pack_size
@*/
int MPI_Unpack(const void *inbuf, int insize, int *position,
               void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint position_x;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_UNPACK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_UNPACK);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif

    /* Convert MPI object handles to object pointers */
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            if (insize > 0) {
                MPIR_ERRTEST_ARGNULL(inbuf, "input buffer", mpi_errno);
            }
            /* Note: outbuf could be MPI_BOTTOM; don't test for NULL */
            MPIR_ERRTEST_COUNT(insize, mpi_errno);
            MPIR_ERRTEST_COUNT(outcount, mpi_errno);

            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */

            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            if (datatype != MPI_DATATYPE_NULL && !HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;

                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    position_x = *position;

    MPI_Aint actual_unpack_bytes;
    void *buf = (void *) ((char *) inbuf + position_x);
    mpi_errno =
        MPIR_Typerep_unpack(buf, insize, outbuf, outcount, datatype, 0, &actual_unpack_bytes);
    if (mpi_errno)
        goto fn_fail;

    position_x += actual_unpack_bytes;
    MPIR_Assign_trunc(*position, position_x, int);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_UNPACK);
    return mpi_errno;


  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_unpack", "**mpi_unpack %p %d %p %p %d %D %C", inbuf, insize,
                                 position, outbuf, outcount, datatype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
