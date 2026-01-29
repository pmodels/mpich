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
    int *R;                     /* R[q] - in round k, this process receives the block R[k] (from r-Skip[k]) */
    int *S;                     /* S[q] - in round k, this process sends the block S[k] (to r+Skip[k]) */

    void *mem;                  /* allocated memory */
} MPII_circ_graph;

int MPII_circ_graph_create(MPII_circ_graph * cga, int p, int r);
int MPII_circ_graph_free(MPII_circ_graph * cga);

/* runtime request queue */

/* We assume there are n chunks in a contiguous buf with equal chunk_size except the last chunk.
 * This may get relaxed (extended) later */

enum MPII_cga_type {
    MPII_CGA_BCAST,
    /* more types to be defined later */
};

typedef struct {
    int q_len;
    int n;                      /* total number of blocks */
    int chunk_count;
    int last_chunk_count;
    MPI_Aint chunk_extent;      /* chunk_count * extent */
    MPI_Datatype datatype;

    enum MPII_cga_type coll_type;
    union {
        struct {
            void *buf;
        } bcast;
    } u;

    MPIR_Comm *comm;
    int tag;
    int coll_attr;

    int *pending_blocks;        /* pending_blocks[n], points to the index of the pending requests
                                 * if the block is in transit */
    struct {
        MPIR_Request *req;
        int chunk_id;
    } *requests;                /* requests[q_len] */
    int q_head;
    int q_tail;
} MPII_cga_request_queue;       /* note: cga refers to "circulant graph algorithm" */

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue,
                              int q_len, void *buf, int n, int chunk_size, int last_chunk_size,
                              MPIR_Comm * comm, int coll_attr);
int MPII_cga_issue_send(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_issue_recv(MPII_cga_request_queue * queue, int block, int peer_rank);
int MPII_cga_waitall(MPII_cga_request_queue * queue);

#endif
