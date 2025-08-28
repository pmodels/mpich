/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_COLL_H_INCLUDED
#define MPIR_COLL_H_INCLUDED

typedef struct MPIR_Csel_coll_sig MPIR_Csel_coll_sig_s;
typedef struct MPII_Csel_container MPII_Csel_container_s;

#include "coll_impl.h"
#include "coll_algos.h"

typedef enum {
    MPIR_CSEL_COLL_TYPE__ALLGATHER = 0,
    MPIR_CSEL_COLL_TYPE__ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__ALLREDUCE,
    MPIR_CSEL_COLL_TYPE__ALLTOALL,
    MPIR_CSEL_COLL_TYPE__ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__BARRIER,
    MPIR_CSEL_COLL_TYPE__BCAST,
    MPIR_CSEL_COLL_TYPE__EXSCAN,
    MPIR_CSEL_COLL_TYPE__GATHER,
    MPIR_CSEL_COLL_TYPE__GATHERV,
    MPIR_CSEL_COLL_TYPE__IALLGATHER,
    MPIR_CSEL_COLL_TYPE__IALLGATHERV,
    MPIR_CSEL_COLL_TYPE__IALLREDUCE,
    MPIR_CSEL_COLL_TYPE__IALLTOALL,
    MPIR_CSEL_COLL_TYPE__IALLTOALLV,
    MPIR_CSEL_COLL_TYPE__IALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IBARRIER,
    MPIR_CSEL_COLL_TYPE__IBCAST,
    MPIR_CSEL_COLL_TYPE__IEXSCAN,
    MPIR_CSEL_COLL_TYPE__IGATHER,
    MPIR_CSEL_COLL_TYPE__IGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__INEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__IREDUCE,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__ISCAN,
    MPIR_CSEL_COLL_TYPE__ISCATTER,
    MPIR_CSEL_COLL_TYPE__ISCATTERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHER,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLGATHERV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALL,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLV,
    MPIR_CSEL_COLL_TYPE__NEIGHBOR_ALLTOALLW,
    MPIR_CSEL_COLL_TYPE__REDUCE,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER,
    MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK,
    MPIR_CSEL_COLL_TYPE__SCAN,
    MPIR_CSEL_COLL_TYPE__SCATTER,
    MPIR_CSEL_COLL_TYPE__SCATTERV,
    MPIR_CSEL_COLL_TYPE__END,
} MPIR_Csel_coll_type_e;

typedef enum {
    MPIR_COLL_ALGORITHM_IDS(),
    /* composition algorithms */
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Coll_auto,
    /* end */
    MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count,
} MPII_Csel_container_type_e;

struct MPIR_Csel_coll_sig {
    MPIR_Csel_coll_type_e coll_type;
    MPIR_Comm *comm_ptr;
    void *sched;
    enum MPIR_sched_type sched_type;
    bool is_persistent;

    union {
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype recvtype;
        } allgather, iallgather, neighbor_allgather, ineighbor_allgather;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
        } allgatherv, iallgatherv, neighbor_allgatherv, ineighbor_allgatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } allreduce, iallreduce;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
        } alltoall, ialltoall, neighbor_alltoall, ineighbor_alltoall;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            MPI_Datatype recvtype;
        } alltoallv, ialltoallv, neighbor_alltoallv, ineighbor_alltoallv;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } alltoallw, ialltoallw;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *sdispls;
            const MPI_Datatype *sendtypes;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *rdispls;
            const MPI_Datatype *recvtypes;
        } neighbor_alltoallw, ineighbor_alltoallw;
        struct {
            int dummy;          /* some compiler (suncc) doesn't like empty struct */
        } barrier, ibarrier;
        struct {
            void *buffer;
            MPI_Aint count;
            MPI_Datatype datatype;
            int root;
        } bcast, ibcast;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } exscan, iexscan;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } gather, igather, scatter, iscatter;
        struct {
            const void *sendbuf;
            MPI_Aint sendcount;
            MPI_Datatype sendtype;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            const MPI_Aint *displs;
            MPI_Datatype recvtype;
            int root;
        } gatherv, igatherv;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
            int root;
        } reduce, ireduce;
        struct {
            const void *sendbuf;
            void *recvbuf;
            const MPI_Aint *recvcounts;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter, ireduce_scatter;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint recvcount;
            MPI_Datatype datatype;
            MPI_Op op;
        } reduce_scatter_block, ireduce_scatter_block;
        struct {
            const void *sendbuf;
            void *recvbuf;
            MPI_Aint count;
            MPI_Datatype datatype;
            MPI_Op op;
        } scan, iscan;
        struct {
            const void *sendbuf;
            const MPI_Aint *sendcounts;
            const MPI_Aint *displs;
            MPI_Datatype sendtype;
            MPI_Aint recvcount;
            void *recvbuf;
            MPI_Datatype recvtype;
            int root;
        } scatterv, iscatterv;
    } u;
};

