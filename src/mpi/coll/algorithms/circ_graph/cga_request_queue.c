/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

static int calc_chunks(MPI_Aint data_size, MPI_Aint chunk_size, int *last_msg_size_out);
static int issue_send(MPII_cga_request_queue * queue, const void *buf, MPI_Aint count,
                      int peer_rank, int chunk_id, int *req_idx_out);
static int issue_recv(MPII_cga_request_queue * queue, void *buf, MPI_Aint count,
                      int peer_rank, int chunk_id, int *req_idx_out);
static int wait_if_full(MPII_cga_request_queue * queue);
static int wait_for_request(MPII_cga_request_queue * queue, int i);
static int reduce_local(MPII_cga_request_queue * queue, int block);

/* Routines for managing non-blocking send/recv of chunks  */
static int init_request_queue_common(MPII_cga_request_queue * queue,
                                     int q_len, int num_chunks, int all_size,
                                     MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    queue->q_len = q_len;
    queue->num_chunks = num_chunks;
    queue->comm = comm;
    queue->coll_attr = coll_attr;

    queue->q_head = 0;
    queue->q_tail = 0;

    queue->pending_blocks = MPL_malloc(all_size * num_chunks * sizeof(*queue->pending_blocks),
                                       MPL_MEM_OTHER);
    if (!queue->pending_blocks) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    queue->requests = MPL_malloc(q_len * sizeof(*queue->requests), MPL_MEM_OTHER);
    if (!queue->requests) {
        MPL_free(queue->pending_blocks);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int i = 0; i < all_size * num_chunks; i++) {
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
                              void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPIR_Comm * comm, int coll_attr, bool is_root)
{
    int mpi_errno;

    int chunk_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    int q_len = MPIR_CVAR_CIRC_GRAPH_Q_LEN;

    /* minimum q_len is 2 to avoid circular dependency issues within a round */
    if (q_len < 2) {
        q_len = 2;
    }

    int is_contig;
    MPIR_Datatype_is_contig(datatype, &is_contig);

    MPI_Aint type_size, data_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    data_size = count * type_size;

    bool need_pack = (!is_contig && data_size > 0);

    int last_chunk_size;
    int num_chunks = calc_chunks(data_size, chunk_size, &last_chunk_size);

    mpi_errno = init_request_queue_common(queue, q_len, num_chunks, 1, comm, coll_attr);

    queue->coll_type = MPII_CGA_BCAST;
    queue->u.bcast.buf = buf;
    queue->u.bcast.need_pack = need_pack;
    if (need_pack) {
        /* TODO: In theory, we only need q blocks of pack buffer. */
        queue->u.bcast.pack_buf = MPL_malloc(data_size, MPL_MEM_OTHER);
        MPIR_Assert(queue->u.bcast.pack_buf);   /* TODO: proper error handling */

        queue->u.bcast.is_root = is_root;
        queue->u.bcast.last_chunk_unpacked = false;
        queue->u.bcast.count = count;
        queue->u.bcast.datatype = datatype;
    }

    /* bcast send and recv in bytes */
    queue->chunk_count = chunk_size;
    queue->last_chunk_count = last_chunk_size;
    queue->chunk_extent = chunk_size;
    queue->datatype = MPIR_BYTE_INTERNAL;
    queue->tag = MPIR_BCAST_TAG;

    return mpi_errno;
}

int MPII_cga_init_allgather_queue(MPII_cga_request_queue * queue,
                                  void *buf, MPI_Aint count, MPI_Datatype datatype,
                                  MPIR_Comm * comm, int coll_attr, int rank, int comm_size)
{
    int mpi_errno;

    int chunk_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    int q_len = MPIR_CVAR_CIRC_GRAPH_Q_LEN;

    /* minimum q_len is 2 to avoid circular dependency issues within a round */
    if (q_len < 2) {
        q_len = 2;
    }

    int is_contig;
    MPIR_Datatype_is_contig(datatype, &is_contig);

    bool need_pack = (!is_contig);

    MPI_Aint type_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);

    int last_chunk_size;
    int num_chunks = calc_chunks(count * type_size, chunk_size, &last_chunk_size);
    int all_size = comm_size;

    mpi_errno = init_request_queue_common(queue, q_len, num_chunks, all_size, comm, coll_attr);

    queue->coll_type = MPII_CGA_ALLGATHER;
    queue->u.allgather.buf = buf;
    queue->u.allgather.rank = rank;
    queue->u.allgather.comm_size = comm_size;
    queue->u.allgather.buf_size = count * type_size;
    queue->u.allgather.need_pack = need_pack;
    if (need_pack) {
        /* TODO: In theory, we only need q blocks of pack buffer. */
        queue->u.allgather.pack_buf = MPL_malloc(count * type_size * comm_size, MPL_MEM_OTHER);
        MPIR_Assert(queue->u.allgather.pack_buf);       /* TODO: proper error handling */

        queue->u.allgather.last_chunk_unpacked = false;
        queue->u.allgather.count = count;
        queue->u.allgather.datatype = datatype;

        MPI_Aint extent;
        MPIR_Datatype_get_extent_macro(datatype, extent);
        queue->u.allgather.buf_extent = count * extent;
    }

    /* allgather send and recv in bytes */
    queue->chunk_count = chunk_size;
    queue->last_chunk_count = last_chunk_size;
    queue->chunk_extent = chunk_size;
    queue->datatype = MPIR_BYTE_INTERNAL;
    queue->tag = MPIR_BCAST_TAG;

    return mpi_errno;
}

