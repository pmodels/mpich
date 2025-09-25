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

/* Define values for collective attribute.
 * - The first 8 bits are passed down to basic collective algorithms.
 * - The higher bits are used to assist algorithm selections
 *   - The lower 32 bits are reserved by MPIR-layer
 *   - The higher 32 bits are reserved for MPID-layer
 */
#define MPIR_COLL_ATTR_CORE_BITS  8
#define MPIR_COLL_ATTR_MPIR_BITS 32

/* bit 0-7 */
#define MPIR_COLL_ATTR_SYNC  0x1        /* It's an internal collective that focuses
                                         * on synchronization rather than batch latency.
                                         * In particular, advise netmod to avoid using
                                         * injection send. */
#define MPIR_ERR_PROC_FAILED 0x2
#define MPIR_ERR_OTHER       0x4
#define MPIR_COLL_ATTR_ERR_MASK 0x6

#define MPIR_COLL_ATTR_HAS_ERR(coll_attr) ((coll_attr) & MPIR_COLL_ATTR_ERR_MASK)

struct MPIR_Csel_coll_sig {
    MPIR_Csel_coll_type_e coll_type;
    MPIR_Comm *comm_ptr;
    void *sched;
    enum MPIR_sched_type sched_type;
    bool is_persistent;

    struct {
        bool is_gpu;
    } cache;

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
