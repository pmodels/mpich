/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

#include "coll_impl.h"

/* During init, not all algorithms are safe to use. For example, the csel
 * may not have been initialized. We define a set of fallback routines that
 * are safe to use during init. They are all intra algorithms.
 */
#define MPIR_Barrier_fallback    MPIR_Barrier_intra_dissemination
#define MPIR_Allgather_fallback  MPIR_Allgather_intra_brucks
#define MPIR_Allgatherv_fallback MPIR_Allgatherv_intra_brucks
#define MPIR_Allreduce_fallback  MPIR_Allreduce_intra_recursive_doubling


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


/******************************** Allgather ********************************/
/* intracomm-only functions */
int MPIR_Allgather_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_brucks(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_recursive_doubling(const void *sendbuf, MPI_Aint sendcount,
                                            MPI_Datatype sendtype, void *recvbuf,
                                            MPI_Aint recvcount, MPI_Datatype recvtype,
                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgather_intra_ring(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allgather_inter_local_gather_remote_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allgather_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Allgatherv ********************************/
/* intracomm-only functions */
int MPIR_Allgatherv_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, const MPI_Aint * recvcounts,
                                 const MPI_Aint * displs, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_brucks(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, const MPI_Aint * recvcounts,
                                 const MPI_Aint * displs, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_recursive_doubling(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                             MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);
int MPIR_Allgatherv_intra_ring(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allgatherv_inter_remote_gather_local_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const MPI_Aint * recvcounts,
                                                    const MPI_Aint * displs, MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allgatherv_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag);


/******************************** Allreduce ********************************/
/* intracomm-only functions */
int MPIR_Allreduce_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_reduce_scatter_allgather(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Allreduce_intra_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Allreduce_inter_reduce_exchange_bcast(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                               MPI_Datatype datatype, MPI_Op op,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Allreduce_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint count,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);


/******************************** Alltoall ********************************/
/* intracomm-only functions */
int MPIR_Alltoall_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_brucks(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_pairwise(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                 void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_pairwise_sendrecv_replace(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoall_intra_scattered(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoall_inter_pairwise_exchange(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                          MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoall_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Alltoallv ********************************/
/* intracomm-only functions */
int MPIR_Alltoallv_allcomm_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_intra_pairwise_sendrecv_replace(const void *sendbuf, const MPI_Aint * sendcounts,
                                                   const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                                   void *recvbuf, const MPI_Aint * recvcounts,
                                                   const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallv_intra_scattered(const void *sendbuf, const MPI_Aint * sendcounts,
                                   const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                   const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                   MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoallv_inter_pairwise_exchange(const void *sendbuf, const MPI_Aint * sendcounts,
                                           const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                           void *recvbuf, const MPI_Aint * recvcounts,
                                           const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoallv_allcomm_nb(const void *sendbuf, const MPI_Aint * sendcounts,
                              const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                              const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                              MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);


/******************************** Alltoallw ********************************/
/* intracomm-only functions */
int MPIR_Alltoallw_allcomm_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                void *recvbuf, const MPI_Aint * recvcounts,
                                const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_intra_pairwise_sendrecv_replace(const void *sendbuf, const MPI_Aint * sendcounts,
                                                   const MPI_Aint * sdispls,
                                                   const MPI_Datatype * sendtypes, void *recvbuf,
                                                   const MPI_Aint * recvcounts,
                                                   const MPI_Aint * rdispls,
                                                   const MPI_Datatype * recvtypes,
                                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Alltoallw_intra_scattered(const void *sendbuf, const MPI_Aint * sendcounts,
                                   const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                   void *recvbuf, const MPI_Aint * recvcounts,
                                   const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                   MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Alltoallw_inter_pairwise_exchange(const void *sendbuf, const MPI_Aint * sendcounts,
                                           const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                           void *recvbuf, const MPI_Aint * recvcounts,
                                           const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Alltoallw_allcomm_nb(const void *sendbuf, const MPI_Aint * sendcounts,
                              const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                              void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                              const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);


/******************************** Barrier ********************************/
/* intracomm-only functions */
int MPIR_Barrier_allcomm_auto(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Barrier_intra_smp(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Barrier_inter_bcast(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Barrier_allcomm_nb(MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Bcast ********************************/
/* intracomm-only functions */
int MPIR_Bcast_allcomm_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_binomial(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_scatter_recursive_doubling_allgather(void *buffer, MPI_Aint count,
                                                          MPI_Datatype datatype, int root,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_scatter_ring_allgather(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                            int root, MPIR_Comm * comm_ptr,
                                            MPIR_Errflag_t * errflag);
int MPIR_Bcast_intra_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Bcast_inter_remote_send_local_bcast(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                             int root, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Bcast_allcomm_nb(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                          MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Exscan ********************************/
/* intracomm-only functions */
int MPIR_Exscan_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);
int MPIR_Exscan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Exscan_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);


/******************************** Gather ********************************/
/* intracomm-only functions */
int MPIR_Gather_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Gather_intra_binomial(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Gather_inter_linear(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Gather_inter_local_gather_remote_send(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Gather_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                           void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Gatherv ********************************/
/* intracomm-only functions */
int MPIR_Gatherv_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                              MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                              MPIR_Errflag_t * errflag);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Gatherv_allcomm_linear(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag);
int MPIR_Gatherv_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                            MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                            MPIR_Errflag_t * errflag);


/******************************** Iallgather ********************************/
/* request-based functions */
int MPIR_Iallgather_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                       bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p);
int MPIR_Iallgather_sched_impl(const void *sendbuf, MPI_Aint sendcount,
                               MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                               void **sched_p, enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Iallgather_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                               void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                               MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallgather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_sched_brucks(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_sched_recursive_doubling(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_intra_sched_ring(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
/* sched-based intercomm-only functions */
int MPIR_Iallgather_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgather_inter_sched_local_gather_remote_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                          MPI_Datatype sendtype, void *recvbuf,
                                                          MPI_Aint recvcount, MPI_Datatype recvtype,
                                                          MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iallgatherv ********************************/
/* request-based functions */
int MPIR_Iallgatherv_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p);
int MPIR_Iallgatherv_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                                void **sched_p, enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Iallgatherv_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallgatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_sched_brucks(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_sched_recursive_doubling(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    const MPI_Aint recvcounts[],
                                                    const MPI_Aint displs[], MPI_Datatype recvtype,
                                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_intra_sched_ring(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iallgatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf,
                                      const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallgatherv_inter_sched_remote_gather_local_bcast(const void *sendbuf, MPI_Aint sendcount,
                                                           MPI_Datatype sendtype, void *recvbuf,
                                                           const MPI_Aint * recvcounts,
                                                           const MPI_Aint * displs,
                                                           MPI_Datatype recvtype,
                                                           MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iallreduce ********************************/
/* request-based functions */
int MPIR_Iallreduce_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p);

int MPIR_Iallreduce_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               bool is_persistent, void **sched_p,
                               enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Iallreduce_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iallreduce_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched_naive(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                      MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                      MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched_reduce_scatter_allgather(const void *sendbuf, void *recvbuf,
                                                         MPI_Aint count, MPI_Datatype datatype,
                                                         MPI_Op op, MPIR_Comm * comm_ptr,
                                                         MPIR_Sched_t s);
int MPIR_Iallreduce_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iallreduce_inter_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                     MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                     MPIR_Sched_t s);
int MPIR_Iallreduce_inter_sched_remote_reduce_local_bcast(const void *sendbuf, void *recvbuf,
                                                          MPI_Aint count, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Sched_t s);


/******************************** Ialltoall ********************************/
/* request-based functions */
int MPIR_Ialltoall_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                      bool is_persistent, void **sched_p,
                                      enum MPIR_sched_type *sched_type_p);
int MPIR_Ialltoall_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                              enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ialltoall_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoall_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched_brucks(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched_inplace(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched_pairwise(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);
int MPIR_Ialltoall_intra_sched_permuted_sendrecv(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoall_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoall_inter_sched_pairwise_exchange(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ialltoallv ********************************/
/* request-based functions */
int MPIR_Ialltoallv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                       const MPI_Aint * sdispls, MPI_Datatype sendtype,
                                       void *recvbuf, const MPI_Aint * recvcounts,
                                       const MPI_Aint * rdispls, MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p);
int MPIR_Ialltoallv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                               const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf,
                               const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr, bool is_persistent,
                               void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ialltoallv_allcomm_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                 const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                 const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                 MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                 MPIR_Request ** request);

/* sched-based functions */
int MPIR_Ialltoallv_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                               const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                               const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoallv_intra_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                     const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                     const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_sched_blocked(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_intra_sched_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoallv_inter_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                     const MPI_Aint * sdispls, MPI_Datatype sendtype, void *recvbuf,
                                     const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                                     MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallv_inter_sched_pairwise_exchange(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ialltoallw ********************************/
/* request-based functions */
int MPIR_Ialltoallw_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                       const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                       void *recvbuf, const MPI_Aint * recvcounts,
                                       const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                       MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p);
int MPIR_Ialltoallw_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                               const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                               void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                               const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                               bool is_persistent, void **sched_p,
                               enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ialltoallw_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                               const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                               void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * rdispls,
                               const MPI_Datatype * recvtypes, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ialltoallw_intra_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                     const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                     void *recvbuf, const MPI_Aint * recvcounts,
                                     const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_sched_blocked(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_intra_sched_inplace(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ialltoallw_inter_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                     const MPI_Aint * sdispls, const MPI_Datatype * sendtypes,
                                     void *recvbuf, const MPI_Aint * recvcounts,
                                     const MPI_Aint * rdispls, const MPI_Datatype * recvtypes,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ialltoallw_inter_sched_pairwise_exchange(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ibarrier ********************************/
/* request-based functions */
int MPIR_Ibarrier_allcomm_sched_auto(MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                     enum MPIR_sched_type *sched_type_p);
int MPIR_Ibarrier_sched_impl(MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                             enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ibarrier_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ibarrier_intra_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_intra_sched_recursive_doubling(MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ibarrier_inter_sched_auto(MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibarrier_inter_sched_bcast(MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ibcast ********************************/
/* request-based functions */
int MPIR_Ibcast_sched_impl(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, bool is_persistent,
                           void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ibcast_allcomm_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                   MPIR_Comm * comm_ptr, bool is_persistent,
                                   void **sched_p, enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ibcast_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                           MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ibcast_intra_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_intra_sched_binomial(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_intra_sched_scatter_recursive_doubling_allgather(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype, int root,
                                                                 MPIR_Comm * comm_ptr,
                                                                 MPIR_Sched_t s);
int MPIR_Ibcast_intra_sched_scatter_ring_allgather(void *buffer, MPI_Aint count,
                                                   MPI_Datatype datatype, int root,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_intra_sched_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ibcast_inter_sched_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ibcast_inter_sched_flat(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Iexscan ********************************/
/* request-based functions */
int MPIR_Iexscan_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p);
int MPIR_Iexscan_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                            bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p);

/* sched-based intracomm-only functions */
int MPIR_Iexscan_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Iexscan_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                                MPI_Datatype datatype, MPI_Op op,
                                                MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Igather ********************************/
/* request-based functions */
int MPIR_Igather_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                    void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                    int root, MPIR_Comm * comm_ptr, bool is_persistent,
                                    void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Igather_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                            enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Igather_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Igather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_intra_sched_binomial(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                      MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Igather_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_sched_long(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                  void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                  int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Igather_inter_sched_short(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Igatherv ********************************/
/* request-based functions */
int MPIR_Igatherv_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, const MPI_Aint * recvcounts,
                                     const MPI_Aint * displs, MPI_Datatype recvtype, int root,
                                     MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                     enum MPIR_sched_type *sched_type_p);
int MPIR_Igatherv_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const MPI_Aint recvcounts[], const MPI_Aint displs[],
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                             bool is_persistent, void **sched_p,
                             enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Igatherv_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, const MPI_Aint * recvcounts, const MPI_Aint * displs,
                             MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Igatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const MPI_Aint * recvcounts,
                                   const MPI_Aint * displs, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Igatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, const MPI_Aint * recvcounts,
                                   const MPI_Aint * displs, MPI_Datatype recvtype, int root,
                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Igatherv_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf,
                                       const MPI_Aint * recvcounts, const MPI_Aint * displs,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Sched_t s);


/******************************** Ineighbor_allgather ********************************/
/* request-based functions */
int MPIR_Ineighbor_allgather_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                                MPI_Datatype sendtype, void *recvbuf,
                                                MPI_Aint recvcount, MPI_Datatype recvtype,
                                                MPIR_Comm * comm_ptr, bool is_persistent,
                                                void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ineighbor_allgather_sched_impl(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ineighbor_allgather_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                        MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_allgather_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              MPI_Aint recvcount, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_allgather_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                              MPI_Datatype sendtype, void *recvbuf,
                                              MPI_Aint recvcount, MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_allgather_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                                  MPI_Datatype sendtype, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_allgatherv ********************************/
/* request-based functions */
int MPIR_Ineighbor_allgatherv_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 const MPI_Aint recvcounts[],
                                                 const MPI_Aint displs[], MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, bool is_persistent,
                                                 void **sched_p,
                                                 enum MPIR_sched_type *sched_type_p);
int MPIR_Ineighbor_allgatherv_sched_impl(const void *sendbuf, MPI_Aint sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                         bool is_persistent, void **sched_p,
                                         enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ineighbor_allgatherv_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                         MPI_Datatype sendtype, void *recvbuf,
                                         const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                         MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_allgatherv_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_allgatherv_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                               MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                               MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_allgatherv_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                                   MPI_Datatype sendtype, void *recvbuf,
                                                   const MPI_Aint recvcounts[],
                                                   const MPI_Aint displs[], MPI_Datatype recvtype,
                                                   MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_alltoall ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoall_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                               MPI_Datatype sendtype, void *recvbuf,
                                               MPI_Aint recvcount, MPI_Datatype recvtype,
                                               MPIR_Comm * comm_ptr, bool is_persistent,
                                               void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ineighbor_alltoall_sched_impl(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr,
                                       bool is_persistent, void **sched_p,
                                       enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ineighbor_alltoall_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoall_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             MPI_Aint recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoall_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount,
                                             MPI_Datatype sendtype, void *recvbuf,
                                             MPI_Aint recvcount, MPI_Datatype recvtype,
                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoall_allcomm_sched_linear(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_alltoallv ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoallv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                                const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                void *recvbuf, const MPI_Aint recvcounts[],
                                                const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                                MPIR_Comm * comm_ptr, bool is_persistent,
                                                void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ineighbor_alltoallv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ineighbor_alltoallv_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoallv_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoallv_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                              void *recvbuf, const MPI_Aint recvcounts[],
                                              const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoallv_allcomm_sched_linear(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                                  void *recvbuf, const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ineighbor_alltoallw ********************************/
/* request-based functions */
int MPIR_Ineighbor_alltoallw_allcomm_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                                const MPI_Aint sdispls[],
                                                const MPI_Datatype sendtypes[], void *recvbuf,
                                                const MPI_Aint recvcounts[],
                                                const MPI_Aint rdispls[],
                                                const MPI_Datatype recvtypes[],
                                                MPIR_Comm * comm_ptr, bool is_persistent,
                                                void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Ineighbor_alltoallw_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                        enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ineighbor_alltoallw_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                        const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                        void *recvbuf, const MPI_Aint recvcounts[],
                                        const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ineighbor_alltoallw_intra_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ineighbor_alltoallw_inter_sched_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                              const MPI_Aint sdispls[],
                                              const MPI_Datatype sendtypes[], void *recvbuf,
                                              const MPI_Aint recvcounts[], const MPI_Aint rdispls[],
                                              const MPI_Datatype recvtypes[], MPIR_Comm * comm_ptr,
                                              MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Ineighbor_alltoallw_allcomm_sched_linear(const void *sendbuf, const MPI_Aint sendcounts[],
                                                  const MPI_Aint sdispls[],
                                                  const MPI_Datatype sendtypes[], void *recvbuf,
                                                  const MPI_Aint recvcounts[],
                                                  const MPI_Aint rdispls[],
                                                  const MPI_Datatype recvtypes[],
                                                  MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Ireduce ********************************/
/* request-based functions */
int MPIR_Ireduce_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                    MPI_Datatype datatype, MPI_Op op, int root,
                                    MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p);
int MPIR_Ireduce_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                            bool is_persistent, void **sched_p, enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ireduce_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                            MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                            MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Ireduce_intra_sched_binomial(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_sched_reduce_scatter_gather(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint count, MPI_Datatype datatype, MPI_Op op,
                                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                 MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                 MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_inter_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                  MPIR_Sched_t s);
int MPIR_Ireduce_inter_sched_local_reduce_remote_send(const void *sendbuf, void *recvbuf,
                                                      MPI_Aint count, MPI_Datatype datatype,
                                                      MPI_Op op, int root, MPIR_Comm * comm_ptr,
                                                      MPIR_Sched_t s);


/******************************** Ireduce_scatter ********************************/
/* request-based functions */
int MPIR_Ireduce_scatter_allcomm_sched_auto(const void *sendbuf, void *recvbuf,
                                            const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                            MPI_Op op, MPIR_Comm * comm_ptr,
                                            bool is_persistent, void **sched_p,
                                            enum MPIR_sched_type *sched_type_p);
int MPIR_Ireduce_scatter_sched_impl(const void *sendbuf, void *recvbuf, const MPI_Aint recvcounts[],
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    bool is_persistent, void **sched_p,
                                    enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ireduce_scatter_sched_auto(const void *sendbuf, void *recvbuf, const MPI_Aint * recvcounts,
                                    MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                    MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_scatter_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched_noncommutative(const void *sendbuf, void *recvbuf,
                                                    const MPI_Aint * recvcounts,
                                                    MPI_Datatype datatype, MPI_Op op,
                                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched_pairwise(const void *sendbuf, void *recvbuf,
                                              const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                              MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                        const MPI_Aint * recvcounts,
                                                        MPI_Datatype datatype, MPI_Op op,
                                                        MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_intra_sched_recursive_halving(const void *sendbuf, void *recvbuf,
                                                       const MPI_Aint * recvcounts,
                                                       MPI_Datatype datatype, MPI_Op op,
                                                       MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_scatter_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                          const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                          MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_inter_sched_remote_reduce_local_scatterv(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  const MPI_Aint * recvcounts,
                                                                  MPI_Datatype datatype, MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Sched_t s);


/******************************** Ireduce_scatter_block ********************************/
/* request-based functions */
int MPIR_Ireduce_scatter_block_allcomm_sched_auto(const void *sendbuf, void *recvbuf,
                                                  MPI_Aint recvcount, MPI_Datatype datatype,
                                                  MPI_Op op, MPIR_Comm * comm_ptr,
                                                  bool is_persistent, void **sched_p,
                                                  enum MPIR_sched_type *sched_type_p);
int MPIR_Ireduce_scatter_block_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          bool is_persistent, void **sched_p,
                                          enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Ireduce_scatter_block_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                          MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                          MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Ireduce_scatter_block_intra_sched_auto(const void *sendbuf, void *recvbuf,
                                                MPI_Aint recvcount, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched_noncommutative(const void *sendbuf, void *recvbuf,
                                                          MPI_Aint recvcount, MPI_Datatype datatype,
                                                          MPI_Op op, MPIR_Comm * comm_ptr,
                                                          MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched_pairwise(const void *sendbuf, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype datatype,
                                                    MPI_Op op, MPIR_Comm * comm_ptr,
                                                    MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                              MPI_Aint recvcount,
                                                              MPI_Datatype datatype, MPI_Op op,
                                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_intra_sched_recursive_halving(const void *sendbuf, void *recvbuf,
                                                             MPI_Aint recvcount,
                                                             MPI_Datatype datatype, MPI_Op op,
                                                             MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Ireduce_scatter_block_inter_sched_auto(const void *sendbuf, void *recvbuf,
                                                MPI_Aint recvcount, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Ireduce_scatter_block_inter_sched_remote_reduce_local_scatterv(const void *sendbuf,
                                                                        void *recvbuf,
                                                                        MPI_Aint recvcount,
                                                                        MPI_Datatype datatype,
                                                                        MPI_Op op,
                                                                        MPIR_Comm * comm_ptr,
                                                                        MPIR_Sched_t s);


/******************************** Iscan ********************************/
/* request-based functions */
int MPIR_Iscan_allcomm_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                  MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                  bool is_persistent, void **sched_p,
                                  enum MPIR_sched_type *sched_type_p);
int MPIR_Iscan_sched_impl(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                          MPI_Op op, MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                          enum MPIR_sched_type *sched_type_p);

/* sched-based intracomm-only functions */
int MPIR_Iscan_intra_sched_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                MPIR_Sched_t s);
int MPIR_Iscan_intra_sched_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                              MPI_Datatype datatype, MPI_Op op,
                                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscan_intra_sched_smp(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                               MPIR_Sched_t s);

/******************************** Iscatter ********************************/
/* request-based functions */
int MPIR_Iscatter_allcomm_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm_ptr, bool is_persistent,
                                     void **sched_p, enum MPIR_sched_type *sched_type_p);
int MPIR_Iscatter_sched_impl(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                             enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Iscatter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                             void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iscatter_intra_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_intra_sched_binomial(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm_ptr,
                                       MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iscatter_inter_sched_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                   void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter_sched_linear(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                     void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                     int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);
int MPIR_Iscatter_inter_sched_remote_send_local_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                        MPI_Datatype sendtype, void *recvbuf,
                                                        MPI_Aint recvcount, MPI_Datatype recvtype,
                                                        int root, MPIR_Comm * comm_ptr,
                                                        MPIR_Sched_t s);


/******************************** Iscatterv ********************************/
/* request-based functions */
int MPIR_Iscatterv_allcomm_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                      const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                      MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                      MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                                      enum MPIR_sched_type *sched_type_p);
int MPIR_Iscatterv_sched_impl(const void *sendbuf, const MPI_Aint sendcounts[],
                              const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, bool is_persistent, void **sched_p,
                              enum MPIR_sched_type *sched_type_p);

/* sched-based functions */
int MPIR_Iscatterv_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                              const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intracomm-only functions */
int MPIR_Iscatterv_intra_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                    const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based intercomm-only functions */
int MPIR_Iscatterv_inter_sched_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                                    const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                    MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                    MPIR_Comm * comm_ptr, MPIR_Sched_t s);

/* sched-based anycomm functions */
int MPIR_Iscatterv_allcomm_sched_linear(const void *sendbuf, const MPI_Aint * sendcounts,
                                        const MPI_Aint * displs, MPI_Datatype sendtype,
                                        void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                        int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s);


/******************************** Neighbor_allgather ********************************/
/* intracomm-only functions */
int MPIR_Neighbor_allgather_allcomm_auto(const void *sendbuf, MPI_Aint sendcount,
                                         MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                         MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Neighbor_allgather_allcomm_nb(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, MPIR_Comm * comm_ptr);


/******************************** Neighbor_allgatherv ********************************/
/* intracomm-only functions */
int MPIR_Neighbor_allgatherv_allcomm_auto(const void *sendbuf, MPI_Aint sendcount,
                                          MPI_Datatype sendtype, void *recvbuf,
                                          const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                          MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Neighbor_allgatherv_allcomm_nb(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf,
                                        const MPI_Aint recvcounts[], const MPI_Aint displs[],
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoall ********************************/
/* intracomm-only functions */
int MPIR_Neighbor_alltoall_allcomm_auto(const void *sendbuf, MPI_Aint sendcount,
                                        MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                        MPI_Datatype recvtype, MPIR_Comm * comm_ptr);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Neighbor_alltoall_allcomm_nb(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoallv ********************************/
/* intracomm-only functions */
int MPIR_Neighbor_alltoallv_allcomm_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                         const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                         void *recvbuf, const MPI_Aint recvcounts[],
                                         const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                         MPIR_Comm * comm_ptr);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Neighbor_alltoallv_allcomm_nb(const void *sendbuf, const MPI_Aint sendcounts[],
                                       const MPI_Aint sdispls[], MPI_Datatype sendtype,
                                       void *recvbuf, const MPI_Aint recvcounts[],
                                       const MPI_Aint rdispls[], MPI_Datatype recvtype,
                                       MPIR_Comm * comm_ptr);


/******************************** Neighbor_alltoallw ********************************/
/* intracomm-only functions */
int MPIR_Neighbor_alltoallw_allcomm_auto(const void *sendbuf, const MPI_Aint sendcounts[],
                                         const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                         void *recvbuf, const MPI_Aint recvcounts[],
                                         const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                         MPIR_Comm * comm_ptr);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Neighbor_alltoallw_allcomm_nb(const void *sendbuf, const MPI_Aint sendcounts[],
                                       const MPI_Aint sdispls[], const MPI_Datatype sendtypes[],
                                       void *recvbuf, const MPI_Aint recvcounts[],
                                       const MPI_Aint rdispls[], const MPI_Datatype recvtypes[],
                                       MPIR_Comm * comm_ptr);


/******************************** Reduce ********************************/
/* intracomm-only functions */
int MPIR_Reduce_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                             MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                             MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_binomial(const void *sendbuf, void *recvbuf, MPI_Aint count,
                               MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                               MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_reduce_scatter_gather(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                            MPI_Datatype datatype, MPI_Op op, int root,
                                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_intra_smp(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                          MPI_Op op, int root, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_inter_local_reduce_remote_send(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                               MPI_Datatype datatype, MPI_Op op, int root,
                                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, int root, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);


/******************************** Reduce_local ********************************/
int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op);


/******************************** Reduce_scatter ********************************/
/* intracomm-only functions */
int MPIR_Reduce_scatter_allcomm_auto(const void *sendbuf, void *recvbuf,
                                     const MPI_Aint * recvcounts, MPI_Datatype datatype, MPI_Op op,
                                     MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                             const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                             MPI_Op op, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_pairwise(const void *sendbuf, void *recvbuf,
                                       const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                       MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                 const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                                 MPI_Op op, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                const MPI_Aint * recvcounts, MPI_Datatype datatype,
                                                MPI_Op op, MPIR_Comm * comm_ptr,
                                                MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_scatter_inter_remote_reduce_local_scatter(const void *sendbuf, void *recvbuf,
                                                          const MPI_Aint * recvcounts,
                                                          MPI_Datatype datatype, MPI_Op op,
                                                          MPIR_Comm * comm_ptr,
                                                          MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_scatter_allcomm_nb(const void *sendbuf, void *recvbuf, const MPI_Aint * recvcounts,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                   MPIR_Errflag_t * errflag);


/******************************** Reduce_scatter_block ********************************/
/* intracomm-only functions */
int MPIR_Reduce_scatter_block_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                           MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_noncommutative(const void *sendbuf, void *recvbuf,
                                                   MPI_Aint recvcount, MPI_Datatype datatype,
                                                   MPI_Op op, MPIR_Comm * comm_ptr,
                                                   MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_pairwise(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                             MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                             MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_recursive_doubling(const void *sendbuf, void *recvbuf,
                                                       MPI_Aint recvcount, MPI_Datatype datatype,
                                                       MPI_Op op, MPIR_Comm * comm_ptr,
                                                       MPIR_Errflag_t * errflag);
int MPIR_Reduce_scatter_block_intra_recursive_halving(const void *sendbuf, void *recvbuf,
                                                      MPI_Aint recvcount, MPI_Datatype datatype,
                                                      MPI_Op op, MPIR_Comm * comm_ptr,
                                                      MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Reduce_scatter_block_inter_remote_reduce_local_scatter(const void *sendbuf, void *recvbuf,
                                                                MPI_Aint recvcount,
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Reduce_scatter_block_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint recvcount,
                                         MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                         MPIR_Errflag_t * errflag);


/******************************** Scan ********************************/
/* intracomm-only functions */
int MPIR_Scan_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                           MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                           MPIR_Errflag_t * errflag);
int MPIR_Scan_intra_recursive_doubling(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                       MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                       MPIR_Errflag_t * errflag);
int MPIR_Scan_intra_smp(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Scan_allcomm_nb(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                         MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Scatter ********************************/
/* intracomm-only functions */
int MPIR_Scatter_allcomm_auto(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scatter_intra_binomial(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                                void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */
int MPIR_Scatter_inter_linear(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                              void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                              MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scatter_inter_remote_send_local_scatter(const void *sendbuf, MPI_Aint sendcount,
                                                 MPI_Datatype sendtype, void *recvbuf,
                                                 MPI_Aint recvcount, MPI_Datatype recvtype,
                                                 int root, MPIR_Comm * comm_ptr,
                                                 MPIR_Errflag_t * errflag);

/* anycomm functions */
int MPIR_Scatter_allcomm_nb(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                            void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                            MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);


/******************************** Scatterv ********************************/
/* intracomm-only functions */
int MPIR_Scatterv_allcomm_auto(const void *sendbuf, const MPI_Aint * sendcounts,
                               const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                               MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                               MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

/* intercomm-only functions */

/* anycomm functions */
int MPIR_Scatterv_allcomm_linear(const void *sendbuf, const MPI_Aint * sendcounts,
                                 const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                                 MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                                 MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);
int MPIR_Scatterv_allcomm_nb(const void *sendbuf, const MPI_Aint * sendcounts,
                             const MPI_Aint * displs, MPI_Datatype sendtype, void *recvbuf,
                             MPI_Aint recvcount, MPI_Datatype recvtype, int root,
                             MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag);

#endif /* MPIR_COLL_H_INCLUDED */
