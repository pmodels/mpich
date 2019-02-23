/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

#include "coll_impl.h"

/* Internal point-to-point communication for collectives */
/* These functions are used in the implementation of collective and
   other internal operations. They are wrappers around MPID send/recv
   functions. They do sends/receives by setting the context offset to
   MPIR_CONTEXT_INTRA(INTER)_COLL. */
int MPIC_Wait(MPIR_Request * request_ptr, MPIR_Errflag_t * errflag);
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status);

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t * errflag);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request, MPIR_Errflag_t * errflag);
int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                MPIR_Comm * comm_ptr, MPIR_Request ** request, MPIR_Errflag_t * errflag);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status statuses[],
                 MPIR_Errflag_t * errflag);

/* temporary macros just to make this file reads better */
#define _err   MPIR_Errflag_t *errflag
#define _req   MPIR_Request **request
#define _sched MPIR_Sched_t s

/********** Barrier **********/
#define PARAMS_BARRIER \
    MPIR_Comm *comm_ptr
int MPIR_Barrier(PARAMS_BARRIER, _err);
int MPIR_Barrier_impl(PARAMS_BARRIER, _err);
int MPIR_Barrier_intra_auto(PARAMS_BARRIER, _err);
int MPIR_Barrier_intra_dissemination(PARAMS_BARRIER, _err);
int MPIR_Barrier_intra_smp(PARAMS_BARRIER, _err);
int MPIR_Barrier_inter_auto(PARAMS_BARRIER, _err);
int MPIR_Barrier_inter_bcast(PARAMS_BARRIER, _err);
int MPIR_Barrier_allcomm_nb(PARAMS_BARRIER, _err);

/********** Ibarrier **********/
int MPIR_Ibarrier(PARAMS_BARRIER, _req);
int MPIR_Ibarrier_impl(PARAMS_BARRIER, _req);
int MPIR_Ibarrier_intra_recexch(PARAMS_BARRIER, _req);

int MPIR_Ibarrier_sched(PARAMS_BARRIER, _sched);
int MPIR_Ibarrier_sched_impl(PARAMS_BARRIER, _sched);
int MPIR_Ibarrier_sched_intra_auto(PARAMS_BARRIER, _sched);
int MPIR_Ibarrier_sched_intra_recursive_doubling(PARAMS_BARRIER, _sched);
int MPIR_Ibarrier_sched_inter_auto(PARAMS_BARRIER, _sched);
int MPIR_Ibarrier_sched_inter_bcast(PARAMS_BARRIER, _sched);

/********** Bcast **********/
#define PARAMS_BCAST \
    void *buffer, int count, MPI_Datatype datatype, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Bcast(PARAMS_BCAST, _err);
int MPIR_Bcast_impl(PARAMS_BCAST, _err);
int MPIR_Bcast_intra_auto(PARAMS_BCAST, _err);
int MPIR_Bcast_intra_binomial(PARAMS_BCAST, _err);
int MPIR_Bcast_intra_scatter_recursive_doubling_allgather(PARAMS_BCAST, _err);
int MPIR_Bcast_intra_scatter_ring_allgather(PARAMS_BCAST, _err);
int MPIR_Bcast_intra_smp(PARAMS_BCAST, _err);
int MPIR_Bcast_inter_auto(PARAMS_BCAST, _err);
int MPIR_Bcast_inter_remote_send_local_bcast(PARAMS_BCAST, _err);
int MPIR_Bcast_allcomm_nb(PARAMS_BCAST, _err);

/********** Ibcast **********/
int MPIR_Ibcast(PARAMS_BCAST, _req);
int MPIR_Ibcast_impl(PARAMS_BCAST, _req);
int MPIR_Ibcast_intra_tree(PARAMS_BCAST, _req);
int MPIR_Ibcast_intra_scatter_recexch_allgather(PARAMS_BCAST, _req);
int MPIR_Ibcast_intra_ring(PARAMS_BCAST, _req);

