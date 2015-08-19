/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_dup_with_info */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_dup_with_info = PMPI_Comm_dup_with_info
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_dup_with_info  MPI_Comm_dup_with_info
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_dup_with_info as PMPI_Comm_dup_with_info
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm) __attribute__((weak,alias("PMPI_Comm_dup_with_info")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_dup_with_info
#define MPI_Comm_dup_with_info PMPI_Comm_dup_with_info

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_dup_with_info_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_dup_with_info_impl(MPID_Comm * comm_ptr, MPID_Info * info_ptr,
                                 MPID_Comm ** newcomm_p_p)
{
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: We just ignore the info argument for now and just call
     * Comm_dup */
    mpi_errno = MPIR_Comm_dup_impl(comm_ptr, newcomm_p_p);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPICH_MPI_FROM_PMPI */


#undef FUNCNAME
#define FUNCNAME MPI_Comm_dup_with_info
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_dup_with_info - Duplicates an existing communicator with all its cached
               information

Input Parameters:
+ comm - Communicator to be duplicated (handle)
- info - info object (handle)

Output Parameters:
. newcomm - A new communicator over the same group as 'comm' but with a new
  context. See notes.  (handle)

Notes:
  MPI_COMM_DUP_WITH_INFO behaves exactly as MPI_COMM_DUP except that
  the info hints associated with the communicator comm are not
  duplicated in newcomm.  The hints provided by the argument info are
  associated with the output communicator newcomm instead.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

.seealso: MPI_Comm_dup, MPI_Comm_free, MPI_Keyval_create,
 MPI_Attr_put, MPI_Attr_delete, MPI_Comm_create_keyval,
 MPI_Comm_set_attr, MPI_Comm_delete_attr
@*/
int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm * newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_Info *info_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_COMM_DUP_WITH_INFO);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPI_COMM_DUP_WITH_INFO);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr(comm, comm_ptr);
    MPID_Info_get_ptr(info, info_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, FALSE );
            if (mpi_errno) goto fn_fail;
            /* If comm_ptr is not valid, it will be reset to null */
            MPIR_ERRTEST_ARGNULL(newcomm, "newcomm", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Comm_dup_with_info_impl(comm_ptr, info_ptr, &newcomm_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPI_COMM_DUP_WITH_INFO);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpi_comm_dup_with_info",
                                 "**mpi_comm_dup_with_info %C %I %p", comm, info, newcomm);
    }
#endif
    *newcomm = MPI_COMM_NULL;
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
