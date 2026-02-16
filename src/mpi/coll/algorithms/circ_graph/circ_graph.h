/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CIRC_GRAPH_H_INCLUDED
#define CIRC_GRAPH_H_INCLUDED

#include "mpidu_genq_private_pool.h"

/* Because reduce, allgather, allreduce are based on the bcast algorithm, we
 * assume the bcast context unless otherwise noted */

/* note: the cga prefix refers "circulant graph algorithm" */

typedef struct {
    int p;                      /* number of processes, comm_size */
    int r;                      /* relative rank, r = (rank - root) MOD p */
    int q;                      /* ceil(log2(p)), total (n -1 + q) rounds, where n is the number of blocks */

    int *Skip;                  /* Skip[q+1] - each round process r send to r+Skip[k] and receive from r-Skip[k],
                                 *             with a "MOD p" assumption. */
    /* The receive schedule R[] and send scheduld S[].
     * For bcast and reduce, we only need the schedule for this rank, thus the dimension is R[q] and S[q].
     * For allgather and allreduce, we need schedules for every rank, thus the dimension is R[p][q] and S[p][q].
     *     In the latter case, R[r][q] and S[r][q] are the schedule for root = (rank - r) MOD p.
     */
    int *R;                     /* R[q] - in round k, this process receives the block R[k] (from r-Skip[k]) */
    int *S;                     /* S[q] - in round k, this process sends the block S[k] (to r+Skip[k]) */

    void *mem;                  /* allocated memory */
} MPII_circ_graph;

int MPII_circ_graph_create(MPII_circ_graph * cga, int p, int r);
int MPII_circ_graph_create_all(MPII_circ_graph * cga, int p);
int MPII_circ_graph_free(MPII_circ_graph * cga);

/* runtime request queue */

/* We assume there are n chunks in a contiguous buf with equal chunk_size except the last chunk.
 * This may get relaxed (extended) later */

enum MPII_cga_type {
    MPII_CGA_BCAST,
    MPII_CGA_REDUCE,
};

enum MPII_cga_op_type {
    MPII_CGA_OP_SEND,
    MPII_CGA_OP_RECV,
};

enum MPII_cga_op_stage {
    MPII_CGA_STAGE_NULL,
    MPII_CGA_STAGE_START,       /* request created but no op started yet */
    MPII_CGA_STAGE_COPY,        /* waiting for the async local copy */
    MPII_CGA_STAGE_REQUEST,     /* waiting for the send/recv request */
    MPII_CGA_STAGE_REDUCE,      /* at reduce_local pending dependency */
};

typedef struct {
    int q_len;
    int num_pending;
    MPI_Aint num_chunks;        /* total number of blocks */
    MPI_Aint chunk_size;
    MPI_Aint last_chunk_size;

    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    MPI_Aint buf_extent;        /* count * extent, needed by allgather */

    bool need_pack;
    MPL_pointer_attr_t attr;

    enum MPII_cga_type coll_type;
    union {
        struct {
            MPI_Aint type_size; /* for calc chunk_size -> chunk_count */
            MPI_Aint type_extent;
            MPI_Aint chunk_extent;      /* for calc buf offset at a block */
            MPI_Aint true_lb;   /* adjustment for tmp_buf */
            MPI_Aint tmpbuf_size;       /* (chunk_count - 1) * extent + true_extent, but 0 if not needed */
            MPI_Op op;
        } reduce;
    } u;

    MPIR_Comm *comm;
    int tag;
    int coll_attr;
    int all_size;               /* for bcast and reduce, all_size is 1.
                                 * for allgather and reduce_scatter, all_size is comm_size */

    struct {
        int req_id;             /* points to the index of the pending requests */
        void *pack_buf;         /* if need_pack, allocated chunk buffer */
    } *pending_blocks;
    int pending_head;
    int pending_head_block;
    bool inverse_order;

    struct {
        enum MPII_cga_op_type op_type;
        enum MPII_cga_op_stage op_stage;
        /* sends and recvs need be issued in order. It is difficult to figure out whether
         * the request has been issued yet from op_stage. Use an explicit flag as shortcut. */
        bool issued;
        union {
            MPIR_gpu_req async_req;
            MPIR_Request *req;
        } u;
        void *tmpbuf;           /* reduce need recv into a tmpbuf before reduce_local */
        int block;
        int root;

        int peer_rank;

        /* for reduction, we may have multiple requests concurrent on the same block,
         * thus we may need a linked list */
        int next_req_id;
    } *requests;                /* requests[q_len] */
    int q_head;
} MPII_cga_request_queue;       /* note: cga refers to "circulant graph algorithm" */

extern MPIDU_genq_private_pool_t MPIR_cga_chunk_pool;

int MPIR_cga_init(void);
int MPIR_cga_finalize(void);

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue, int num_pending,
                              void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPIR_Comm * comm, int coll_attr);
int MPII_cga_init_allgather_queue(MPII_cga_request_queue * queue, int num_pending,
                                  void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  MPIR_Comm * comm, int coll_attr, int rank, int comm_size);
int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue, int num_pending,
                               void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm, int coll_attr);

int MPII_cga_bcast_isend(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag);
int MPII_cga_bcast_irecv(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag);
int MPII_cga_bcast_send(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_bcast_recv(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_allgather_isend(MPII_cga_request_queue * queue, int block, int root, int peer_rank,
                             bool * flag);
int MPII_cga_allgather_irecv(MPII_cga_request_queue * queue, int block, int root, int peer_rank,
                             bool * flag);
int MPII_cga_allgather_send(MPII_cga_request_queue * queue, int block, int root, int peer_rank);
int MPII_cga_allgather_recv(MPII_cga_request_queue * queue, int block, int root, int peer_rank);
int MPII_cga_reduce_isend(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag);
int MPII_cga_reduce_irecv(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag);
int MPII_cga_reduce_send(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_reduce_recv(MPII_cga_request_queue * queue, int block, int peer_rank);

int MPII_cga_waitall(MPII_cga_request_queue * queue);
int MPII_cga_testall(MPII_cga_request_queue * queue, bool * is_done);

#endif