struct MPII_Csel_container {
    MPII_Csel_container_type_e id;

    union {
        struct {
            struct {
                int k;
            } intra_tsp_brucks;
            struct {
                int k;
            } intra_tsp_recexch_doubling;
            struct {
                int k;
            } intra_tsp_recexch_halving;
        } iallgather;
        struct {
            struct {
                int k;
            } intra_tsp_brucks;
            struct {
                int k;
            } intra_tsp_recexch_doubling;
            struct {
                int k;
            } intra_tsp_recexch_halving;
        } iallgatherv;
        struct {
            struct {
                int k;
            } intra_tsp_recexch_single_buffer;
            struct {
                int k;
            } intra_tsp_recexch;
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
            } intra_tsp_tree;
            struct {
                int k;
            } intra_tsp_recexch_reduce_scatter_recexch_allgatherv;
        } iallreduce;
        struct {
            struct {
                int k;
                int buffer_per_phase;
            } intra_tsp_brucks;
            struct {
                int batch_size;
                int bblock;
            } intra_tsp_scattered;
        } ialltoall;
        struct {
            struct {
                int batch_size;
                int bblock;
            } intra_tsp_scattered;
            struct {
                int bblock;
            } intra_tsp_blocked;
        } ialltoallv;
        struct {
            struct {
                int bblock;
            } intra_tsp_blocked;
        } ialltoallw;
        struct {
            struct {
                int k;
            } intra_k_dissemination;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch;
        } barrier;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
            struct {
                int k;
            } intra_tsp_k_dissemination;
        } ibarrier;
        struct {
            struct {
                int tree_type;
                int k;
                int chunk_size;
            } intra_tsp_tree;
            struct {
                int chunk_size;
            } intra_tsp_ring;
            struct {
                int scatterv_k;
                int allgatherv_k;
            } intra_tsp_scatterv_allgatherv;
            struct {
                int scatterv_k;
            } intra_tsp_scatterv_ring_allgatherv;
        } ibcast;
        struct {
            struct {
                int tree_type;
                int k;
                int is_non_blocking;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tree;
            struct {
                int tree_type;
                int k;
                int is_non_blocking;
                int chunk_size;
                int recv_pre_posted;
            } intra_pipelined_tree;
        } bcast;
        struct {
            struct {
                int k;
            } intra_k_brucks;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch_doubling;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch_halving;
        } allgather;
        struct {
            struct {
                int k;
            } intra_k_brucks;
        } alltoall;
        struct {
            struct {
                int k;
            } intra_tsp_tree;
        } igather;
        struct {
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tsp_tree;
            struct {
                int chunk_size;
                int buffer_per_child;
            } intra_tsp_ring;
        } ireduce;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
        } ireduce_scatter;
        struct {
            struct {
                int k;
            } intra_tsp_recexch;
        } ireduce_scatter_block;
        struct {
            struct {
                int k;
            } intra_recursive_multiplying;
            struct {
                int tree_type;
                int k;
                int chunk_size;
                int buffer_per_child;
                int topo_overhead;
                int topo_diff_groups;
                int topo_diff_switches;
                int topo_same_switches;
            } intra_tree;
            struct {
                int k;
                bool single_phase_recv;
            } intra_recexch;
            struct {
                int k;
                bool single_phase_recv;
            } intra_k_reduce_scatter_allgather;
            struct {
                int ccl;
            } intra_ccl;
        } allreduce;
        struct {
            struct {
                int k;
            } intra_tsp_tree;
        } iscatter;
    } u;
};

