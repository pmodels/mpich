/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CNC_COLLECTIVE_SHIM_H_INCLUDED
#define CNC_COLLECTIVE_SHIM_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#ifndef CNC_TRANSPORT_H
typedef struct cnc_transport cnc_transport_t;

typedef enum cnc_status {
    CNC_OK = 0,
    CNC_ERR = 1,
    CNC_ERR_INVAL = 2,
    CNC_ERR_NOTSUP = 3,
    CNC_INPROGRESS = 4
} cnc_status_t;

typedef struct cnc_oob {
    cnc_status_t(*allgather) (const void *src, void *recv, size_t size,
                              void *context, void **request);
    cnc_status_t(*req_test) (void *request);
    cnc_status_t(*req_free) (void *request);
    void *context;
    uint32_t n_eps;
    uint32_t ep;
} cnc_oob_t;
#endif /* CNC_TRANSPORT_H */

#ifndef CNC_COLLECTIVE_H
#define CNC_IN_PLACE ((void *) -1)

typedef enum cnc_bcast_algo {
    CNC_BCAST_BINOMIAL = 0,
    CNC_BCAST_SCATTER_ALLGATHER = 1,
    CNC_BCAST_CIRCULANT = 2,
    CNC_BCAST_BINE = 3
} cnc_bcast_algo_t;

typedef enum cnc_gather_algo {
    CNC_GATHER_BINOMIAL = 0,
    CNC_GATHER_BINE = 1
} cnc_gather_algo_t;

typedef enum cnc_allgather_algo {
    CNC_ALLGATHER_RING = 0,
    CNC_ALLGATHER_RECURSIVE_DOUBLING = 1,
    CNC_ALLGATHER_BRUCK = 2,
    CNC_ALLGATHER_BINE = 3
} cnc_allgather_algo_t;

typedef enum cnc_allreduce_algo {
    CNC_ALLREDUCE_RECURSIVE_DOUBLING = 0,
    CNC_ALLREDUCE_RABENSEIFNER = 1,
    CNC_ALLREDUCE_RING = 2,
    CNC_ALLREDUCE_BINE = 3,
    CNC_ALLREDUCE_BINE_BANDWIDTH = 4
} cnc_allreduce_algo_t;

typedef enum cnc_alltoall_algo {
    CNC_ALLTOALL_PAIRWISE = 0,
    CNC_ALLTOALL_BRUCK = 1
} cnc_alltoall_algo_t;

typedef enum cnc_reduce_scatter_algo {
    CNC_REDUCE_SCATTER_RECURSIVE_HALVING = 0,
    CNC_REDUCE_SCATTER_BINE = 1
} cnc_reduce_scatter_algo_t;

typedef enum cnc_scatter_algo {
    CNC_SCATTER_BINOMIAL = 0,
    CNC_SCATTER_BINE = 1
} cnc_scatter_algo_t;

typedef enum cnc_reduce_algo {
    CNC_REDUCE_BINOMIAL = 0,
    CNC_REDUCE_RABENSEIFNER = 1,
    CNC_REDUCE_CIRCULANT = 2,
    CNC_REDUCE_BINE = 3
} cnc_reduce_algo_t;

typedef enum cnc_reduce_op {
    CNC_REDUCE_SUM = 0,
    CNC_REDUCE_PROD = 1,
    CNC_REDUCE_MIN = 2,
    CNC_REDUCE_MAX = 3
} cnc_reduce_op_t;

typedef enum cnc_datatype {
    CNC_DTYPE_F32 = 0,
    CNC_DTYPE_I32 = 1
} cnc_datatype_t;

typedef struct cnc_bcast_opts {
    cnc_bcast_algo_t algo;
    int chunk;
    int topology_remap;
} cnc_bcast_opts_t;

typedef struct cnc_gather_opts {
    int chunk;
    cnc_gather_algo_t algo;
} cnc_gather_opts_t;

typedef struct cnc_scatter_opts {
    int chunk;
    cnc_scatter_algo_t algo;
} cnc_scatter_opts_t;