int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue,
                               void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    int chunk_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    int q_len = MPIR_CVAR_CIRC_GRAPH_Q_LEN;

    /* minimum q_len is 2 */
    if (q_len < 2) {
        q_len = 2;
    }

    int num_chunks, chunk_count, last_chunk_count;
    if (chunk_size == 0) {
        num_chunks = 1;
        chunk_count = count;
        last_chunk_count = count;
    } else {
        /* reduction chunks have to contain whole datatypes */
        MPI_Aint type_size;
        MPIR_Datatype_get_size_macro(datatype, type_size);

        chunk_count = chunk_size / type_size;
        if (chunk_size > 0 && chunk_size % type_size > 0) {
            chunk_count++;
        }

        num_chunks = count / chunk_count;
        last_chunk_count = count % chunk_count;
        if (last_chunk_count == 0) {
            last_chunk_count = chunk_count;
        } else {
            num_chunks++;
        }
    }

    /* allocate tmp_buf to receive data */
    MPI_Aint true_lb, true_extent, extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPI_Aint buf_size = (count - 1) * extent + true_extent;

    void *tmp_buf;
    tmp_buf = MPL_malloc(buf_size, MPL_MEM_OTHER);
    if (!tmp_buf) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    mpi_errno = init_request_queue_common(queue, q_len, num_chunks, 1, comm, coll_attr);

    queue->coll_type = MPII_CGA_REDUCE;
    queue->u.reduce.tmp_buf = tmp_buf;
    queue->u.reduce.recvbuf = recvbuf;

    queue->chunk_count = chunk_count;
    queue->last_chunk_count = last_chunk_count;
    queue->chunk_extent = chunk_count * extent;
    queue->datatype = datatype;
    queue->tag = MPIR_REDUCE_TAG;

    queue->u.reduce.op = op;

  fn_fail:
    return mpi_errno;
}

#define GET_BLOCK_BUF(base, block) ((char *) (base) + (block) * queue->chunk_extent)
#define GET_BLOCK_COUNT(block)     (((block) == queue->num_chunks - 1) ? queue->last_chunk_count : queue->chunk_count)

/* ---- bcast ---- */

