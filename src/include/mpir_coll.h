/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

#include "coll_impl.h"
#include "coll_algos.h"
#include "mpir_threadcomm.h"

#ifdef ENABLE_THREADCOMM
#define MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_) do { \
        MPIR_Threadcomm *threadcomm = (comm)->threadcomm; \
        MPIR_Assert(threadcomm); \
        int intracomm_size = (comm)->local_size; \
        size_ = threadcomm->rank_offset_table[intracomm_size - 1]; \
        rank_ = MPIR_THREADCOMM_TID_TO_RANK(threadcomm, MPIR_threadcomm_get_tid(threadcomm)); \
    } while (0)
#else
#define MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_) do { \
        MPIR_Assert(0); \
        size_ = 0; \
        rank_ = -1; \
    } while (0)
#endif

#define MPIR_COLL_RANK_SIZE(comm, coll_group, rank_, size_) do { \
        if (coll_group == MPIR_SUBGROUP_NONE) { \
            rank_ = (comm)->rank; \
            size_ = (comm)->local_size; \
        } else if (coll_group == MPIR_SUBGROUP_THREADCOMM) { \
            MPIR_THREADCOMM_RANK_SIZE(comm, rank_, size_); \
        } else { \
            rank_ = (comm)->subgroups[coll_group].rank; \
            size_ = (comm)->subgroups[coll_group].size; \
        } \
    } while (0)

/* sometime it is convenient to just get the rank or size */
static inline int MPIR_Coll_size(MPIR_Comm * comm, int coll_group)
{
    int rank, size;
    MPIR_COLL_RANK_SIZE(comm, coll_group, rank, size);
    (void) rank;
    return size;
}

static inline int MPIR_Coll_rank(MPIR_Comm * comm, int coll_group)
{
    int rank, size;
    MPIR_COLL_RANK_SIZE(comm, coll_group, rank, size);
    (void) size;
    return rank;
}

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
              MPIR_Comm * comm_ptr, MPIR_Errflag_t errflag);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t errflag);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, MPIR_Errflag_t errflag);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request, MPIR_Errflag_t errflag);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status * statuses);

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op);

int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag);

/* TSP auto */
int MPIR_TSP_Iallreduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, int coll_group,
                                             MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, int coll_group,
                                         MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibarrier_sched_intra_tsp_auto(MPIR_Comm * comm, int coll_group,
                                           MPIR_TSP_sched_t sched);
int MPIR_TSP_Ireduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, int coll_group,
                                          MPIR_TSP_sched_t sched);
#endif /* MPIR_COLL_H_INCLUDED */
