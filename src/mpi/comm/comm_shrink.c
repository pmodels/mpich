/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpicomm.h"
#include <stdint.h>

/* This function has multiple phases.
 *
 * In the first phase, all alive processes must collectively decide which
 * processes are dead. This happens via a fault-tolerant all-reduce style
 * algorithm. This is implemented via the recursive-doubling algorithm as a
 * first pass for simplicity.
 *
 * In the second phase, the remaining processes must create a new communicator
 * based on the group determined in the first phase. This phase simply uses
 * the existing implementation of MPI_Comm_create_group. If the call to
 * MPI_Comm_create_group fails, then the algorithm is restarted in phase one
 * and a new group is determined.
 */

/* -- Begin Profiling Symbol Block for routine MPIX_Comm_shrink */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPIX_Comm_shrink = PMPIX_Comm_shrink
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPIX_Comm_shrink  MPIX_Comm_shrink
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPIX_Comm_shrink as PMPIX_Comm_shrink
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPIX_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm) __attribute__((weak,alias("PMPIX_Comm_shrink")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPIX_Comm_shrink
#define MPIX_Comm_shrink PMPIX_Comm_shrink
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_Comm_shrink
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/* comm shrink impl; assumes that standard error checking has already taken
 * place in the calling function */
int MPIR_Comm_shrink(MPID_Comm *comm_ptr, MPID_Comm **newcomm_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Group *global_failed, *comm_grp, *new_group_ptr;
    int attempts = 0;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_COMM_SHRINK);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_COMM_SHRINK);

    /* TODO - Implement this function for intercommunicators */
    MPIR_Comm_group_impl(comm_ptr, &comm_grp);

    do {
        errflag = MPIR_ERR_NONE;

        MPID_Comm_get_all_failed_procs(comm_ptr, &global_failed, MPIR_SHRINK_TAG);
        /* Ignore the mpi_errno value here as it will definitely communicate
         * with failed procs */

        mpi_errno = MPIR_Group_difference_impl(comm_grp, global_failed, &new_group_ptr);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        if (MPID_Group_empty != global_failed) MPIR_Group_release(global_failed);

        mpi_errno = MPIR_Comm_create_group(comm_ptr, new_group_ptr, MPIR_SHRINK_TAG, newcomm_ptr);
        if (*newcomm_ptr == NULL) {
            errflag = MPIR_ERR_PROC_FAILED;
        } else if (mpi_errno) {
            errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_Comm_release(*newcomm_ptr);
        }

        mpi_errno = MPIR_Allreduce_group(MPI_IN_PLACE, &errflag, 1, MPI_INT, MPI_MAX, comm_ptr,
            new_group_ptr, MPIR_SHRINK_TAG, &errflag);
        MPIR_Group_release(new_group_ptr);

        if (errflag) {
            if (*newcomm_ptr != NULL && MPIU_Object_get_ref(*newcomm_ptr) > 0) {
                MPIU_Object_set_ref(*newcomm_ptr, 1);
                MPIR_Comm_release(*newcomm_ptr);
            }
            if (MPIU_Object_get_ref(new_group_ptr) > 0) {
                MPIU_Object_set_ref(new_group_ptr, 1);
                MPIR_Group_release(new_group_ptr);
            }
        }
    } while (errflag && ++attempts < 5);

    if (errflag && attempts >= 5) goto fn_fail;
    else mpi_errno = MPI_SUCCESS;

  fn_exit:
    MPIR_Group_release(comm_grp);
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_COMM_SHRINK);
    return mpi_errno;
  fn_fail:
    if (*newcomm_ptr) MPIU_Object_set_ref(*newcomm_ptr, 0);
    MPIU_Object_set_ref(global_failed, 0);
    MPIU_Object_set_ref(new_group_ptr, 0);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIX_Comm_shrink
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPIX_Comm_shrink - Creates a new communitor from an existing communicator while
                  excluding failed processes

Input Parameters:
+ comm - communicator (handle)

Output Parameters:
. newcomm - new communicator (handle)

.N Threadsafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM

@*/
int MPIX_Comm_shrink(MPI_Comm comm, MPI_Comm *newcomm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL, *newcomm_ptr;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIX_COMM_SHRINK);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIX_COMM_SHRINK);

    /* Validate parameters, and convert MPI object handles to object pointers */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;

        MPID_Comm_get_ptr( comm, comm_ptr );

        MPID_BEGIN_ERROR_CHECKS;
        {
            /* Validate comm_ptr */
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno, TRUE );
            if (mpi_errno) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#else
    {
        MPID_Comm_get_ptr( comm, comm_ptr );
    }
#endif

    /* ... body of routine ... */
    mpi_errno = MPIR_Comm_shrink(comm_ptr, &newcomm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (newcomm_ptr)
        MPID_OBJ_PUBLISH_HANDLE(*newcomm, newcomm_ptr->handle);
    else
        *newcomm = MPI_COMM_NULL;
    /* ... end of body of routine ... */

  fn_exit:
    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIX_COMM_SHRINK);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                 MPI_ERR_OTHER, "**mpix_comm_shrink",
                                 "**mpix_comm_shrink %C %p", comm, newcomm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
