/*
 *
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef POSIX_COLL_NB_RELEASE_GATHER_H_INCLUDED
#define POSIX_COLL_NB_RELEASE_GATHER_H_INCLUDED

#include "mpiimpl.h"
#include "algo_common.h"
#include "nb_release_gather.h"

/* Intra-node Ibcast is implemented as a release step followed by gather step in release_gather
 * framework. The actual data movement happens in release step. Gather step makes sure that
 * the shared bcast buffer can be reused for next bcast call. Release gather framework has
 * multitple cells in bcast buffer, so that the copying in next cell can be overlapped with
 * copying out of previous cells (pipelining).
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_ibcast_release_gather(void *buffer, int count,
                                                               MPI_Datatype datatype, int root,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_TSP_sched_t sched)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_IBCAST_RELEASE_GATHER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_IBCAST_RELEASE_GATHER);

    int mpi_errno = MPI_SUCCESS;

    MPIDI_POSIX_COMM(comm_ptr, nb_release_gather).num_collective_calls++;
    if (MPIDI_POSIX_COMM(comm_ptr, nb_release_gather).num_collective_calls <
        MPIR_CVAR_POSIX_NUM_NB_COLLS_THRESHOLD) {
        /* Fallback to pt2pt algorithms if the total number of release_gather collective calls is
         * less than the specified threshold */
        goto fallback;
    }

    /* Lazy initialization of release_gather specific struct */
    mpi_errno =
        MPIDI_POSIX_nb_release_gather_comm_init(comm_ptr, MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST);
    MPII_COLLECTIVE_FALLBACK_CHECK(MPIR_Comm_rank(comm_ptr), !mpi_errno, mpi_errno,
                                   "release_gather ibcast cannot create more shared memory. Falling back to pt2pt algorithms.\n");

    mpi_errno =
        MPIDI_POSIX_nb_release_gather_ibcast_impl(buffer, count, datatype, root, comm_ptr, sched);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_MPI_IBCAST_RELEASE_GATHER);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

  fallback:
    /* Fall back to other algo as release_gather based bcast cannot be used */
    mpi_errno =
        MPIR_TSP_Ibcast_sched_intra_tsp_auto(buffer, count, datatype, root, comm_ptr, sched);
    goto fn_exit;
}

/* Intra-node Ireduce is implemented as a release step followed by gather step in release_gather
 * framework. The actual data movement happens in gather step. Release step makes sure that
 * the shared reduce buffer can be reused for next reduce call. Release gather framework has
 * multiple cells in reduce buffer, so that the copying in next cell can be overlapped with
 * reduction and copying out of previous cells (pipelining).
 */
MPL_STATIC_INLINE_PREFIX int MPIDI_POSIX_ireduce_release_gather(const void *sendbuf,
                                                                void *recvbuf, int count,
                                                                MPI_Datatype datatype,
                                                                MPI_Op op, int root,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_TSP_sched_t sched)
{
    MPIR_FUNC_ENTER;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;

    if (MPIR_Comm_size(comm_ptr) == 1) {
        if (sendbuf != MPI_IN_PLACE) {
            /* Simply copy the data from sendbuf to recvbuf if there is only 1 rank and MPI_IN_PLACE
             * is not used */
            mpi_errno_ret = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            if (mpi_errno_ret) {
                MPIR_ERR_ADD(mpi_errno, mpi_errno_ret);
            }
        }
        goto fn_exit;
    }
    MPIDI_POSIX_COMM(comm_ptr, nb_release_gather).num_collective_calls++;
    if (MPIDI_POSIX_COMM(comm_ptr, nb_release_gather).num_collective_calls <
        MPIR_CVAR_POSIX_NUM_NB_COLLS_THRESHOLD) {
        /* Fallback to pt2pt algorithms if the total number of release_gather collective calls is
         * less than the specified threshold */
        goto fallback;
    }

    /* Lazy initialization of release_gather specific struct */
    mpi_errno =
        MPIDI_POSIX_nb_release_gather_comm_init(comm_ptr, MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE);
    MPII_COLLECTIVE_FALLBACK_CHECK(MPIR_Comm_rank(comm_ptr), !mpi_errno, mpi_errno,
                                   "release_gather ibcast cannot create more shared memory. Falling back to pt2pt algorithms.\n");

    mpi_errno =
        MPIDI_POSIX_nb_release_gather_ireduce_impl((void *) sendbuf, recvbuf, count, datatype, op,
                                                   root, comm_ptr, sched);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;

  fn_fail:
    goto fn_exit;

  fallback:
    /* Fall back to other algo as release_gather algo cannot be used */
    mpi_errno =
        MPIR_TSP_Ireduce_sched_intra_tsp_auto(sendbuf, recvbuf, count, datatype, op, root,
                                              comm_ptr, sched);
    goto fn_exit;
}

#endif /* POSIX_COLL_NB_RELEASE_GATHER_H_INCLUDED */
