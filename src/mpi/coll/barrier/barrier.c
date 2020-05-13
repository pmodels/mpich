/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_BARRIER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select barrier algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        nb            - Force nonblocking algorithm
        dissemination - Force dissemination algorithm

    - name        : MPIR_CVAR_BARRIER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select barrier algorithm
        auto - Internal algorithm selection (can be overridden with MPIR_CVAR_COLL_SELECTION_TUNING_JSON_FILE)
        bcast - Force bcast algorithm
        nb    - Force nonblocking algorithm

    - name        : MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This CVAR is only used when MPIR_CVAR_DEVICE_COLLECTIVES
        is set to "percoll".  If set to true, MPI_Barrier will
        allow the device to override the MPIR-level collective
        algorithms.  The device might still call the MPIR-level
        algorithms manually.  If set to false, the device-override
        will be disabled.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Barrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Barrier = PMPI_Barrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Barrier  MPI_Barrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Barrier as PMPI_Barrier
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Barrier(MPI_Comm comm) __attribute__ ((weak, alias("PMPI_Barrier")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Barrier
#define MPI_Barrier PMPI_Barrier


int MPIR_Barrier_allcomm_auto(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__BARRIER,
        .comm_ptr = comm_ptr,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_dissemination:
            mpi_errno = MPIR_Barrier_intra_dissemination(comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_smp:
            mpi_errno = MPIR_Barrier_intra_smp(comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_inter_bcast:
            mpi_errno = MPIR_Barrier_inter_bcast(comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_allcomm_nb:
            mpi_errno = MPIR_Barrier_allcomm_nb(comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }

    return mpi_errno;
}

int MPIR_Barrier_impl(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_BARRIER_INTRA_ALGORITHM) {
            case MPIR_CVAR_BARRIER_INTRA_ALGORITHM_dissemination:
                mpi_errno = MPIR_Barrier_intra_dissemination(comm_ptr, errflag);
                break;
            case MPIR_CVAR_BARRIER_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Barrier_allcomm_nb(comm_ptr, errflag);
                break;
            case MPIR_CVAR_BARRIER_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Barrier_allcomm_auto(comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_BARRIER_INTER_ALGORITHM) {
            case MPIR_CVAR_BARRIER_INTER_ALGORITHM_bcast:
                mpi_errno = MPIR_Barrier_inter_bcast(comm_ptr, errflag);
                break;
            case MPIR_CVAR_BARRIER_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Barrier_allcomm_nb(comm_ptr, errflag);
                break;
            case MPIR_CVAR_BARRIER_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Barrier_allcomm_auto(comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Barrier(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_BARRIER_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Barrier(comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif





/*@

MPI_Barrier - Blocks until all processes in the communicator have
reached this routine.

Input Parameters:
. comm - communicator (handle)

Notes:
Blocks the caller until all processes in the communicator have called it;
that is, the call returns at any process only after all members of the
communicator have entered the call.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
@*/
int MPI_Barrier(MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_BARRIER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_BARRIER);

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
            /* Validate communicator */
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno)
                goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Barrier(comm_ptr, &errflag);
    MPIR_ERR_CHECK(mpi_errno);

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_BARRIER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_barrier", "**mpi_barrier %C", comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
