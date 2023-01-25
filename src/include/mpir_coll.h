/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

#include "coll_impl.h"
#include "coll_algos.h"

/* collective attr bits allocation:
 * 0-7:   errflag
 * 8-15:  subcomm type
 * 16-23: subcomm index
 */

#define MPIR_COLL_ATTR_NONE 0
#define MPIR_COLL_ATTR_GET_ERRFLAG(attr) ((attr) & 0xff)
#define MPIR_COLL_ATTR_GET_SUBCOMM_TYPE(attr) (((attr) >> 8) & 0xff)
#define MPIR_COLL_ATTR_GET_SUBCOMM_INDEX(attr) (((attr) >> 16) & 0xff)

#define MPIR_COLL_SUBCOMM_TYPE_NONE 0
#define MPIR_COLL_SUBCOMM_TYPE_CHILD 1
#define MPIR_COLL_SUBCOMM_TYPE_ROOTS 2

#define MPIR_COLL_GET_RANK_SIZE(comm_ptr, collattr, rank_, size_) \
    do { \
        int subcomm_type = MPIR_COLL_ATTR_GET_SUBCOMM_TYPE(collattr); \
        if (subcomm_type) { \
            int subcomm_index = MPIR_COLL_ATTR_GET_SUBCOMM_INDEX(collattr); \
            switch(subcomm_type) { \
                case MPIR_COLL_SUBCOMM_TYPE_CHILD: \
                    rank_ = comm_ptr->child_subcomm[subcomm_index].rank; \
                    size_ = comm_ptr->child_subcomm[subcomm_index].size; \
                    break; \
                case MPIR_COLL_SUBCOMM_TYPE_ROOTS: \
                    rank_ = comm_ptr->roots_subcomm[subcomm_index].rank; \
                    size_ = comm_ptr->roots_subcomm[subcomm_index].size; \
                    break; \
                default: \
                    MPIR_Assert(0); \
                    rank_ = -1; \
                    size_ = 0; \
            } \
        } else { \
            rank_ = (comm_ptr)->rank; \
            size_ = (comm_ptr)->local_size; \
        } \
    } while (0)


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
int MPIC_Wait(MPIR_Request * request_ptr);
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status);

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, int collattr);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, int collattr, MPI_Status * status);
int MPIC_Ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, int collattr);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, int collattr);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, int collattr);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request, int collattr);
int MPIC_Issend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
                MPIR_Comm * comm_ptr, MPIR_Request ** request, int collattr);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, int collattr, MPIR_Request ** request);
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status statuses[]);

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op);

int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, int collattr);

/* TSP auto */
int MPIR_TSP_Iallreduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, int collattr,
                                             MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, int collattr,
                                         MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibarrier_sched_intra_tsp_auto(MPIR_Comm * comm, int collattr, MPIR_TSP_sched_t sched);
int MPIR_TSP_Ireduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, int collattr,
                                          MPIR_TSP_sched_t sched);
#endif /* MPIR_COLL_H_INCLUDED */
