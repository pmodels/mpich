/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPIX_Mrecv */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Mrecv = PMPIX_Mrecv
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Mrecv  MPIX_Mrecv
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Mrecv as PMPIX_Mrecv
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Mrecv
#define MPIX_Mrecv PMPIX_Mrecv

/* any non-MPI functions go here, especially non-static ones */

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPIX_Mrecv
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@
MPIX_Mrecv - Blocking receive of message matched by MPIX_Mprobe or MPIX_Improbe.

Input/Output Parameters:
. message - message (handle)

Input Parameters:
+ count - number of elements in the receive buffer (non-negative integer)
- datatype - datatype of each receive buffer element (handle)

Output Parameters:
+ buf - initial address of the receive buffer (choice)
- status - status object (status)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPIX_Mrecv(void *buf, int count, MPI_Datatype datatype, MPIX_Message *message, MPI_Status *status)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *msgp = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_MRECV);

    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_MRECV);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);

            /* TODO more checks may be appropriate */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Request_get_ptr(*message, msgp);

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS
        {
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype *datatype_ptr = NULL;
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                MPID_Datatype_committed_ptr(datatype_ptr, mpi_errno);
            }

            /* MPIX_MESSAGE_NO_PROC should yield a "proc null" status */
            if (*message != MPIX_MESSAGE_NO_PROC) {
                MPID_Request_valid_ptr(msgp, mpi_errno);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                MPIU_ERR_CHKANDJUMP((msgp->kind != MPID_REQUEST_MPROBE),
                                    mpi_errno, MPI_ERR_ARG, "**reqnotmsg");
            }

            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPID_Mrecv(buf, count, datatype, msgp, status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    *message = MPIX_MESSAGE_NULL;

    /* ... end of body of routine ... */

fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_MRECV);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno = MPIR_Err_create_code(
            mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
            "**mpix_mrecv", "**mpix_mrecv %p %d %D %p %p", buf, count, datatype, message, status);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm(NULL, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
