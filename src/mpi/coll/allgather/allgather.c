/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 81920
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the short message algorithm will
        be used if the send buffer size is < this value (in bytes).
        (See also: MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 524288
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        For MPI_Allgather and MPI_Allgatherv, the long message algorithm will be
        used if the send buffer size is >= this value (in bytes)
        (See also: MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE)

    - name        : MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto               - Internal algorithm selection
        brucks             - Force brucks algorithm
        nb                 - Force nonblocking algorithm
        recursive_doubling - Force recursive doubling algorithm
        ring               - Force ring algorithm

    - name        : MPIR_CVAR_ALLGATHER_INTER_ALGORITHM
      category    : COLLECTIVE
      type        : string
      default     : auto
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select allgather algorithm
        auto                      - Internal algorithm selection
        local_gather_remote_bcast - Force local-gather-remote-bcast algorithm
        nb                        - Force nonblocking algorithm

    - name        : MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI_Allgather will allow the device to override the
        MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.
        If set to false, the device-level allgather function will not be
        called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* -- Begin Profiling Symbol Block for routine MPI_Allgather */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Allgather = PMPI_Allgather
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Allgather  MPI_Allgather
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Allgather as PMPI_Allgather
#elif defined(HAVE_WEAK_ATTRIBUTE)
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
    __attribute__ ((weak, alias("PMPI_Allgather")));
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Allgather
#define MPI_Allgather PMPI_Allgather

#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_intra_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_intra_auto(const void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              int recvcount,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int comm_size;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint tot_bytes;
    int type_size, rank, color, count = 0, i, num_of_nodes = -1;
    int *rank_node_map, *node_to_rank_map;
    int uniform_ranks_per_node = 1;
    int threshold = 0, proc_per_node = 0;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        return MPI_SUCCESS;

    // comm_size = comm_ptr->local_size;
    comm_size = MPIR_Comm_size(comm_ptr);
    rank = MPIR_Comm_rank(comm_ptr);

    rank_node_map = MPL_calloc(comm_size, sizeof(int), MPL_MEM_OTHER);

    MPIR_Datatype_get_size_macro(recvtype, type_size);

    tot_bytes = (MPI_Aint) recvcount *comm_size * type_size;

    mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &color);
    /* Topology information collection for selection logic */
    rank_node_map[rank] = color;
    MPIR_Allreduce(MPI_IN_PLACE, rank_node_map, comm_size, MPI_INT, MPI_MAX, comm_ptr, errflag);

    for (i = 0; i < comm_size; i++) {
        if (rank_node_map[i] == color) {
            count++;
        }
        if (num_of_nodes < rank_node_map[i]) {
            num_of_nodes = rank_node_map[i];
        }
    }

    node_to_rank_map = MPL_calloc(num_of_nodes + 1, sizeof(int), MPL_MEM_OTHER);
    for (i = 0; i < comm_size; i++) {
        node_to_rank_map[rank_node_map[i]]++;
    }

    for (i = 0; i < num_of_nodes + 1; i++) {
        if (count != node_to_rank_map[i]) {
            uniform_ranks_per_node = 0;
            break;
        }
    }

    if ((!uniform_ranks_per_node) && !(comm_size & (comm_size - 1))) {
        /* Use default selection logic */
        if ((tot_bytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, errflag);
        } else if (tot_bytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, errflag);
        } else {
            mpi_errno =
                MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, comm_ptr, errflag);
        }

    } else {
        /* Threshold Computation for new selection logic */
        proc_per_node = count;

        if (((num_of_nodes + 1) > 32) && (proc_per_node <= ((num_of_nodes + 1) / 32))) {
            threshold = MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE;
        } else {
            threshold = ((num_of_nodes + 1) * 32768) / proc_per_node;
        }
    }
    /* Use new selection logic */
    if ((tot_bytes < threshold) || ((num_of_nodes + 1) == 1)) {
        if (!(comm_size & (comm_size - 1))) {
            mpi_errno =
                MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                        recvcount, recvtype, comm_ptr, errflag);
        } else {
            mpi_errno =
                MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                            recvtype, comm_ptr, errflag);
        }
    } else {
        mpi_errno =
            MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                      comm_ptr, errflag);
    }


    MPL_free(rank_node_map);
    MPL_free(node_to_rank_map);

    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");

    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_inter_auto
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_inter_auto(const void *sendbuf,
                              int sendcount,
                              MPI_Datatype sendtype,
                              void *recvbuf,
                              int recvcount,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcount, recvtype,
                                                               comm_ptr, errflag);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Allgather_impl
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather_impl(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                        void *recvbuf, int recvcount, MPI_Datatype recvtype,
                        MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_Allgather_intra_algo_choice) {
            case MPIR_ALLGATHER_INTRA_ALGO_BRUCKS:
                mpi_errno =
                    MPIR_Allgather_intra_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTRA_ALGO_RECURSIVE_DOUBLING:
                mpi_errno =
                    MPIR_Allgather_intra_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcount, recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTRA_ALGO_RING:
                mpi_errno =
                    MPIR_Allgather_intra_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTRA_ALGO_NB:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTRA_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno =
                    MPIR_Allgather_intra_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
        }
    } else {
        /* intercommunicator */
        switch (MPIR_Allgather_inter_algo_choice) {
            case MPIR_ALLGATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_BCAST:
                mpi_errno =
                    MPIR_Allgather_inter_local_gather_remote_bcast(sendbuf, sendcount, sendtype,
                                                                   recvbuf, recvcount, recvtype,
                                                                   comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTER_ALGO_NB:
                mpi_errno =
                    MPIR_Allgather_allcomm_nb(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
            case MPIR_ALLGATHER_INTER_ALGO_AUTO:
                MPL_FALLTHROUGH;
            default:
                mpi_errno =
                    MPIR_Allgather_inter_auto(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, errflag);
                break;
        }
    }
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype,
                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ALLGATHER_DEVICE_COLLECTIVE && MPIR_CVAR_DEVICE_COLLECTIVES) {
        mpi_errno = MPID_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                   comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Allgather_impl(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype,
                                        comm_ptr, errflag);
    }

    return mpi_errno;
}

