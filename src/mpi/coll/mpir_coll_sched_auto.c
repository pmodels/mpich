/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* This file contains hand coded nonblocking auto selection algorithms.
 * They are superseded by csel-based selection logic. However, some
 * non-blocking algorithm (e.g. sched_smp) may need call auto algorithms
 * on its sub-problems with a pre-allocated schedule. Thus, we are still
 * defining them here.
 */

int MPIR_Ibarrier_intra_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibarrier_intra_sched_recursive_doubling(comm_ptr, s);

    return mpi_errno;
}

/* It will choose between several different algorithms based on the given
 * parameters. */
int MPIR_Ibarrier_inter_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno;

    mpi_errno = MPIR_Ibarrier_inter_sched_bcast(comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ibcast_intra_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint type_size, nbytes;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT) {
        mpi_errno = MPIR_Ibcast_intra_sched_smp(buffer, count, datatype, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;

    /* simplistic implementation for now */
    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS)) {
        mpi_errno = MPIR_Ibcast_intra_sched_binomial(buffer, count, datatype, root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (nbytes >= MPIR_CVAR_BCAST_SHORT_MSG_SIZE) && (comm_size >= MPIR_CVAR_BCAST_MIN_PROCS) */

        if ((nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE) && (MPL_is_pof2(comm_size))) {
            mpi_errno =
                MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(buffer, count,
                                                                             datatype, root,
                                                                             comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            mpi_errno =
                MPIR_Ibcast_intra_sched_scatter_ring_allgather(buffer, count, datatype, root,
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
int MPIR_Ibcast_inter_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ibcast_inter_sched_flat(buffer, count, datatype, root, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Igather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igather_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Igather_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint local_size, remote_size;
    MPI_Aint recvtype_size, sendtype_size, nbytes;

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        goto fn_exit;
    }

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * remote_size;
    } else {
        /* remote side */
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * local_size;
    }

    if (nbytes < MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Igather_inter_sched_short(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Igather_inter_sched_long(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                          recvtype, root, comm_ptr, s);
    }

  fn_exit:
    return mpi_errno;
}

int MPIR_Igatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const MPI_Aint recvcounts[],
                                   const MPI_Aint displs[], MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igatherv_allcomm_sched_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Igatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const MPI_Aint recvcounts[],
                                   const MPI_Aint displs[], MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Igatherv_allcomm_sched_linear(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                           displs, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatter_intra_sched_binomial(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                           recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);


  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatter_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int local_size, remote_size;
    MPI_Aint sendtype_size, recvtype_size, nbytes;
    int mpi_errno = MPI_SUCCESS;

    remote_size = comm_ptr->remote_size;
    local_size = comm_ptr->local_size;

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    } else {
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Iscatter_inter_sched_remote_send_local_scatter(sendbuf, sendcount, sendtype,
                                                                recvbuf, recvcount, recvtype, root,
                                                                comm_ptr, s);
    } else {
        mpi_errno = MPIR_Iscatter_inter_sched_linear(sendbuf, sendcount, sendtype,
                                                     recvbuf, recvcount, recvtype, root, comm_ptr,
                                                     s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                    const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iscatterv_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                    const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iscatterv_allcomm_sched_linear(sendbuf, sendcounts, displs, sendtype, recvbuf,
                                            recvcount, recvtype, root, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint recvtype_size, tot_bytes;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
    tot_bytes = (MPI_Aint) recvcount *comm_size * recvtype_size;

    if ((tot_bytes < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) && !(comm_size & (comm_size - 1))) {
        mpi_errno =
            MPIR_Iallgather_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                           recvcount, recvtype, comm_ptr, s);
    } else if (tot_bytes < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        mpi_errno =
            MPIR_Iallgather_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Iallgather_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                             recvtype, comm_ptr, s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgather_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                     MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgather_inter_sched_local_gather_remote_bcast(sendbuf, sendcount,
                                                                      sendtype, recvbuf, recvcount,
                                                                      recvtype, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Iallgatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i, comm_size;
    MPI_Aint total_count, recvtype_size;

    comm_size = comm_ptr->local_size;
    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);

    total_count = 0;
    for (i = 0; i < comm_size; i++)
        total_count += recvcounts[i];

    if (total_count == 0)
        goto fn_exit;

    if ((total_count * recvtype_size < MPIR_CVAR_ALLGATHER_LONG_MSG_SIZE) &&
        !(comm_size & (comm_size - 1))) {
        /* Short or medium size message and power-of-two no. of processes. Use
         * recursive doubling algorithm */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_recursive_doubling(sendbuf, sendcount, sendtype, recvbuf,
                                                            recvcounts, displs, recvtype, comm_ptr,
                                                            s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (total_count * recvtype_size < MPIR_CVAR_ALLGATHER_SHORT_MSG_SIZE) {
        /* Short message and non-power-of-two no. of processes. Use
         * Bruck algorithm (see description above). */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                                displs, recvtype, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* long message or medium-size message and non-power-of-two
         * no. of processes. Use ring algorithm. */
        mpi_errno =
            MPIR_Iallgatherv_intra_sched_ring(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                                              displs, recvtype, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallgatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(sendbuf, sendcount,
                                                                       sendtype, recvbuf,
                                                                       recvcounts, displs, recvtype,
                                                                       comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoall_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size;
    MPI_Aint nbytes, sendtype_size;

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
    nbytes = sendtype_size * sendcount;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_inplace(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                               recvtype, comm_ptr, s);
    } else if ((nbytes <= MPIR_CVAR_ALLTOALL_SHORT_MSG_SIZE) && (comm_size >= 8)) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_brucks(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                              recvtype, comm_ptr, s);
    } else if (nbytes <= MPIR_CVAR_ALLTOALL_MEDIUM_MSG_SIZE) {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_permuted_sendrecv(sendbuf, sendcount, sendtype, recvbuf,
                                                         recvcount, recvtype, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Ialltoall_intra_sched_pairwise(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                                                recvtype, comm_ptr, s);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIR_Ialltoall_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype
                                    sendtype, void *recvbuf, MPI_Aint recvcount,
                                    MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoall_inter_sched_pairwise_exchange(sendbuf, sendcount,
                                                             sendtype, recvbuf, recvcount, recvtype,
                                                             comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoallv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                     const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Ialltoallv_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallv_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                        sendtype, recvbuf, recvcounts,
                                                        rdispls, recvtype, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallv_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                                     const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallv_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                              sendtype, recvbuf, recvcounts,
                                                              rdispls, recvtype, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ialltoallw_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const MPI_Aint recvcounts[],
                                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (sendbuf == MPI_IN_PLACE) {
        mpi_errno = MPIR_Ialltoallw_intra_sched_inplace(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    } else {
        mpi_errno = MPIR_Ialltoallw_intra_sched_blocked(sendbuf, sendcounts, sdispls,
                                                        sendtypes, recvbuf, recvcounts,
                                                        rdispls, recvtypes, comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Ialltoallw_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                     const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                     void *recvbuf, const MPI_Aint recvcounts[],
                                     const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ialltoallw_inter_sched_pairwise_exchange(sendbuf, sendcounts, sdispls,
                                                              sendtypes, recvbuf, recvcounts,
                                                              rdispls, recvtypes, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2;
    MPI_Aint type_size;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT && MPIR_Op_is_commutative(op)) {
        mpi_errno = MPIR_Ireduce_intra_sched_smp(sendbuf, recvbuf, count,
                                                 datatype, op, root, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->coll.pof2;

    if ((count * type_size > MPIR_CVAR_REDUCE_SHORT_MSG_SIZE) &&
        (HANDLE_IS_BUILTIN(op)) && (count >= pof2)) {
        /* do a reduce-scatter followed by gather to root. */
        mpi_errno =
            MPIR_Ireduce_intra_sched_reduce_scatter_gather(sendbuf, recvbuf, count, datatype, op,
                                                           root, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* use a binomial tree algorithm */
        mpi_errno =
            MPIR_Ireduce_intra_sched_binomial(sendbuf, recvbuf, count, datatype, op, root, comm_ptr,
                                              s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op, int root,
                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ireduce_inter_sched_local_reduce_remote_send(sendbuf, recvbuf, count,
                                                                  datatype, op, root, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Iallreduce_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int pof2;

    MPIR_Assert(comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM);

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT && MPIR_Op_is_commutative(op)) {
        mpi_errno =
            MPIR_Iallreduce_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        goto fn_exit;
    }

    MPI_Aint type_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);

    /* get nearest power-of-two less than or equal to number of ranks in the communicator */
    pof2 = comm_ptr->coll.pof2;

    /* If op is user-defined or count is less than pof2, use
     * recursive doubling algorithm. Otherwise do a reduce-scatter
     * followed by allgather. (If op is user-defined,
     * derived datatypes are allowed and the user could pass basic
     * datatypes on one process and derived on another as long as
     * the type maps are the same. Breaking up derived
     * datatypes to do the reduce-scatter is tricky, therefore
     * using recursive doubling in that case.) */

    if ((count * type_size <= MPIR_CVAR_ALLREDUCE_SHORT_MSG_SIZE) ||
        (!HANDLE_IS_BUILTIN(op)) || (count < pof2)) {
        /* use recursive doubling */
        mpi_errno =
            MPIR_Iallreduce_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                           comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        /* do a reduce-scatter followed by allgather */
        mpi_errno =
            MPIR_Iallreduce_intra_sched_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype,
                                                                 op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Iallreduce_inter_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast(sendbuf, recvbuf, count,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_scatter_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int is_commutative;
    MPI_Aint total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = 0;
    for (i = 0; i < comm_size; i++) {
        total_count += recvcounts[i];
    }
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_recursive_halving(sendbuf, recvbuf, recvcounts,
                                                               datatype, op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_intra_sched_pairwise(sendbuf, recvbuf, recvcounts, datatype, op,
                                                      comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (!is_commutative) */

        int is_block_regular = TRUE;
        for (i = 0; i < (comm_size - 1); ++i) {
            if (recvcounts[i] != recvcounts[i + 1]) {
                is_block_regular = FALSE;
                break;
            }
        }

        if (MPL_is_pof2(comm_size) && is_block_regular) {
            /* noncommutative, pof2 size, and block regular */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_noncommutative(sendbuf, recvbuf, recvcounts,
                                                                datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* noncommutative and (non-pof2 or block irregular), use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_intra_sched_recursive_doubling(sendbuf, recvbuf, recvcounts,
                                                                    datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Ireduce_scatter_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint recvcounts[], MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf, recvcounts,
                                                                      datatype, op, comm_ptr, s);

    return mpi_errno;
}

int MPIR_Ireduce_scatter_block_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                                MPI_Aint recvcount, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;
    MPI_Aint total_count, type_size, nbytes;
    int comm_size;

    is_commutative = MPIR_Op_is_commutative(op);

    comm_size = comm_ptr->local_size;
    total_count = recvcount * comm_size;
    if (total_count == 0) {
        goto fn_exit;
    }
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = total_count * type_size;

    /* select an appropriate algorithm based on commutivity and message size */
    if (is_commutative && (nbytes < MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_block_intra_sched_recursive_halving(sendbuf, recvbuf, recvcount,
                                                                     datatype, op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (is_commutative && (nbytes >= MPIR_CVAR_REDUCE_SCATTER_COMMUTATIVE_LONG_MSG_SIZE)) {
        mpi_errno =
            MPIR_Ireduce_scatter_block_intra_sched_pairwise(sendbuf, recvbuf, recvcount, datatype,
                                                            op, comm_ptr, s);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* (!is_commutative) */

        if (MPL_is_pof2(comm_size)) {
            /* noncommutative, pof2 size */
            mpi_errno =
                MPIR_Ireduce_scatter_block_intra_sched_noncommutative(sendbuf, recvbuf, recvcount,
                                                                      datatype, op, comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* noncommutative and non-pof2, use recursive doubling. */
            mpi_errno =
                MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling(sendbuf, recvbuf,
                                                                          recvcount, datatype, op,
                                                                          comm_ptr, s);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIR_Ireduce_scatter_block_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                                MPI_Aint recvcount, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv(sendbuf, recvbuf,
                                                                            recvcount, datatype, op,
                                                                            comm_ptr, s);

    return mpi_errno;
}

int MPIR_Iscan_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT) {
        mpi_errno = MPIR_Iscan_intra_sched_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, s);
    } else {
        mpi_errno =
            MPIR_Iscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                      comm_ptr, s);
    }

    return mpi_errno;
}

int MPIR_Iexscan_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Iexscan_intra_sched_recursive_doubling(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                                    s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_allgather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              MPI_Aint recvcount, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_allgather_allcomm_sched_linear(sendbuf, sendcount, sendtype,
                                                              recvbuf, recvcount, recvtype,
                                                              comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_allgatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_allgatherv_allcomm_sched_linear(sendbuf, sendcount, sendtype,
                                                               recvbuf, recvcounts, displs,
                                                               recvtype, comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoall_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             MPI_Aint recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIR_Ineighbor_alltoall_allcomm_sched_linear(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype, comm_ptr,
                                                             s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallv_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtype,
                                                      recvbuf, recvcounts, rdispls, recvtype,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPIR_Ineighbor_alltoallw_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Ineighbor_alltoallw_allcomm_sched_linear(sendbuf, sendcounts, sdispls, sendtypes,
                                                      recvbuf, recvcounts, rdispls, recvtypes,
                                                      comm_ptr, s);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}