int MPII_cga_bcast_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    /* In bcast, send may depend on a recv in the previous rounds, recv does not depend
     * on sends since we only receive each block exactly once */

    int chunk_id = block;
    if (queue->pending_blocks[chunk_id] >= 0) {
        int i = queue->pending_blocks[chunk_id];
        mpi_errno = wait_for_request(queue, i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.bcast.need_pack && queue->u.bcast.is_root && !queue->u.bcast.last_chunk_unpacked) {
        buf = GET_BLOCK_BUF(queue->u.bcast.pack_buf, block);

        MPI_Aint actual_pack_bytes;
        MPI_Aint offset = block * queue->chunk_count;   /* this is in bytes */
        MPIR_Typerep_pack(queue->u.bcast.buf, queue->u.bcast.count, queue->u.bcast.datatype,
                          offset, buf, count, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == count);

        if (block == queue->num_chunks - 1) {
            queue->u.bcast.last_chunk_unpacked = true;
        }
    } else {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);
    }

    int req_idx /* unused */ ;
    mpi_errno = issue_send(queue, buf, count, peer_rank, block, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_bcast_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    /* In bcast, recv does not check dependency since we only recv each block exactly once
     * before send (if at all).
     */

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.bcast.need_pack) {
        buf = GET_BLOCK_BUF(queue->u.bcast.pack_buf, block);
        /* unpack in wait_for_request() */
    } else {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);
    }

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, block, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    int chunk_id = block;
    queue->requests[req_idx].chunk_id = chunk_id;
    queue->pending_blocks[chunk_id] = req_idx;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int bcast_recv_unpack(MPII_cga_request_queue * queue, int block)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = GET_BLOCK_BUF(queue->u.bcast.pack_buf, block);

    MPI_Aint actual_unpack_bytes;
    MPI_Aint offset = block * queue->chunk_count;       /* this is in bytes */
    MPIR_Typerep_unpack(buf, count,
                        queue->u.bcast.buf, queue->u.bcast.count,
                        queue->u.bcast.datatype, offset, &actual_unpack_bytes,
                        MPIR_TYPEREP_FLAG_NONE);
    MPIR_Assert(actual_unpack_bytes == count);

    return mpi_errno;
}

/* ---- allgather ---- */