typedef struct cnc_allgather_opts {
    int chunk;
    cnc_allgather_algo_t algo;
} cnc_allgather_opts_t;

typedef struct cnc_alltoall_opts {
    int chunk;
    cnc_alltoall_algo_t algo;
} cnc_alltoall_opts_t;

typedef struct cnc_reduce_opts {
    cnc_reduce_op_t op;
    int chunk;
    cnc_reduce_algo_t algo;
    int topology_remap;
} cnc_reduce_opts_t;

typedef struct cnc_allreduce_opts {
    cnc_reduce_op_t op;
    int chunk;
    cnc_allreduce_algo_t algo;
} cnc_allreduce_opts_t;

typedef struct cnc_reduce_scatter_opts {
    cnc_reduce_op_t op;
    int chunk;
    cnc_reduce_scatter_algo_t algo;
} cnc_reduce_scatter_opts_t;
#endif /* CNC_COLLECTIVE_H */

cnc_status_t cnc_init(cnc_transport_t ** out, const cnc_oob_t * oob);
void cnc_fini(cnc_transport_t * t);

void cnc_barrier(int n, int rank, cnc_transport_t * t);

cnc_status_t cnc_bcast(const void *sendbuf, void *recvbuf, size_t nbytes,
                       int n, int rank, int root, cnc_transport_t * t,
                       const cnc_bcast_opts_t * opts);

cnc_status_t cnc_gather(const void *sendbuf, void *recvbuf, size_t nbytes,
                        int n, int rank, int root, cnc_transport_t * t,
                        const cnc_gather_opts_t * opts);

cnc_status_t cnc_gatherv_linear(const void *sendbuf, size_t sendcount, void *recvbuf,
                                const size_t * recvcounts, const ptrdiff_t * displs,
                                int n, int rank, int root, cnc_transport_t * t);

cnc_status_t cnc_scatter(const void *sendbuf, void *recvbuf, size_t nbytes,
                         int n, int rank, int root, cnc_transport_t * t,
                         const cnc_scatter_opts_t * opts);

cnc_status_t cnc_scatterv_linear(const void *sendbuf, const size_t * sendcounts,
                                 const ptrdiff_t * displs, void *recvbuf, size_t recvcount,
                                 int n, int rank, int root, cnc_transport_t * t);

cnc_status_t cnc_allgather(const void *sendbuf, void *recvbuf, size_t nbytes,
                           int n, int rank, cnc_transport_t * t, const cnc_allgather_opts_t * opts);

cnc_status_t cnc_allgatherv_ring(const void *sendbuf, void *recvbuf,
                                 const size_t * recvcounts, const ptrdiff_t * displs,
                                 int n, int rank, cnc_transport_t * t);

cnc_status_t cnc_alltoall(const void *sendbuf, void *recvbuf, size_t nbytes,
                          int n, int rank, cnc_transport_t * t, const cnc_alltoall_opts_t * opts);

cnc_status_t cnc_alltoallv_pairwise(const void *sendbuf, const size_t * sendcounts,
                                    const ptrdiff_t * sdispls, void *recvbuf,
                                    const size_t * recvcounts, const ptrdiff_t * rdispls,
                                    int n, int rank, cnc_transport_t * t);

cnc_status_t cnc_reduce(const void *sendbuf, void *recvbuf, size_t count,
                        cnc_datatype_t datatype, int n, int rank, int root,
                        cnc_transport_t * t, const cnc_reduce_opts_t * opts);

cnc_status_t cnc_allreduce(const void *sendbuf, void *recvbuf, size_t count,
                           cnc_datatype_t datatype, int n, int rank,
                           cnc_transport_t * t, const cnc_allreduce_opts_t * opts);

cnc_status_t cnc_reduce_scatter(const void *sendbuf, void *recvbuf, size_t count,
                                cnc_datatype_t datatype, int n, int rank,
                                cnc_transport_t * t, const cnc_reduce_scatter_opts_t * opts);

#endif /* CNC_COLLECTIVE_SHIM_H_INCLUDED */
