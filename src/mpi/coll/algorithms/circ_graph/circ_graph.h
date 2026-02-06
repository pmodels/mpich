/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CIRC_GRAPH_H_INCLUDED
#define CIRC_GRAPH_H_INCLUDED

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
    MPII_CGA_ALLGATHER,
};

enum MPII_cga_op_type {
    MPII_CGA_OP_SEND,
    MPII_CGA_OP_RECV,
};

typedef struct {
    int q_len;
    int num_chunks;             /* total number of blocks */

    /* datatype for each chunk */
    int chunk_count;
    int last_chunk_count;
    MPI_Aint chunk_extent;      /* chunk_count * extent */
    MPI_Datatype datatype;

    enum MPII_cga_type coll_type;
    union {
        struct {
            void *buf;
            bool need_pack;
            /* following fields are needed for pack/unpack */
            bool is_root;       /* root pack on send, non-root unpack on recv */
            bool last_chunk_unpacked;   /* root will send the last chunk in the last q rounds,
                                         * but we only need unpack once */
            void *pack_buf;
            MPI_Aint count;
            MPI_Datatype datatype;
        } bcast;
        struct {
            void *buf;
            int rank;
            int comm_size;
            MPI_Aint buf_size;
            MPI_Aint buf_extent;
            bool need_pack;
            /* following fields are needed for pack/unpack */
            bool last_chunk_unpacked;   /* The last chunk will be sent multiple times in the last q rounds,
                                         * but we only need unpack once */
            void *pack_buf;
            MPI_Aint count;
            MPI_Datatype datatype;
        } allgather;
        struct {
            void *tmp_buf;
            void *recvbuf;
            MPI_Op op;
        } reduce;
        struct {
            void *tmp_buf;
            void *recvbuf;
            MPI_Op op;
        } allreduce;
    } u;

    MPIR_Comm *comm;
    int tag;
    int coll_attr;

    int *pending_blocks;        /* pending_blocks[n], points to the index of the pending requests
                                 * if the block is in transit */
    struct {
        enum MPII_cga_op_type op_type;
        MPIR_Request *req;
        int chunk_id;
        int round;
        /* for reduction, we may have multiple requests concurrent on the same block,
         * thus we may need a linked list */
        int next_req_id;
    } *requests;                /* requests[q_len] */
    int q_head;
} MPII_cga_request_queue;       /* note: cga refers to "circulant graph algorithm" */

extern MPIDU_genq_private_pool_t MPIR_cga_chunk_pool;

int MPIR_cga_init(void);
int MPIR_cga_finalize(void);

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue,
                              void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPIR_Comm * comm, int coll_attr, bool is_root);
int MPII_cga_init_allgather_queue(MPII_cga_request_queue * queue,
                                  void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  MPIR_Comm * comm, int coll_attr, int rank, int comm_size);
int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue,
                               void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm, int coll_attr);

int MPII_cga_bcast_send(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_bcast_recv(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_allgather_send(MPII_cga_request_queue * queue, int root, int block, int peer_rank);
int MPII_cga_allgather_recv(MPII_cga_request_queue * queue, int root, int block, int peer_rank);
int MPII_cga_reduce_send(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_reduce_recv(MPII_cga_request_queue * queue, int block, int peer_rank);

int MPII_cga_waitall(MPII_cga_request_queue * queue);

#endif
