/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "ibcast.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_IBCAST_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree (kary, knomial, etc.) based ibcast

    - name        : MPIR_CVAR_IBCAST_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for tree based ibcast
        kary      - kary tree type
        knomial_1 - knomial_1 tree type
        knomial_2 - knomial_2 tree type

    - name        : MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in tree based
        ibcast. Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IBCAST_RING_CHUNK_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum chunk size (in bytes) for pipelining in ibcast ring algorithm.
        Default value is 0, that is, no pipelining by default

    - name        : MPIR_CVAR_IBCAST_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibcast algorithm
        auto                                 - Internal algorithm selection
        binomial                             - Force Binomial algorithm
        scatter_recursive_doubling_allgather - Force Scatter Recursive Doubling Allgather algorithm
        scatter_ring_allgather               - Force Scatter Ring Allgather algorithm
        gentran_tree                         - Force Generic Transport Tree algorithm
        gentran_scatterv_recexch_allgatherv  - Force Generic Transport Scatterv followed by Recursive Exchange Allgatherv algorithm
        gentran_ring                         - Force Generic Transport Ring algorithm

    - name        : MPIR_CVAR_IBCAST_SCATTERV_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for tree based scatter in scatter_recexch_allgather algorithm

    - name        : MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 2
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        k value for recursive exchange based allgather in scatter_recexch_allgather algorithm

    - name        : MPIR_CVAR_IBCAST_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : enum
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select ibcast algorithm
        auto - Internal algorithm selection
        flat - Force flat algorithm

    - name        : MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Ibcast will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.  If set to false,
        the device-level ibcast function will not be called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Ibcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Ibcast = PMPI_Ibcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Ibcast  MPI_Ibcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Ibcast as PMPI_Ibcast
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request * request) __attribute__ ((weak, alias("PMPI_Ibcast")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Ibcast
#define MPI_Ibcast PMPI_Ibcast

/* any non-MPI functions go here, especially non-static ones */

/* Provides a "flat" broadcast that doesn't know anything about
 * hierarchy.  It will choose between several different algorithms
 * based on the given parameters. */
int MPIR_Ibcast_sched_intra_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint type_size, nbytes;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        mpi_errno = MPIR_Ibcast_sched_intra_binomial(buffer, count, datatype, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPL_is_pof2(comm_size, NULL))) {
            mpi_errno =
                MPIR_Ibcast_sched_intra_scatter_recursive_doubling_allgather(buffer, count,
                                                                             datatype, root,
                                                                             comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno =
                MPIR_Ibcast_sched_intra_scatter_ring_allgather(buffer, count, datatype, root,
                                                               comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Provides a "flat" broadcast for intercommunicators that doesn't
 * know anything about hierarchy.  It will choose between several
 * different algorithms based on the given parameters. */
int MPIR_Ibcast_sched_inter_auto(void *buffer, int count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast_sched_inter_flat(buffer, count, datatype, root, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ibcast_sched_impl(void *buffer, int count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT &&
            MPIR_CVAR_ENABLE_SMP_COLLECTIVES && !MPIR_CVAR_ENABLE_SMP_BCAST) {
            mpi_errno = MPIR_Ibcast_sched_intra_smp(buffer, count, datatype, root, comm_ptr, s);
        } else {
            /* intercommunicator */
            switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
                case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_binomial:
                    mpi_errno = MPIR_Ibcast_sched_intra_binomial(buffer, count, datatype,
                                                                 root, comm_ptr, s);
                    break;
                case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_scatter_recursive_doubling_allgather:
                    mpi_errno =
                        MPIR_Ibcast_sched_intra_scatter_recursive_doubling_allgather(buffer, count,
                                                                                     datatype, root,
                                                                                     comm_ptr, s);
                    break;
                case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_scatter_ring_allgather:
                    mpi_errno =
                        MPIR_Ibcast_sched_intra_scatter_ring_allgather(buffer, count, datatype,
                                                                       root, comm_ptr, s);
                    break;
                case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_auto:
                    MPL_FALLTHROUGH;
                default:
                    mpi_errno = MPIR_Ibcast_sched_intra_auto(buffer, count, datatype,
                                                             root, comm_ptr, s);
                    break;
            }
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_IBCAST_INTER_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_flat:
                mpi_errno = MPIR_Ibcast_sched_inter_flat(buffer, count, datatype, root,
                                                         comm_ptr, s);
                break;
            case MPIR_CVAR_IBCAST_INTER_ALGORITHM_auto:
                MPL_FALLTHROUGH;
            default:
                mpi_errno = MPIR_Ibcast_sched_inter_auto(buffer, count, datatype, root,
                                                         comm_ptr, s);
                break;
        }
    }

    return mpi_errno;
}

int MPIR_Ibcast_sched(void *buffer, int count, MPI_Datatype datatype, int root,
                      MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ibcast_sched(buffer, count, datatype, root, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ibcast_sched_impl(buffer, count, datatype, root, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ibcast_impl(void *buffer, int count, MPI_Datatype datatype, int root,
                     MPIR_Comm * comm_ptr, MPIR_Request ** request)
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
        switch (MPIR_CVAR_IBCAST_INTRA_ALGORITHM) {
            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_tree:
                mpi_errno =
                    MPIR_Ibcast_intra_gentran_tree(buffer, count, datatype, root, comm_ptr,
                                                   MPIR_Ibcast_tree_type,
                                                   MPIR_CVAR_IBCAST_TREE_KVAL,
                                                   MPIR_CVAR_IBCAST_TREE_PIPELINE_CHUNK_SIZE,
                                                   request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_scatterv_recexch_allgatherv:
                mpi_errno =
                    MPIR_Ibcast_intra_gentran_scatterv_recexch_allgatherv(buffer, count, datatype,
                                                                          root, comm_ptr,
                                                                          MPIR_CVAR_IBCAST_SCATTERV_KVAL,
                                                                          MPIR_CVAR_IBCAST_ALLGATHERV_RECEXCH_KVAL,
                                                                          request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            case MPIR_CVAR_IBCAST_INTRA_ALGORITHM_gentran_ring:
                mpi_errno =
                    MPIR_Ibcast_intra_gentran_ring(buffer, count, datatype, root, comm_ptr,
                                                   MPIR_CVAR_IBCAST_RING_CHUNK_SIZE, request);
                MPIR_ERR_CHECK(mpi_errno);
                goto fn_exit;
                break;
            default:
                /* go down to the MPIR_Sched-based algorithms */
                break;
        }
    }

    /* If the user doesn't pick a transport-enabled algorithm, go to the old
     * sched function. */
    mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIR_Sched_create(&s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Ibcast_sched(buffer, count, datatype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPIR_Comm * comm_ptr,
                MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_IBCAST_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Ibcast(buffer, count, datatype, root, comm_ptr, request);
    } else {
        mpi_errno = MPIR_Ibcast_impl(buffer, count, datatype, root, comm_ptr, request);
    }

    return mpi_errno;
}

#endif /* MPICH_MPI_FROM_PMPI */

/*@
MPI_Ibcast - Broadcasts a message from the process with rank "root" to
             all other processes of the communicator in a nonblocking way

Input/Output Parameters:
. buffer - starting address of buffer (choice)

Input Parameters:
+ count - number of entries in buffer (non-negative integer)
. datatype - datatype of buffer (handle)
. root - rank of broadcast root (integer)
- comm - communicator (handle)

Output Parameters:
. request - communication request (handle)

.N ThreadSafe

.N Fortran

.N Errors
@*/
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm,
               MPI_Request * request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Request *request_ptr = NULL;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_IBCAST);

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_ENTER(VCI, MPIR_THREAD_VCI_GLOBAL_MUTEX);
    MPIR_FUNC_TERSE_ENTER(MPID_STATE_MPI_IBCAST);

    /* Validate parameters, especially handles needing to be converted */
#ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
            MPIR_ERRTEST_COUNT(count, mpi_errno);
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

            if (!HANDLE_IS_BUILTIN(datatype)) {
                MPIR_Datatype *datatype_ptr = NULL;
                MPIR_Datatype_get_ptr(datatype, datatype_ptr);
                MPIR_Datatype_valid_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(datatype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }

            MPIR_ERRTEST_ARGNULL(request, "request", mpi_errno);
            /* TODO more checks may be appropriate (counts, in_place, buffer aliasing, etc) */
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Ibcast(buffer, count, datatype, root, comm_ptr, &request_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* create a complete request, if needed */
    if (!request_ptr)
        request_ptr = MPIR_Request_create_complete(MPIR_REQUEST_KIND__COLL);
    /* return the handle of the request to the user */
    *request = request_ptr->handle;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_EXIT(MPID_STATE_MPI_IBCAST);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPID_THREAD_CS_EXIT(VCI, MPIR_THREAD_VCI_GLOBAL_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, __func__, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_ibcast", "**mpi_ibcast %p %d %D %C %p", buffer, count,
                                 datatype, comm, request);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, __func__, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