typedef int (*MPIR_Coll_algo_fn) (MPIR_Csel_coll_sig_s * coll_sig, MPII_Csel_container_s * cnt);
void MPIR_Init_coll_sig(MPIR_Csel_coll_sig_s * coll_sig);

/* During init, not all algorithms are safe to use. For example, the csel
 * may not have been initialized. We define a set of fallback routines that
 * are safe to use during init. They are all intra algorithms.
 */
#define MPIR_Barrier_fallback    MPIR_Barrier_intra_dissemination
#define MPIR_Allgather_fallback  MPIR_Allgather_intra_brucks
#define MPIR_Allgatherv_fallback MPIR_Allgatherv_intra_brucks
#define MPIR_Allreduce_fallback  MPIR_Allreduce_intra_recursive_doubling
#define MPIR_Bcast_fallback      MPIR_Bcast_intra_binomial
#define MPIR_Gather_fallback     MPIR_Gather_intra_binomial


/* Internal point-to-point communication for collectives */
/* These functions are used in the implementation of collective and
   other internal operations. They are wrappers around MPID send/recv
   functions. They do sends/receives by setting the context offset to
   MPIR_CONTEXT_INTRA(INTER)_COLL. */
int MPIC_Wait(MPIR_Request * request_ptr);
int MPIC_Probe(int source, int tag, MPI_Comm comm, MPI_Status * status);

int MPIC_Send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
              MPIR_Comm * comm_ptr, int coll_attr);
int MPIC_Recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source, int tag,
              MPIR_Comm * comm_ptr, MPI_Status * status);
int MPIC_Sendrecv(const void *sendbuf, MPI_Aint sendcount, MPI_Datatype sendtype,
                  int dest, int sendtag, void *recvbuf, MPI_Aint recvcount,
                  MPI_Datatype recvtype, int source, int recvtag,
                  MPIR_Comm * comm_ptr, MPI_Status * status, int coll_attr);
int MPIC_Sendrecv_replace(void *buf, MPI_Aint count, MPI_Datatype datatype,
                          int dest, int sendtag,
                          int source, int recvtag,
                          MPIR_Comm * comm_ptr, MPI_Status * status, int coll_attr);
int MPIC_Isend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest, int tag,
               MPIR_Comm * comm_ptr, MPIR_Request ** request, int coll_attr);
int MPIC_Irecv(void *buf, MPI_Aint count, MPI_Datatype datatype, int source,
               int tag, MPIR_Comm * comm_ptr, MPIR_Request ** request);
int MPIC_Waitall(int numreq, MPIR_Request * requests[], MPI_Status * statuses);

int MPIR_Reduce_local(const void *inbuf, void *inoutbuf, MPI_Aint count, MPI_Datatype datatype,
                      MPI_Op op);

int MPIR_Barrier_intra_dissemination(MPIR_Comm * comm_ptr, int coll_attr);
int MPIR_Allgather_intra_smp_no_order(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype,
                                      void *recvbuf, MPI_Aint recvcount, MPI_Datatype recvtype,
                                      MPIR_Comm * comm_ptr, int coll_attr);

/* TSP auto */
int MPIR_TSP_Iallreduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                             MPI_Datatype datatype, MPI_Op op,
                                             MPIR_Comm * comm, MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibcast_sched_intra_tsp_auto(void *buffer, MPI_Aint count, MPI_Datatype datatype,
                                         int root, MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched);
int MPIR_TSP_Ibarrier_sched_intra_tsp_auto(MPIR_Comm * comm, MPIR_TSP_sched_t sched);
int MPIR_TSP_Ireduce_sched_intra_tsp_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                          MPI_Datatype datatype, MPI_Op op, int root,
                                          MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched);
int MPIR_Ireduce_sched_intra_tsp_flat_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                           MPI_Datatype datatype, MPI_Op op, int root,
                                           MPIR_Comm * comm_ptr, MPIR_TSP_sched_t sched);
#endif /* MPIR_COLL_H_INCLUDED */
