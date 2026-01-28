/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

static int wait_if_full(MPII_cga_request_queue * queue);
static int wait_for_request(MPII_cga_request_queue * queue, int i);

/* Routines for managing non-blocking send/recv of chunks  */
static int init_request_queue_common(MPII_cga_request_queue * queue,
                                     int q_len, int n, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    queue->q_len = q_len;
    queue->n = n;
    queue->comm = comm;
    queue->coll_attr = coll_attr;

    queue->q_head = 0;
    queue->q_tail = 0;

    queue->pending_blocks = MPL_malloc(n * sizeof(*queue->pending_blocks), MPL_MEM_OTHER);
    if (!queue->pending_blocks) {
        MPL_free(queue);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    queue->requests = MPL_malloc(q_len * sizeof(*queue->requests), MPL_MEM_OTHER);
    if (!queue->requests) {
        MPL_free(queue);
        MPL_free(queue->pending_blocks);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int i = 0; i < n; i++) {
        /* -1 marks the block not pending (available to send) */
        queue->pending_blocks[i] = -1;
    }

    for (int i = 0; i < q_len; i++) {
        queue->requests[i].req = NULL;
        queue->requests[i].chunk_id = -1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue,
                              int q_len, void *buf, int n, int chunk_size, int last_chunk_size,
                              MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno;
    mpi_errno = init_request_queue_common(queue, q_len, n, comm, coll_attr);

    queue->coll_type = MPII_CGA_BCAST;
    queue->u.bcast.buf = buf;

    /* only with contig chunks for now */
    queue->chunk_count = chunk_size;
    queue->last_chunk_count = last_chunk_size;
    queue->chunk_extent = chunk_size;
    queue->datatype = MPIR_BYTE_INTERNAL;
    queue->tag = MPIR_BCAST_TAG;

    return mpi_errno;
}

int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue, int q_len,
                               void *tmp_buf, void *recvbuf, int n,
                               int chunk_count, int last_chunk_count,
                               MPI_Datatype datatype, MPI_Aint extent, MPI_Op op,
                               MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno;
    mpi_errno = init_request_queue_common(queue, q_len, n, comm, coll_attr);

    queue->coll_type = MPII_CGA_REDUCE;
    queue->u.reduce.tmp_buf = tmp_buf;
    queue->u.reduce.recvbuf = recvbuf;

    queue->chunk_count = chunk_count;
    queue->last_chunk_count = last_chunk_count;
    queue->chunk_extent = chunk_count * extent;
    queue->datatype = datatype;
    queue->tag = MPIR_REDUCE_TAG;

    queue->u.reduce.op = op;

    return mpi_errno;
}

#define GET_BLOCK_BUF(base, block) ((char *) (base) + (block) * queue->chunk_extent)
#define GET_BLOCK_COUNT(block)     (((block) == queue->n - 1) ? queue->last_chunk_count : queue->chunk_count)

#define GET_REDUCE_BUF_COUNT(base, block) \
    void *buf = (char *) (base) + (block) * queue->chunk_extent; \
    MPI_Aint count = ((block) == queue->n - 1) ? queue->last_chunk_count : queue->chunk_count

#define GET_REDUCE_LOCAL_BUFS_COUNT(block) \
    void *buf_in = (char *) (queue->u.reduce.tmp_buf) + (block) * queue->chunk_extent; \
    void *buf_inout = (char *) (queue->u.reduce.recvbuf) + (block) * queue->chunk_extent; \
    MPI_Aint count = ((block) == queue->n - 1) ? queue->last_chunk_count : queue->chunk_count

int MPII_cga_issue_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->pending_blocks[block] >= 0) {
        int i = queue->pending_blocks[block];
        mpi_errno = wait_for_request(queue, i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *buf;
    if (queue->coll_type == MPII_CGA_BCAST) {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);

    } else if (queue->coll_type == MPII_CGA_REDUCE) {
        buf = GET_BLOCK_BUF(queue->u.reduce.recvbuf, block);
    } else {
        MPIR_Assert(0);
        buf = NULL;
    }

    MPI_Aint count = GET_BLOCK_COUNT(block);
    mpi_errno = MPIC_Isend(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req), queue->coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].chunk_id = -1;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_issue_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    void *buf;
    if (queue->coll_type == MPII_CGA_BCAST) {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);

    } else if (queue->coll_type == MPII_CGA_REDUCE) {
        /* reduction receives the same block from different processes */
        if (queue->pending_blocks[block] >= 0) {
            int i = queue->pending_blocks[block];
            mpi_errno = wait_for_request(queue, i);
            MPIR_ERR_CHECK(mpi_errno);
        }

        buf = GET_BLOCK_BUF(queue->u.reduce.tmp_buf, block);
    } else {
        MPIR_Assert(0);
        buf = NULL;
    }
    MPI_Aint count = GET_BLOCK_COUNT(block);
    mpi_errno = MPIC_Irecv(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req));
    MPIR_ERR_CHECK(mpi_errno);

    queue->pending_blocks[block] = queue->q_head;
    queue->requests[queue->q_head].chunk_id = block;

    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_waitall(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].req) {
            mpi_errno = wait_for_request(queue, i);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPL_free(queue->pending_blocks);
    MPL_free(queue->requests);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static int wait_if_full(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->q_head == queue->q_tail) {
        if (queue->requests[queue->q_tail].req) {
            mpi_errno = wait_for_request(queue, queue->q_tail);
            MPIR_ERR_CHECK(mpi_errno);
        }

        queue->q_tail = (queue->q_tail + 1) % queue->q_len;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int reduce_local(MPII_cga_request_queue * queue, int block);

static int wait_for_request(MPII_cga_request_queue * queue, int i)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIC_Wait(queue->requests[i].req);
    MPIR_ERR_CHECK(mpi_errno);

    if (queue->requests[i].chunk_id != -1) {
        /* it's a recv, update pending_blocks */
        int block = queue->requests[i].chunk_id;
        queue->pending_blocks[block] = -1;
        if (queue->coll_type == MPII_CGA_REDUCE) {
            mpi_errno = reduce_local(queue, block);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    MPIR_Request_free(queue->requests[i].req);
    queue->requests[i].req = NULL;
    queue->requests[i].chunk_id = -1;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int reduce_local(MPII_cga_request_queue * queue, int block)
{
    int mpi_errno;

    void *buf_in = GET_BLOCK_BUF(queue->u.reduce.tmp_buf, block);
    void *buf_inout = GET_BLOCK_BUF(queue->u.reduce.recvbuf, block);
    MPI_Aint count = GET_BLOCK_COUNT(block);
    mpi_errno = MPIR_Reduce_local(buf_in, buf_inout, count, queue->datatype, queue->u.reduce.op);

    return mpi_errno;
}