#endif


#undef FUNCNAME
#define FUNCNAME MPI_Allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
/*@
MPI_Allgather - Gathers data from all tasks and distribute the combined
    data to all tasks

Input Parameters:
+ sendbuf - starting address of send buffer (choice)
. sendcount - number of elements in send buffer (integer)
. sendtype - data type of send buffer elements (handle)
. recvcount - number of elements received from any process (integer)
. recvtype - data type of receive buffer elements (handle)
- comm - communicator (handle)

Output Parameters:
. recvbuf - address of receive buffer (choice)

Notes:
 The MPI standard (1.0 and 1.1) says that
.n
.n
 The jth block of data sent from  each process is received by every process
 and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This is misleading; a better description is
.n
.n
 The block of data sent from the jth process is received by every
 process and placed in the jth block of the buffer 'recvbuf'.
.n
.n
 This text was suggested by Rajeev Thakur and has been adopted as a
 clarification by the MPI Forum.

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr = NULL;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    MPIR_FUNC_TERSE_STATE_DECL(MPID_STATE_MPI_ALLGATHER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();

    MPID_THREAD_CS_ENTER(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    MPIR_FUNC_TERSE_COLL_ENTER(MPID_STATE_MPI_ALLGATHER);

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
            MPIR_Datatype *recvtype_ptr = NULL, *sendtype_ptr = NULL;

            MPIR_Comm_valid_ptr(comm_ptr, mpi_errno, FALSE);
            if (mpi_errno != MPI_SUCCESS)
                goto fn_fail;

            if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM) {
                MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcount, mpi_errno);
            } else {
                /* catch common aliasing cases */
                if (sendbuf != MPI_IN_PLACE && sendtype == recvtype &&
                    recvcount != 0 && sendcount != 0) {
                    int recvtype_size;
                    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
                    MPIR_ERRTEST_ALIAS_COLL(sendbuf,
                                            (char *) recvbuf +
                                            comm_ptr->rank * recvcount * recvtype_size, mpi_errno);
                }
            }

            if (sendbuf != MPI_IN_PLACE) {
                MPIR_ERRTEST_COUNT(sendcount, mpi_errno);
                MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                    MPIR_Datatype_get_ptr(sendtype, sendtype_ptr);
                    MPIR_Datatype_valid_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                    MPIR_Datatype_committed_ptr(sendtype_ptr, mpi_errno);
                    if (mpi_errno != MPI_SUCCESS)
                        goto fn_fail;
                }
                MPIR_ERRTEST_USERBUFFER(sendbuf, sendcount, sendtype, mpi_errno);
            }

            MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcount, mpi_errno);
            MPIR_ERRTEST_COUNT(recvcount, mpi_errno);
            MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
            if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                MPIR_Datatype_get_ptr(recvtype, recvtype_ptr);
                MPIR_Datatype_valid_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
                MPIR_Datatype_committed_ptr(recvtype_ptr, mpi_errno);
                if (mpi_errno != MPI_SUCCESS)
                    goto fn_fail;
            }
            MPIR_ERRTEST_USERBUFFER(recvbuf, recvcount, recvtype, mpi_errno);
        }
        MPID_END_ERROR_CHECKS;
    }
#endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */
    mpi_errno = MPIR_Allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype, comm_ptr, &errflag);
    if (mpi_errno)
        goto fn_fail;

    /* ... end of body of routine ... */

  fn_exit:
    MPIR_FUNC_TERSE_COLL_EXIT(MPID_STATE_MPI_ALLGATHER);
    MPID_THREAD_CS_EXIT(GLOBAL, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
    {
        mpi_errno =
            MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER,
                                 "**mpi_allgather", "**mpi_allgather %p %d %D %p %d %D %C", sendbuf,
                                 sendcount, sendtype, recvbuf, recvcount, recvtype, comm);
    }
#endif
    mpi_errno = MPIR_Err_return_comm(comm_ptr, FCNAME, mpi_errno);
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
