/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"

/* -- Begin Profiling Symbol Block for routine MPI_Comm_group */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Comm_group = PMPI_Comm_group
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Comm_group  MPI_Comm_group
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Comm_group as PMPI_Comm_group
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Comm_group(MPI_Comm comm, MPI_Group * group)
    __attribute__ ((weak, alias("PMPI_Comm_group")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Comm_group
#define MPI_Comm_group PMPI_Comm_group

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_group_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Comm_group_impl(MPIR_Comm * comm_ptr, MPIR_Group ** group_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    int i, lpid, n;
    int comm_world_size = MPIR_Process.comm_world->local_size;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPIR_COMM_GROUP_IMPL);

    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPIR_COMM_GROUP_IMPL);
    /* Create a group if necessary and populate it with the
     * local process ids */
    if (!comm_ptr->local_group) {
        n = comm_ptr->local_size;
        mpi_errno = MPIR_Group_create(n, group_ptr);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        (*group_ptr)->is_local_dense_monotonic = TRUE;
        for (i = 0; i < n; i++) {
            (void) MPID_Comm_get_lpid(comm_ptr, i, &lpid, FALSE);
            (*group_ptr)->lrank_to_lpid[i].lpid = lpid;
            if (lpid > comm_world_size ||
                (i > 0 && (*group_ptr)->lrank_to_lpid[i - 1].lpid != (lpid - 1))) {
                (*group_ptr)->is_local_dense_monotonic = FALSE;
            }
        }

        (*group_ptr)->size = n;
        (*group_ptr)->rank = comm_ptr->rank;
        (*group_ptr)->idx_of_first_lpid = -1;

        comm_ptr->local_group = *group_ptr;
    } else {
        *group_ptr = comm_ptr->local_group;
    }

    /* FIXME : Add a sanity check that the size of the group is the same as
     * the size of the communicator.  This helps catch corrupted
     * communicators */

    MPIR_Group_add_ref(comm_ptr->local_group);

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPIR_COMM_GROUP_IMPL);
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

#endif

#undef FUNCNAME
#define FUNCNAME MPI_Comm_group
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@

MPI_Comm_group - Accesses the group associated with given communicator

Input Parameters:
. comm - Communicator (handle)

Output Parameters:
. group - Group in communicator (handle)

Notes:
.N COMMNULL

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Comm_group(MPI_Comm comm, MPI_Group * group)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Group *group_ptr;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_COMM_GROUP);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_COMM_GROUP);

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
    MPIR_Comm_get_ptr(comm, comm_ptr);

    /* Validate parameters and objects (post conversion) */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, TRUE);
            /* If comm_ptr is not valid, it will be reset to null */
            if (mpi_errno)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(group, "group", mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Comm_group_impl(comm_ptr, &group_ptr);
    if (mpi_errno)
        goto fn_fail;
    *group = group_ptr->handle;
    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_COMM_GROUP);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_comm_group", "**mpi_comm_group %C %p", comm, group);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
