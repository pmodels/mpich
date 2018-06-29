#ifndef SHM_COLL_PARAMS_H_INCLUDED
#define SHM_COLL_PARAMS_H_INCLUDED

#include "../posix/posix_coll_params.h"

typedef union {
    MPIDI_POSIX_BARRIER_PARAMS_DECL;
} MPIDI_SHM_Barrier_params_t;

typedef union {
    MPIDI_POSIX_BCAST_PARAMS_DECL;
} MPIDI_SHM_Bcast_params_t;

typedef union {
    MPIDI_POSIX_REDUCE_PARAMS_DECL;
} MPIDI_SHM_Reduce_params_t;

typedef union {
    MPIDI_POSIX_ALLREDUCE_PARAMS_DECL;
} MPIDI_SHM_Allreduce_params_t;

typedef union {
    MPIDI_POSIX_ALLTOALL_PARAMS_DECL;
} MPIDI_SHM_Alltoall_params_t;

typedef union {
    MPIDI_POSIX_ALLTOALLV_PARAMS_DECL;
} MPIDI_SHM_Alltoallv_params_t;

typedef union {
    MPIDI_POSIX_ALLTOALLW_PARAMS_DECL;
} MPIDI_SHM_Alltoallw_params_t;

typedef union {
    MPIDI_POSIX_ALLGATHER_PARAMS_DECL;
} MPIDI_SHM_Allgather_params_t;

typedef union {
    MPIDI_POSIX_ALLGATHERV_PARAMS_DECL;
} MPIDI_SHM_Allgatherv_params_t;

typedef union {
    MPIDI_POSIX_GATHER_PARAMS_DECL;
} MPIDI_SHM_Gather_params_t;

typedef union {
    MPIDI_POSIX_GATHERV_PARAMS_DECL;
} MPIDI_SHM_Gatherv_params_t;

typedef union {
    MPIDI_POSIX_SCATTER_PARAMS_DECL;
} MPIDI_SHM_Scatter_params_t;

typedef union {
    MPIDI_POSIX_SCATTERV_PARAMS_DECL;
} MPIDI_SHM_Scatterv_params_t;

typedef union {
    MPIDI_POSIX_REDUCE_SCATTER_PARAMS_DECL;
} MPIDI_SHM_Reduce_scatter_params_t;

typedef union {
    MPIDI_POSIX_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
} MPIDI_SHM_Reduce_scatter_block_params_t;

typedef union {
    MPIDI_POSIX_SCAN_PARAMS_DECL;
} MPIDI_SHM_Scan_params_t;

typedef union {
    MPIDI_POSIX_EXSCAN_PARAMS_DECL;
} MPIDI_SHM_Exscan_params_t;

#define MPIDI_SHM_BARRIER_PARAMS_DECL MPIDI_SHM_Barrier_params_t shm_barrier_params;
#define MPIDI_SHM_BCAST_PARAMS_DECL MPIDI_SHM_Bcast_params_t shm_bcast_params;
#define MPIDI_SHM_REDUCE_PARAMS_DECL MPIDI_SHM_Reduce_params_t shm_reduce_params;
#define MPIDI_SHM_ALLREDUCE_PARAMS_DECL MPIDI_SHM_Allreduce_params_t shm_allreduce_params;
#define MPIDI_SHM_ALLTOALL_PARAMS_DECL MPIDI_SHM_Alltoall_params_t shm_alltoall_params;
#define MPIDI_SHM_ALLTOALLV_PARAMS_DECL MPIDI_SHM_Alltoallv_params_t shm_alltoallv_params;
#define MPIDI_SHM_ALLTOALLW_PARAMS_DECL MPIDI_SHM_Alltoallw_params_t shm_alltoallw_params;
#define MPIDI_SHM_ALLGATHER_PARAMS_DECL MPIDI_SHM_Allgather_params_t shm_allgather_params;
#define MPIDI_SHM_ALLGATHERV_PARAMS_DECL MPIDI_SHM_Allgatherv_params_t shm_allgatherv_params;
#define MPIDI_SHM_GATHER_PARAMS_DECL MPIDI_SHM_Gather_params_t shm_gather_params;
#define MPIDI_SHM_GATHERV_PARAMS_DECL MPIDI_SHM_Gatherv_params_t shm_gatherv_params;
#define MPIDI_SHM_SCATTER_PARAMS_DECL MPIDI_SHM_Scatter_params_t shm_scatter_params;
#define MPIDI_SHM_SCATTERV_PARAMS_DECL MPIDI_SHM_Scatterv_params_t shm_scatterv_params;
#define MPIDI_SHM_REDUCE_SCATTER_PARAMS_DECL MPIDI_SHM_Reduce_scatter_params_t shm_reduce_scatter_params;
#define MPIDI_SHM_REDUCE_SCATTER_BLOCK_PARAMS_DECL MPIDI_SHM_Reduce_scatter_block_params_t shm_reduce_scatter_block_params;
#define MPIDI_SHM_SCAN_PARAMS_DECL MPIDI_SHM_Scan_params_t shm_scan_params;
#define MPIDI_SHM_EXSCAN_PARAMS_DECL MPIDI_SHM_Exscan_params_t shm_exscan_params;

typedef union {
    MPIDI_SHM_BARRIER_PARAMS_DECL;
    MPIDI_SHM_BCAST_PARAMS_DECL;
    MPIDI_SHM_REDUCE_PARAMS_DECL;
    MPIDI_SHM_ALLREDUCE_PARAMS_DECL;
    MPIDI_SHM_ALLTOALL_PARAMS_DECL;
    MPIDI_SHM_ALLTOALLV_PARAMS_DECL;
    MPIDI_SHM_ALLTOALLW_PARAMS_DECL;
    MPIDI_SHM_ALLGATHER_PARAMS_DECL;
    MPIDI_SHM_ALLGATHERV_PARAMS_DECL;
    MPIDI_SHM_GATHER_PARAMS_DECL;
    MPIDI_SHM_GATHERV_PARAMS_DECL;
    MPIDI_SHM_SCATTER_PARAMS_DECL;
    MPIDI_SHM_SCATTERV_PARAMS_DECL;
    MPIDI_SHM_REDUCE_SCATTER_PARAMS_DECL;
    MPIDI_SHM_REDUCE_SCATTER_BLOCK_PARAMS_DECL;
    MPIDI_SHM_SCAN_PARAMS_DECL;
    MPIDI_SHM_EXSCAN_PARAMS_DECL;
} MPIDI_SHM_coll_params_t;

typedef struct MPIDI_SHM_coll_algo_container {
    int id;
    MPIDI_SHM_coll_params_t params;
} MPIDI_SHM_coll_algo_container_t;

#endif /* SHM_COLL_PARAMS_H_INCLUDED */