int MPIR_Ibcast_sched(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_impl(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_intra_auto(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_intra_binomial(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_intra_scatter_recursive_doubling_allgather(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_intra_scatter_ring_allgather(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_intra_smp(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_inter_auto(PARAMS_BCAST, _sched);
int MPIR_Ibcast_sched_inter_flat(PARAMS_BCAST, _sched);

/********** Gather **********/
#define PARAMS_GATHER \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Gather(PARAMS_GATHER, _err);
int MPIR_Gather_impl(PARAMS_GATHER, _err);
int MPIR_Gather_intra_auto(PARAMS_GATHER, _err);
int MPIR_Gather_intra_binomial(PARAMS_GATHER, _err);
int MPIR_Gather_inter_auto(PARAMS_GATHER, _err);
int MPIR_Gather_inter_linear(PARAMS_GATHER, _err);
int MPIR_Gather_inter_local_gather_remote_send(PARAMS_GATHER, _err);
int MPIR_Gather_allcomm_nb(PARAMS_GATHER, _err);

/********** Igather **********/
int MPIR_Igather(PARAMS_GATHER, _req);
int MPIR_Igather_impl(PARAMS_GATHER, _req);
int MPIR_Igather_intra_tree(PARAMS_GATHER, _req);

int MPIR_Igather_sched(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_impl(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_intra_auto(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_intra_binomial(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_inter_auto(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_inter_long(PARAMS_GATHER, _sched);
int MPIR_Igather_sched_inter_short(PARAMS_GATHER, _sched);

/********** Gatherv **********/
#define PARAMS_GATHERV \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype recvtype, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Gatherv(PARAMS_GATHERV, _err);
int MPIR_Gatherv_impl(PARAMS_GATHERV, _err);
int MPIR_Gatherv_intra_auto(PARAMS_GATHERV, _err);
int MPIR_Gatherv_inter_auto(PARAMS_GATHERV, _err);
int MPIR_Gatherv_allcomm_linear(PARAMS_GATHERV, _err);
int MPIR_Gatherv_allcomm_nb(PARAMS_GATHERV, _err);

/********** Igatherv **********/
int MPIR_Igatherv(PARAMS_GATHERV, _req);
int MPIR_Igatherv_impl(PARAMS_GATHERV, _req);

int MPIR_Igatherv_sched(PARAMS_GATHERV, _sched);
int MPIR_Igatherv_sched_impl(PARAMS_GATHERV, _sched);
int MPIR_Igatherv_sched_intra_auto(PARAMS_GATHERV, _sched);
int MPIR_Igatherv_sched_inter_auto(PARAMS_GATHERV, _sched);
int MPIR_Igatherv_sched_allcomm_linear(PARAMS_GATHERV, _sched);

/********** Scatter **********/
#define PARAMS_SCATTER \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Scatter(PARAMS_SCATTER, _err);
int MPIR_Scatter_impl(PARAMS_SCATTER, _err);
int MPIR_Scatter_intra_auto(PARAMS_SCATTER, _err);
int MPIR_Scatter_intra_binomial(PARAMS_SCATTER, _err);
int MPIR_Scatter_inter_auto(PARAMS_SCATTER, _err);
int MPIR_Scatter_inter_linear(PARAMS_SCATTER, _err);
int MPIR_Scatter_inter_remote_send_local_scatter(PARAMS_SCATTER, _err);
int MPIR_Scatter_allcomm_nb(PARAMS_SCATTER, _err);

/********** Iscatter **********/
int MPIR_Iscatter(PARAMS_SCATTER, _req);
int MPIR_Iscatter_impl(PARAMS_SCATTER, _req);
int MPIR_Iscatter_intra_tree(PARAMS_SCATTER, _req);

int MPIR_Iscatter_sched(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_impl(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_intra_auto(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_intra_binomial(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_inter_auto(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_inter_linear(PARAMS_SCATTER, _sched);
int MPIR_Iscatter_sched_inter_remote_send_local_scatter(PARAMS_SCATTER, _sched);

/********** Scatterv **********/
#define PARAMS_SCATTERV \
    const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype sendtype, \
    void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Scatterv(PARAMS_SCATTERV, _err);
int MPIR_Scatterv_impl(PARAMS_SCATTERV, _err);
int MPIR_Scatterv_intra_auto(PARAMS_SCATTERV, _err);
int MPIR_Scatterv_inter_auto(PARAMS_SCATTERV, _err);
int MPIR_Scatterv_allcomm_linear(PARAMS_SCATTERV, _err);
int MPIR_Scatterv_allcomm_nb(PARAMS_SCATTERV, _err);

/********** Iscatterv **********/
int MPIR_Iscatterv(PARAMS_SCATTERV, _req);
int MPIR_Iscatterv_impl(PARAMS_SCATTERV, _req);

int MPIR_Iscatterv_sched(PARAMS_SCATTERV, _sched);
int MPIR_Iscatterv_sched_impl(PARAMS_SCATTERV, _sched);
int MPIR_Iscatterv_sched_intra_auto(PARAMS_SCATTERV, _sched);
int MPIR_Iscatterv_sched_inter_auto(PARAMS_SCATTERV, _sched);
int MPIR_Iscatterv_sched_allcomm_linear(PARAMS_SCATTERV, _sched);

/********** Allgather **********/
#define PARAMS_ALLGATHER \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, int recvcount, MPI_Datatype recvtype, \
    MPIR_Comm *comm_ptr
int MPIR_Allgather(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_impl(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_intra_auto(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_intra_brucks(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_intra_recursive_doubling(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_intra_ring(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_inter_auto(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_inter_local_gather_remote_bcast(PARAMS_ALLGATHER, _err);
int MPIR_Allgather_allcomm_nb(PARAMS_ALLGATHER, _err);

/********** Iallgather **********/
int MPIR_Iallgather(PARAMS_ALLGATHER, _req);
int MPIR_Iallgather_impl(PARAMS_ALLGATHER, _req);
int MPIR_Iallgather_intra_gentran_brucks(PARAMS_ALLGATHER, _req);
int MPIR_Iallgather_intra_recexch_distance_doubling(PARAMS_ALLGATHER, _req);
int MPIR_Iallgather_intra_recexch_distance_halving(PARAMS_ALLGATHER, _req);
int MPIR_Iallgather_intra_gentran_ring(PARAMS_ALLGATHER, _req);

int MPIR_Iallgather_sched(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_impl(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_intra_auto(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_intra_brucks(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_intra_recursive_doubling(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_intra_ring(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_inter_auto(PARAMS_ALLGATHER, _sched);
int MPIR_Iallgather_sched_inter_local_gather_remote_bcast(PARAMS_ALLGATHER, _sched);

/********** Allgatherv **********/
#define PARAMS_ALLGATHERV \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype recvtype, \
    MPIR_Comm *comm_ptr
int MPIR_Allgatherv(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_impl(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_intra_auto(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_intra_brucks(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_intra_recursive_doubling(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_intra_ring(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_inter_auto(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_inter_remote_gather_local_bcast(PARAMS_ALLGATHERV, _err);
int MPIR_Allgatherv_allcomm_nb(PARAMS_ALLGATHERV, _err);

/********** Iallgatherv **********/
int MPIR_Iallgatherv(PARAMS_ALLGATHERV, _req);
int MPIR_Iallgatherv_impl(PARAMS_ALLGATHERV, _req);
int MPIR_Iallgatherv_intra_gentran_brucks(PARAMS_ALLGATHERV, _req);
int MPIR_Iallgatherv_intra_recexch_distance_doubling(PARAMS_ALLGATHERV, _req);
int MPIR_Iallgatherv_intra_recexch_distance_halving(PARAMS_ALLGATHERV, _req);
int MPIR_Iallgatherv_intra_gentran_ring(PARAMS_ALLGATHERV, _req);

int MPIR_Iallgatherv_sched(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_impl(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_intra_auto(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_intra_brucks(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_intra_recursive_doubling(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_intra_ring(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_inter_auto(PARAMS_ALLGATHERV, _sched);
int MPIR_Iallgatherv_sched_inter_remote_gather_local_bcast(PARAMS_ALLGATHERV, _sched);

/********** Alltoall **********/
#define PARAMS_ALLTOALL \
    const void *sendbuf, int sendcount, MPI_Datatype sendtype, \
    void *recvbuf, int recvcount, MPI_Datatype recvtype, \
    MPIR_Comm *comm_ptr
int MPIR_Alltoall(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_impl(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_intra_auto(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_intra_brucks(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_intra_pairwise(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_intra_scattered(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_inter_auto(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_inter_pairwise_exchange(PARAMS_ALLTOALL, _err);
int MPIR_Alltoall_allcomm_nb(PARAMS_ALLTOALL, _err);

/********** Ialltoall **********/
int MPIR_Ialltoall(PARAMS_ALLTOALL, _req);
int MPIR_Ialltoall_impl(PARAMS_ALLTOALL, _req);
int MPIR_Ialltoall_intra_gentran_ring(PARAMS_ALLTOALL, _req);
int MPIR_Ialltoall_intra_gentran_brucks(PARAMS_ALLTOALL, _req);

int MPIR_Ialltoall_sched(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_impl(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_intra_auto(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_intra_brucks(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_intra_inplace(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_intra_pairwise(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_intra_permuted_sendrecv(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_inter_auto(PARAMS_ALLTOALL, _sched);
int MPIR_Ialltoall_sched_inter_pairwise_exchange(PARAMS_ALLTOALL, _sched);

/********** Alltoallv **********/
#define PARAMS_ALLTOALLV \
    const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype sendtype, \
    void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype recvtype, \
    MPIR_Comm *comm_ptr
int MPIR_Alltoallv(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_impl(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_intra_auto(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_intra_scattered(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_inter_auto(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_inter_pairwise_exchange(PARAMS_ALLTOALLV, _err);
int MPIR_Alltoallv_allcomm_nb(PARAMS_ALLTOALLV, _err);

/********** Ialltoallv **********/
int MPIR_Ialltoallv(PARAMS_ALLTOALLV, _req);
int MPIR_Ialltoallv_impl(PARAMS_ALLTOALLV, _req);
int MPIR_Ialltoallv_intra_gentran_scattered(PARAMS_ALLTOALLV, _req);

int MPIR_Ialltoallv_sched(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_impl(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_intra_auto(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_intra_blocked(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_intra_inplace(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_inter_auto(PARAMS_ALLTOALLV, _sched);
int MPIR_Ialltoallv_sched_inter_pairwise_exchange(PARAMS_ALLTOALLV, _sched);

/********** Alltoallw **********/
#define PARAMS_ALLTOALLW \
    const void *sendbuf, const int *sendcnts, const int *sdispls, const MPI_Datatype *sendtypes, \
    void *recvbuf, const int *recvcnts, const int *rdispls, const MPI_Datatype *recvtypes, \
    MPIR_Comm *comm_ptr
int MPIR_Alltoallw(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_impl(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_intra_auto(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_intra_scattered(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_inter_auto(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_inter_pairwise_exchange(PARAMS_ALLTOALLW, _err);
int MPIR_Alltoallw_allcomm_nb(PARAMS_ALLTOALLW, _err);

/********** Ialltoallw **********/
int MPIR_Ialltoallw(PARAMS_ALLTOALLW, _req);
int MPIR_Ialltoallw_impl(PARAMS_ALLTOALLW, _req);

int MPIR_Ialltoallw_sched(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_impl(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_intra_auto(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_intra_blocked(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_intra_inplace(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_inter_auto(PARAMS_ALLTOALLW, _sched);
int MPIR_Ialltoallw_sched_inter_pairwise_exchange(PARAMS_ALLTOALLW, _sched);

/********** Reduce **********/
#define PARAMS_REDUCE \
    const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, \
    MPI_Op op, int root, \
    MPIR_Comm *comm_ptr
int MPIR_Reduce(PARAMS_REDUCE, _err);
int MPIR_Reduce_impl(PARAMS_REDUCE, _err);
int MPIR_Reduce_intra_auto(PARAMS_REDUCE, _err);
int MPIR_Reduce_intra_binomial(PARAMS_REDUCE, _err);
int MPIR_Reduce_intra_reduce_scatter_gather(PARAMS_REDUCE, _err);
int MPIR_Reduce_intra_smp(PARAMS_REDUCE, _err);
int MPIR_Reduce_inter_auto(PARAMS_REDUCE, _err);
int MPIR_Reduce_inter_local_reduce_remote_send(PARAMS_REDUCE, _err);
int MPIR_Reduce_allcomm_nb(PARAMS_REDUCE, _err);

/********** Ireduce **********/
int MPIR_Ireduce(PARAMS_REDUCE, _req);
int MPIR_Ireduce_impl(PARAMS_REDUCE, _req);
int MPIR_Ireduce_intra_tree(PARAMS_REDUCE, _req);
int MPIR_Ireduce_intra_ring(PARAMS_REDUCE, _req);

int MPIR_Ireduce_sched(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_impl(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_intra_auto(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_intra_binomial(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_intra_reduce_scatter_gather(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_intra_smp(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_inter_auto(PARAMS_REDUCE, _sched);
int MPIR_Ireduce_sched_inter_local_reduce_remote_send(PARAMS_REDUCE, _sched);

/********** Allreduce **********/
#define PARAMS_ALLREDUCE \
    const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, \
    MPIR_Comm *comm_ptr
int MPIR_Allreduce(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_impl(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_intra_auto(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_intra_recursive_doubling(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_intra_reduce_scatter_allgather(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_intra_smp(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_inter_auto(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_inter_reduce_exchange_bcast(PARAMS_ALLREDUCE, _err);
int MPIR_Allreduce_allcomm_nb(PARAMS_ALLREDUCE, _err);

/********** Iallreduce **********/
int MPIR_Iallreduce(PARAMS_ALLREDUCE, _req);
int MPIR_Iallreduce_impl(PARAMS_ALLREDUCE, _req);
int MPIR_Iallreduce_intra_recexch_single_buffer(PARAMS_ALLREDUCE, _req);
int MPIR_Iallreduce_intra_recexch_multiple_buffer(PARAMS_ALLREDUCE, _req);
int MPIR_Iallreduce_intra_tree(PARAMS_ALLREDUCE, _req);
int MPIR_Iallreduce_intra_ring(PARAMS_ALLREDUCE, _req);

int MPIR_Iallreduce_sched(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_impl(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_intra_auto(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_intra_naive(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_intra_recursive_doubling(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_intra_reduce_scatter_allgather(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_intra_smp(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_inter_auto(PARAMS_ALLREDUCE, _sched);
int MPIR_Iallreduce_sched_inter_remote_reduce_local_bcast(PARAMS_ALLREDUCE, _sched);

/********** Reduce_scatter **********/
#define PARAMS_REDUCE_SCATTER \
    const void *sendbuf, void *recvbuf, const int *recvcnts, MPI_Datatype datatype, MPI_Op op, \
    MPIR_Comm *comm_ptr
int MPIR_Reduce_scatter(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_impl(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_intra_auto(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_intra_noncommutative(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_intra_pairwise(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_intra_recursive_doubling(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_intra_recursive_halving(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_inter_auto(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(PARAMS_REDUCE_SCATTER, _err);
int MPIR_Reduce_scatter_allcomm_nb(PARAMS_REDUCE_SCATTER, _err);

/********** Ireduce_scatter **********/
int MPIR_Ireduce_scatter(PARAMS_REDUCE_SCATTER, _req);
int MPIR_Ireduce_scatter_impl(PARAMS_REDUCE_SCATTER, _req);
int MPIR_Ireduce_scatter_intra_recexch(PARAMS_REDUCE_SCATTER, _req);

int MPIR_Ireduce_scatter_sched(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_impl(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_intra_auto(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_intra_noncommutative(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_intra_pairwise(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_intra_recursive_doubling(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_intra_recursive_halving(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_inter_auto(PARAMS_REDUCE_SCATTER, _sched);
int MPIR_Ireduce_scatter_sched_inter_remote_reduce_local_scatterv(PARAMS_REDUCE_SCATTER, _sched);

/********** Reduce_scatter_block **********/
#define PARAMS_REDUCE_SCATTER_BLOCK \
    const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, \
    MPIR_Comm *comm_ptr
int MPIR_Reduce_scatter_block(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_impl(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_intra_auto(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_intra_noncommutative(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_intra_pairwise(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_intra_recursive_doubling(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_intra_recursive_halving(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_inter_auto(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter(PARAMS_REDUCE_SCATTER_BLOCK, _err);
int MPIR_Reduce_scatter_block_allcomm_nb(PARAMS_REDUCE_SCATTER_BLOCK, _err);

/********** Ireduce_scatter_block **********/
int MPIR_Ireduce_scatter_block(PARAMS_REDUCE_SCATTER_BLOCK, _req);
int MPIR_Ireduce_scatter_block_impl(PARAMS_REDUCE_SCATTER_BLOCK, _req);
int MPIR_Ireduce_scatter_block_intra_recexch(PARAMS_REDUCE_SCATTER_BLOCK, _req);

int MPIR_Ireduce_scatter_block_sched(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_impl(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_intra_auto(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_intra_noncommutative(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_intra_pairwise(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_intra_recursive_doubling(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_intra_recursive_halving(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_inter_auto(PARAMS_REDUCE_SCATTER_BLOCK, _sched);
int MPIR_Ireduce_scatter_block_sched_inter_remote_reduce_local_scatterv(PARAMS_REDUCE_SCATTER_BLOCK, _sched);

/********** Scan **********/
#define PARAMS_SCAN \
    const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, \
    MPIR_Comm *comm_ptr
int MPIR_Scan(PARAMS_SCAN, _err);
int MPIR_Scan_impl(PARAMS_SCAN, _err);
int MPIR_Scan_intra_auto(PARAMS_SCAN, _err);
int MPIR_Scan_intra_recursive_doubling(PARAMS_SCAN, _err);
int MPIR_Scan_intra_smp(PARAMS_SCAN, _err);
int MPIR_Scan_allcomm_nb(PARAMS_SCAN, _err);

/********** Iscan **********/
int MPIR_Iscan(PARAMS_SCAN, _req);
int MPIR_Iscan_impl(PARAMS_SCAN, _req);
int MPIR_Iscan_intra_gentran_recursive_doubling(PARAMS_SCAN, _req);

int MPIR_Iscan_sched(PARAMS_SCAN, _sched);
int MPIR_Iscan_sched_impl(PARAMS_SCAN, _sched);
int MPIR_Iscan_sched_intra_auto(PARAMS_SCAN, _sched);
int MPIR_Iscan_sched_intra_recursive_doubling(PARAMS_SCAN, _sched);
int MPIR_Iscan_sched_intra_smp(PARAMS_SCAN, _sched);

/********** Exscan **********/
#define PARAMS_EXSCAN \
    const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, \
    MPIR_Comm *comm_ptr
int MPIR_Exscan(PARAMS_EXSCAN, _err);
int MPIR_Exscan_impl(PARAMS_EXSCAN, _err);
int MPIR_Exscan_intra_auto(PARAMS_EXSCAN, _err);
int MPIR_Exscan_intra_recursive_doubling(PARAMS_EXSCAN, _err);
int MPIR_Exscan_allcomm_nb(PARAMS_EXSCAN, _err);

/********** Iexscan **********/
int MPIR_Iexscan(PARAMS_EXSCAN, _req);
int MPIR_Iexscan_impl(PARAMS_EXSCAN, _req);

int MPIR_Iexscan_sched(PARAMS_EXSCAN, _sched);
int MPIR_Iexscan_sched_impl(PARAMS_EXSCAN, _sched);
int MPIR_Iexscan_sched_intra_auto(PARAMS_EXSCAN, _sched);
int MPIR_Iexscan_sched_intra_recursive_doubling(PARAMS_EXSCAN, _sched);

/********** Neighbor_allgather **********/
int MPIR_Neighbor_allgather(PARAMS_ALLGATHER);
int MPIR_Neighbor_allgather_impl(PARAMS_ALLGATHER);
int MPIR_Neighbor_allgather_intra_auto(PARAMS_ALLGATHER);
int MPIR_Neighbor_allgather_inter_auto(PARAMS_ALLGATHER);
int MPIR_Neighbor_allgather_allcomm_nb(PARAMS_ALLGATHER);

int MPIR_Ineighbor_allgather(PARAMS_ALLGATHER, _req);
int MPIR_Ineighbor_allgather_impl(PARAMS_ALLGATHER, _req);
int MPIR_Ineighbor_allgather_allcomm_gentran_linear(PARAMS_ALLGATHER, _req);

int MPIR_Ineighbor_allgather_sched(PARAMS_ALLGATHER, _sched);
int MPIR_Ineighbor_allgather_sched_impl(PARAMS_ALLGATHER, _sched);
int MPIR_Ineighbor_allgather_sched_intra_auto(PARAMS_ALLGATHER, _sched);
int MPIR_Ineighbor_allgather_sched_inter_auto(PARAMS_ALLGATHER, _sched);
int MPIR_Ineighbor_allgather_sched_allcomm_linear(PARAMS_ALLGATHER, _sched);

/********** Neighbor_allgatherv **********/
int MPIR_Neighbor_allgatherv(PARAMS_ALLGATHERV);
int MPIR_Neighbor_allgatherv_impl(PARAMS_ALLGATHERV);
int MPIR_Neighbor_allgatherv_intra_auto(PARAMS_ALLGATHERV);
int MPIR_Neighbor_allgatherv_inter_auto(PARAMS_ALLGATHERV);
int MPIR_Neighbor_allgatherv_allcomm_nb(PARAMS_ALLGATHERV);

int MPIR_Ineighbor_allgatherv(PARAMS_ALLGATHERV, _req);
int MPIR_Ineighbor_allgatherv_impl(PARAMS_ALLGATHERV, _req);
int MPIR_Ineighbor_allgatherv_allcomm_gentran_linear(PARAMS_ALLGATHERV, _req);

int MPIR_Ineighbor_allgatherv_sched(PARAMS_ALLGATHERV, _sched);
int MPIR_Ineighbor_allgatherv_sched_impl(PARAMS_ALLGATHERV, _sched);
int MPIR_Ineighbor_allgatherv_sched_intra_auto(PARAMS_ALLGATHERV, _sched);
int MPIR_Ineighbor_allgatherv_sched_inter_auto(PARAMS_ALLGATHERV, _sched);
int MPIR_Ineighbor_allgatherv_sched_allcomm_linear(PARAMS_ALLGATHERV, _sched);

/********** Neighbor_alltoall **********/
int MPIR_Neighbor_alltoall(PARAMS_ALLTOALL);
int MPIR_Neighbor_alltoall_impl(PARAMS_ALLTOALL);
int MPIR_Neighbor_alltoall_intra_auto(PARAMS_ALLTOALL);
int MPIR_Neighbor_alltoall_inter_auto(PARAMS_ALLTOALL);
int MPIR_Neighbor_alltoall_allcomm_nb(PARAMS_ALLTOALL);

int MPIR_Ineighbor_alltoall(PARAMS_ALLTOALL, _req);
int MPIR_Ineighbor_alltoall_impl(PARAMS_ALLTOALL, _req);
int MPIR_Ineighbor_alltoall_allcomm_gentran_linear(PARAMS_ALLTOALL, _req);

int MPIR_Ineighbor_alltoall_sched(PARAMS_ALLTOALL, _sched);
int MPIR_Ineighbor_alltoall_sched_impl(PARAMS_ALLTOALL, _sched);
int MPIR_Ineighbor_alltoall_sched_intra_auto(PARAMS_ALLTOALL, _sched);
int MPIR_Ineighbor_alltoall_sched_inter_auto(PARAMS_ALLTOALL, _sched);
int MPIR_Ineighbor_alltoall_sched_allcomm_linear(PARAMS_ALLTOALL, _sched);

/********** Neighbor_alltoallv **********/
int MPIR_Neighbor_alltoallv(PARAMS_ALLTOALLV);
int MPIR_Neighbor_alltoallv_impl(PARAMS_ALLTOALLV);
int MPIR_Neighbor_alltoallv_intra_auto(PARAMS_ALLTOALLV);
int MPIR_Neighbor_alltoallv_inter_auto(PARAMS_ALLTOALLV);
int MPIR_Neighbor_alltoallv_allcomm_nb(PARAMS_ALLTOALLV);

int MPIR_Ineighbor_alltoallv(PARAMS_ALLTOALLV, _req);
int MPIR_Ineighbor_alltoallv_impl(PARAMS_ALLTOALLV, _req);
int MPIR_Ineighbor_alltoallv_allcomm_gentran_linear(PARAMS_ALLTOALLV, _req);

int MPIR_Ineighbor_alltoallv_sched(PARAMS_ALLTOALLV, _sched);
int MPIR_Ineighbor_alltoallv_sched_impl(PARAMS_ALLTOALLV, _sched);
int MPIR_Ineighbor_alltoallv_sched_intra_auto(PARAMS_ALLTOALLV, _sched);
int MPIR_Ineighbor_alltoallv_sched_inter_auto(PARAMS_ALLTOALLV, _sched);
int MPIR_Ineighbor_alltoallv_sched_allcomm_linear(PARAMS_ALLTOALLV, _sched);

/********** Neighbor_alltoallw **********/
#define PARAMS_NEIGHBOR_ALLTOALLW \
    const void *sendbuf, const int *sendcnts, const MPI_Aint sdispls[], const MPI_Datatype *sendtypes, \
    void *recvbuf, const int *recvcnts, const MPI_Aint rdispls[], const MPI_Datatype *recvtypes, \
    MPIR_Comm *comm_ptr
int MPIR_Neighbor_alltoallw(PARAMS_NEIGHBOR_ALLTOALLW);
int MPIR_Neighbor_alltoallw_impl(PARAMS_NEIGHBOR_ALLTOALLW);
int MPIR_Neighbor_alltoallw_intra_auto(PARAMS_NEIGHBOR_ALLTOALLW);
int MPIR_Neighbor_alltoallw_inter_auto(PARAMS_NEIGHBOR_ALLTOALLW);
int MPIR_Neighbor_alltoallw_allcomm_nb(PARAMS_NEIGHBOR_ALLTOALLW);

int MPIR_Ineighbor_alltoallw(PARAMS_NEIGHBOR_ALLTOALLW, _req);
int MPIR_Ineighbor_alltoallw_impl(PARAMS_NEIGHBOR_ALLTOALLW, _req);
int MPIR_Ineighbor_alltoallw_allcomm_gentran_linear(PARAMS_NEIGHBOR_ALLTOALLW, _req);

int MPIR_Ineighbor_alltoallw_sched(PARAMS_NEIGHBOR_ALLTOALLW, _sched);
int MPIR_Ineighbor_alltoallw_sched_impl(PARAMS_NEIGHBOR_ALLTOALLW, _sched);
int MPIR_Ineighbor_alltoallw_sched_intra_auto(PARAMS_NEIGHBOR_ALLTOALLW, _sched);
int MPIR_Ineighbor_alltoallw_sched_inter_auto(PARAMS_NEIGHBOR_ALLTOALLW, _sched);
int MPIR_Ineighbor_alltoallw_sched_allcomm_linear(PARAMS_NEIGHBOR_ALLTOALLW, _sched);

/* undef temporary macros */
#undef _err
#undef _req
#undef _sched

/******************************** Reduce_local ********************************/
int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype,
                      MPI_Op op);


#endif /* MPIR_COLL_H_INCLUDED */
