/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IBARRIER_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based ibarrier

    - name        : MPIR_CVAR_IBARRIER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibarrier algorithm
        auto               - Internal algorithm selection
        recursive_doubling - Force recursive doubling algorithm
        recexch            - Force generic transport based recursive exchange algorithm

    - name        : MPIR_CVAR_IBARRIER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibarrier algorithm
        auto  - Internal algorithm selection
        bcast - Force bcast algorithm

    - name        : MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ibarrier will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level ibarrier function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ibarrier */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibarrier = PMPI_Ibarrier
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibarrier  MPI_Ibarrier
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibarrier as PMPI_Ibarrier
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibarrier(MPI_Comm comm, MPI_Request * request)
    __attribute__ ((weak, alias("PMPI_Ibarrier")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibarrier
#define MPI_Ibarrier PMPI_Ibarrier

/* any non-MPI functions go here, especially non-static ones */

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched_intra_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier_sched_intra_recursive_doubling(comm_ptr, s);

    return mpi_errno;
}

/* It will choose between several different algorithms based on the given
 * parameters. */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched_inter_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_sched_inter_bcast(comm_ptr, s);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched_impl(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Ibarrier_intra_algo_choice) {
            case MPIR_IBARRIER_INTRA_ALGO_RECURSIVE_DOUBLING:
                mpi_errno = MPIR_Ibarrier_sched_intra_recursive_doubling(comm_ptr, s);
                break;
            case MPIR_IBARRIER_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ibarrier_sched_intra_auto(comm_ptr, s);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Ibarrier_inter_algo_choice) {
            case MPIR_IBARRIER_INTER_ALGO_BCAST:
                mpi_errno = MPIR_Ibarrier_sched_inter_bcast(comm_ptr, s);
                break;
            case MPIR_IBARRIER_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ibarrier_sched_inter_auto(comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_sched(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ibarrier_sched(comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibarrier_sched_impl(comm_ptr, s);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier_impl(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;
    int tag = -1;
    MPIR_Sched_t s = MPIR_SCHED_NULL;

    *request = NULL;

    /* If the user picks one of the transport-enabled algorithms, branch there
     * before going down to the MPIR_Sched-based algorithms. */
    /* TODO - Eventually the intention is to replace all of the
     * MPIR_Sched-based algorithms with transport-enabled algorithms, but that
     * will require sufficient performance testing and replacement algorithms. */
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Ibarrier_intra_algo_choice) {
            case MPIR_IBARRIER_INTRA_ALGO_GENTRAN_RECEXCH:
                mpi_errno = MPIR_Ibarrier_intra_recexch(comm_ptr, request);

                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                goto fn_exit;
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }
    if (comm_ptr->local_size != 1 || comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        mpi_errno = MPIR_Sched_create(&s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Ibarrier_sched(comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibarrier(MPIR_Comm * comm_ptr, MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IBARRIER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ibarrier(comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibarrier_impl(comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Ibarrier
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Ibarrier - Notifies the process that it has reached the barrier and returns
               immediately

Input Parameters:
. comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

Notes:
MPI_Ibarrier is a nonblocking version of MPI_barrier. By calling MPI_Ibarrier,
a process notifies that it has reached the barrier. The call returns
immediately, independent of whether other processes have called MPI_Ibarrier.
The usual barrier semantics are enforced at the corresponding completion
operation (test or wait), which in the intra-communicator case will complete
only after all other processes in the communicator have called MPI_Ibarrier. In
the intercommunicator case, it will complete when all processes in the remote
group have called MPI_Ibarrier.

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibarrier(MPI_Comm comm, MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBARRIER);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBARRIER);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_COMM(comm, mpi_errno);

            /* TODO more checks may be appropriate */
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
            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;
            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibarrier(comm_ptr, &request_ptr);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBARRIER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ibarrier", "**mpi_ibarrier %C %p", comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