int MPII_cga_allgather_send(MPII_cga_request_queue * queue, int root, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    int chunk_id = block + root * queue->num_chunks;
    if (queue->pending_blocks[chunk_id] >= 0) {
        int i = queue->pending_blocks[chunk_id];
        mpi_errno = wait_for_request(queue, i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.allgather.need_pack &&
        root == queue->u.allgather.rank && !queue->u.allgather.last_chunk_unpacked) {
        void *pack_buf = (char *) queue->u.allgather.pack_buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(pack_buf, block);

        MPI_Aint actual_pack_bytes;
        MPI_Aint offset = block * queue->chunk_count;   /* this is in bytes */
        void *src_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_extent;
        MPIR_Typerep_pack(src_buf, queue->u.allgather.count, queue->u.allgather.datatype,
                          offset, buf, count, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
        MPIR_Assert(actual_pack_bytes == count);

        if (block == queue->num_chunks - 1) {
            queue->u.allgather.last_chunk_unpacked = true;
        }
    } else {
        void *src_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(src_buf, block);
    }

    int req_idx /* unused */ ;
    mpi_errno = issue_send(queue, buf, count, peer_rank, chunk_id, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_allgather_recv(MPII_cga_request_queue * queue, int root, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.allgather.need_pack) {
        void *dst_buf = (char *) queue->u.allgather.pack_buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(dst_buf, block);
    } else {
        void *dst_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(dst_buf, block);
    }

    int chunk_id = block + root * queue->num_chunks;

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, chunk_id, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[req_idx].chunk_id = chunk_id;
    queue->pending_blocks[chunk_id] = req_idx;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int allgather_recv_unpack(MPII_cga_request_queue * queue, int block)
{
    int mpi_errno = MPI_SUCCESS;

    int root = chunk_id / queue->num_chunks;
    int block = chunk_id % queue->num_chunks;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *src_buf = (char *) queue->u.allgather.pack_buf + root * queue->u.allgather.buf_size;
    void *buf = GET_BLOCK_BUF(src_buf, block);

    MPI_Aint actual_unpack_bytes;
    MPI_Aint offset = block * queue->chunk_count;       /* this is in bytes */
    void *dst_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_extent;
    MPIR_Typerep_unpack(buf, count, dst_buf, queue->u.allgather.count,
                        queue->u.allgather.datatype, offset, &actual_unpack_bytes,
                        MPIR_TYPEREP_FLAG_NONE);
    MPIR_Assert(actual_unpack_bytes == count);

    return mpi_errno;
}

/* ---- reduce ---- */

int MPII_cga_reduce_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    /* in reduce, send may depend on recv from the previous rounds; recv may depend
     * on both the previous send and the previous recv */

    int chunk_id = block;
    if (queue->pending_blocks[chunk_id] >= 0) {
        int i = queue->pending_blocks[chunk_id];
        mpi_errno = wait_for_request(queue, i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = GET_BLOCK_BUF(queue->u.reduce.recvbuf, block);

    int req_idx;
    mpi_errno = issue_send(queue, buf, count, peer_rank, chunk_id, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    queue->pending_blocks[chunk_id] = req_idx;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_reduce_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    /* reduction receives the same block from multiple processes and may reduce
     * into the pending send buffer, thus it may depend on either previous send
     * or recv.
     *
     * However, strictly, only the reduce_local depend on previous send. We will need
     * separaten pending_blocks tracking to make it precise.
     * */
    if (queue->pending_blocks[block] >= 0) {
        int i = queue->pending_blocks[block];
        mpi_errno = wait_for_request(queue, i);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = GET_BLOCK_BUF(queue->u.reduce.tmp_buf, block);

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, block, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    int chunk_id = block;
    queue->requests[req_idx].chunk_id = chunk_id;
    queue->pending_blocks[chunk_id] = req_idx;

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

/* ---- final completion ---- */

int MPII_cga_waitall(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].req) {
            mpi_errno = wait_for_request(queue, i);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    if (queue->coll_type == MPII_CGA_BCAST) {
        if (queue->u.bcast.need_pack) {
            MPL_free(queue->u.bcast.pack_buf);
        }
    } else if (queue->coll_type == MPII_CGA_ALLGATHER) {
        if (queue->u.allgather.need_pack) {
            MPL_free(queue->u.allgather.pack_buf);
        }
    } else if (queue->coll_type == MPII_CGA_REDUCE) {
        MPL_free(queue->u.reduce.tmp_buf);
    }
    MPL_free(queue->pending_blocks);
    MPL_free(queue->requests);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static int issue_send(MPII_cga_request_queue * queue, const void *buf, MPI_Aint count,
                      int peer_rank, int chunk_id, int *req_idx_out)
{
    int mpi_errno;

    *req_idx_out = queue->q_head;
    mpi_errno = MPIC_Isend(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req), queue->coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].op_type = MPII_CGA_OP_SEND;
    queue->requests[queue->q_head].chunk_id = chunk_id;
    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_recv(MPII_cga_request_queue * queue, void *buf, MPI_Aint count,
                      int peer_rank, int chunk_id, int *req_idx_out)
{
    int mpi_errno;

    *req_idx_out = queue->q_head;
    mpi_errno = MPIC_Irecv(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req));
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].op_type = MPII_CGA_OP_RECV;
    queue->requests[queue->q_head].chunk_id = chunk_id;
    queue->q_head = (queue->q_head + 1) % queue->q_len;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

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

static int wait_for_request(MPII_cga_request_queue * queue, int i)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIC_Wait(queue->requests[i].req);
    MPIR_ERR_CHECK(mpi_errno);

    if (queue->requests[i].op_type == MPII_CGA_OP_RECV) {
        /* it's a recv, update pending_blocks */
        int chunk_id = queue->requests[i].chunk_id;
        queue->pending_blocks[chunk_id] = -1;
        if (queue->coll_type == MPII_CGA_BCAST) {
            if (queue->u.bcast.need_pack) {
                mpi_errno = bcast_recv_unpack(queue, chunk_id);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else if (queue->coll_type == MPII_CGA_ALLGATHER) {
            if (queue->u.allgather.need_pack) {
                mpi_errno = allgather_recv_unpack(queue, chunk_id);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else if (queue->coll_type == MPII_CGA_REDUCE) {
            mpi_errno = reduce_local(queue, chunk_id);
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

static int calc_chunks(MPI_Aint buf_size, MPI_Aint chunk_size, int *last_msg_size_out)
{
    int n;
    int last_msg_size;

    /* note: bcast zero sized messages is valid */
    if (chunk_size == 0 || buf_size == 0) {
        n = 1;
        last_msg_size = buf_size;
    } else {
        n = (buf_size / chunk_size);
        if (buf_size % chunk_size == 0) {
            last_msg_size = chunk_size;
        } else {
            n++;
            last_msg_size = buf_size % chunk_size;
        }
    }
    *last_msg_size_out = last_msg_size;
    return n;
}
