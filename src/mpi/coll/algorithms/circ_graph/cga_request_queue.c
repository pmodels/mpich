/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

static int calc_chunks(MPI_Aint data_size, MPI_Aint chunk_size, int *last_msg_size_out);

static int issue_send(MPII_cga_request_queue * queue, const void *buf, MPI_Aint count,
                      int peer_rank, int block, int root, int *req_idx_out);
static int issue_recv(MPII_cga_request_queue * queue, void *buf, MPI_Aint count,
                      int peer_rank, int block, int root, int *req_idx_out);

static int get_pending_id(MPII_cga_request_queue * queue, int block, int root);
static int get_pending_req_id(MPII_cga_request_queue * queue, int block, int root);
static void *get_pending_pack_buf(MPII_cga_request_queue * queue, int block, int root);
static void add_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx);
static int alloc_pending_pack_buf(MPII_cga_request_queue * queue, int block, int root,
                                  void **pack_buf_out);
static void remove_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx);

static int wait_if_full(MPII_cga_request_queue * queue);
static int wait_for_request(MPII_cga_request_queue * queue, int i);
static int reduce_local(MPII_cga_request_queue * queue, int block);

/* Routines for managing non-blocking send/recv of chunks  */
static int init_request_queue_common(MPII_cga_request_queue * queue,
                                     int q_len, int num_pending, int num_chunks, int all_size,
                                     MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    queue->comm = comm;
    queue->coll_attr = coll_attr;
    queue->num_chunks = num_chunks;
    queue->all_size = all_size;

    /* circular array of pending blocks
     *     block2pending(n): n % num_pending
     *     pending2block(i): pending_head_block + (i - pending_head) % num_pending
     */
    if (num_pending > num_chunks) {
        num_pending = num_chunks;
    }
    queue->num_pending = num_pending;
    queue->pending_head = 0;
    queue->pending_head_block = 0;
    /* circular array of pending requests */
    queue->q_len = q_len;
    queue->q_head = 0;

    queue->pending_blocks = MPL_malloc(all_size * num_pending * sizeof(*queue->pending_blocks),
                                       MPL_MEM_OTHER);
    if (!queue->pending_blocks) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    queue->requests = MPL_malloc(q_len * sizeof(*queue->requests), MPL_MEM_OTHER);
    if (!queue->requests) {
        MPL_free(queue->pending_blocks);
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
    }

    for (int i = 0; i < all_size * num_pending; i++) {
        /* -1 marks the block not pending (available to send) */
        queue->pending_blocks[i].req_id = -1;
        queue->pending_blocks[i].pack_buf = NULL;
    }

    for (int i = 0; i < q_len; i++) {
        queue->requests[i].req = NULL;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue, int num_pending,
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

    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, 1,
                                          comm, coll_attr);

    queue->coll_type = MPII_CGA_BCAST;
    queue->u.bcast.buf = buf;
    queue->u.bcast.need_pack = need_pack;
    if (need_pack) {
        queue->u.bcast.is_root = is_root;
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

int MPII_cga_init_allgather_queue(MPII_cga_request_queue * queue, int num_pending,
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

    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, all_size,
                                          comm, coll_attr);

    queue->coll_type = MPII_CGA_ALLGATHER;
    queue->u.allgather.buf = buf;
    queue->u.allgather.rank = rank;
    queue->u.allgather.comm_size = comm_size;
    queue->u.allgather.buf_size = count * type_size;
    queue->u.allgather.need_pack = need_pack;
    if (need_pack) {
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

int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue, int num_pending,
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

    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, 1,
                                          comm, coll_attr);

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

    int req_id = get_pending_req_id(queue, block, 0);
    if (req_id >= 0) {
        mpi_errno = wait_for_request(queue, req_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.bcast.need_pack) {
        void *pack_buf = get_pending_pack_buf(queue, block, 0);
        if (!pack_buf) {
            mpi_errno = alloc_pending_pack_buf(queue, block, 0, &pack_buf);
            MPIR_ERR_CHECK(mpi_errno);

            MPI_Aint actual_pack_bytes;
            MPI_Aint offset = block * queue->chunk_count;       /* this is in bytes */
            MPIR_Typerep_pack(queue->u.bcast.buf, queue->u.bcast.count, queue->u.bcast.datatype,
                              offset, pack_buf, count, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_pack_bytes == count);
        }
        buf = pack_buf;

    } else {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);
    }

    int req_idx /* unused */ ;
    mpi_errno = issue_send(queue, buf, count, peer_rank, block, 0, &req_idx);
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
        void *pack_buf = get_pending_pack_buf(queue, block, 0);
        if (!pack_buf) {
            mpi_errno = alloc_pending_pack_buf(queue, block, 0, &pack_buf);
            MPIR_ERR_CHECK(mpi_errno);
        }
        buf = pack_buf;
        /* unpack in wait_for_request() */
    } else {
        buf = GET_BLOCK_BUF(queue->u.bcast.buf, block);
    }

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, block, 0, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    add_pending_req_id(queue, block, 0, req_idx);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int bcast_recv_unpack(MPII_cga_request_queue * queue, int block)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = get_pending_pack_buf(queue, block, 0);

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

int MPII_cga_allgather_send(MPII_cga_request_queue * queue, int block, int root, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    int req_id = get_pending_req_id(queue, block, root);
    if (req_id >= 0) {
        mpi_errno = wait_for_request(queue, req_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.allgather.need_pack) {
        void *pack_buf = get_pending_pack_buf(queue, block, root);
        if (!pack_buf) {
            mpi_errno = alloc_pending_pack_buf(queue, block, root, &pack_buf);
            MPIR_ERR_CHECK(mpi_errno);

            MPI_Aint actual_pack_bytes;
            MPI_Aint offset = block * queue->chunk_count;       /* this is in bytes */
            void *src_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_extent;
            MPIR_Typerep_pack(src_buf, queue->u.allgather.count, queue->u.allgather.datatype,
                              offset, pack_buf, count, &actual_pack_bytes, MPIR_TYPEREP_FLAG_NONE);
            MPIR_Assert(actual_pack_bytes == count);
        }
        buf = pack_buf;
    } else {
        void *src_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(src_buf, block);
    }

    int req_idx /* unused */ ;
    mpi_errno = issue_send(queue, buf, count, peer_rank, block, root, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_allgather_recv(MPII_cga_request_queue * queue, int block, int root, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    void *buf;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    if (queue->u.allgather.need_pack) {
        void *pack_buf = get_pending_pack_buf(queue, block, root);
        if (!pack_buf) {
            mpi_errno = alloc_pending_pack_buf(queue, block, root, &pack_buf);
            MPIR_ERR_CHECK(mpi_errno);
        }
        buf = pack_buf;
    } else {
        void *dst_buf = (char *) queue->u.allgather.buf + root * queue->u.allgather.buf_size;
        buf = GET_BLOCK_BUF(dst_buf, block);
    }

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, block, root, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    add_pending_req_id(queue, block, root, req_idx);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int allgather_recv_unpack(MPII_cga_request_queue * queue, int chunk_id)
{
    int mpi_errno = MPI_SUCCESS;

    int root = chunk_id / queue->num_chunks;
    int block = chunk_id % queue->num_chunks;
    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = get_pending_pack_buf(queue, block, root);

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

    /* Dependency consideration for reduce:
     * * Recv - there are two operations, recv into tmp_buf and reduce into recvbuf.
     *          Recv into tmp_buf require clear of previous recv (with the same block).
     *          Reduction require clear of previous sends (with the same block).
     * * Send - assume we always issue send before recv, send require clear previous
     *          recv (from previous rounds with the same block) and clear all previous
     *          sends as part of recv completion (the reduction dependency). However,
     *          if there is no pending recv, multiple pending sends are ok.
     * Thus, we may have a single pending recv request and multiple pending send requests.
     */

    int req_id = get_pending_req_id(queue, block, 0);
    if (req_id >= 0 && queue->requests[req_id].op_type == MPII_CGA_OP_RECV) {
        mpi_errno = wait_for_request(queue, req_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = GET_BLOCK_BUF(queue->u.reduce.recvbuf, block);

    int req_idx;
    mpi_errno = issue_send(queue, buf, count, peer_rank, block, 0, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    add_pending_req_id(queue, block, 0, req_idx);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_reduce_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    /* Reference the comments in MPII_cga_reduce_send. Issuing recv only need clear
     * previous pending recvs
     */
    int req_id = get_pending_req_id(queue, block, 0);
    if (req_id >= 0 && queue->requests[req_id].op_type == MPII_CGA_OP_RECV) {
        mpi_errno = wait_for_request(queue, req_id);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPI_Aint count = GET_BLOCK_COUNT(block);
    void *buf = GET_BLOCK_BUF(queue->u.reduce.tmp_buf, block);

    int req_idx;
    mpi_errno = issue_recv(queue, buf, count, peer_rank, block, 0, &req_idx);
    MPIR_ERR_CHECK(mpi_errno);

    add_pending_req_id(queue, block, 0, req_idx);

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

    /* complete all requests */
    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].req) {
            mpi_errno = wait_for_request(queue, i);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }
    if (queue->coll_type == MPII_CGA_REDUCE) {
        MPL_free(queue->u.reduce.tmp_buf);
    }

    /* free all genq blocks */
    for (int i = 0; i < queue->num_pending; i++) {
        if (queue->pending_blocks[i].pack_buf) {
            MPIDU_genq_private_pool_free_cell(MPIR_cga_chunk_pool,
                                              queue->pending_blocks[i].pack_buf);
        }
    }

    /* free the memory */
    MPL_free(queue->pending_blocks);
    MPL_free(queue->requests);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* internal routines */

static int issue_send(MPII_cga_request_queue * queue, const void *buf, MPI_Aint count,
                      int peer_rank, int block, int root, int *req_idx_out)
{
    int mpi_errno;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

    *req_idx_out = queue->q_head;
    mpi_errno = MPIC_Isend(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req), queue->coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].op_type = MPII_CGA_OP_SEND;
    queue->requests[queue->q_head].block = block;
    queue->requests[queue->q_head].root = root;
    queue->q_head = (queue->q_head + 1) % queue->q_len;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_recv(MPII_cga_request_queue * queue, void *buf, MPI_Aint count,
                      int peer_rank, int block, int root, int *req_idx_out)
{
    int mpi_errno;

    mpi_errno = wait_if_full(queue);
    MPIR_ERR_CHECK(mpi_errno);

    *req_idx_out = queue->q_head;
    mpi_errno = MPIC_Irecv(buf, count, queue->datatype,
                           peer_rank, queue->tag, queue->comm,
                           &(queue->requests[queue->q_head].req));
    MPIR_ERR_CHECK(mpi_errno);

    queue->requests[queue->q_head].op_type = MPII_CGA_OP_RECV;
    queue->requests[queue->q_head].block = block;
    queue->requests[queue->q_head].root = root;
    queue->q_head = (queue->q_head + 1) % queue->q_len;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- routines for pending_blocks ---- */

static int get_pending_id(MPII_cga_request_queue * queue, int block, int root)
{
    if (block >= queue->pending_head_block &&
        block < queue->pending_head_block + queue->num_pending) {
        return (block % queue->num_pending) * queue->all_size + root;
    } else {
        return -1;
    }
}

static int get_new_pending_id(MPII_cga_request_queue * queue, int block, int root)
{
    /* The schedule must guarantee num_pending is sufficient */
    MPIR_Assert(block >= queue->pending_head_block);

    /* if block is beyond num_pending, vacate earlier blocks */
    if (block >= queue->pending_head_block + queue->num_pending) {
        int num_vacate = block - queue->num_pending + 1 - queue->pending_head_block;
        int k = queue->pending_head * queue->all_size;
        for (int i = 0; i < num_vacate; i++) {
            for (int j = 0; j < queue->all_size; j++) {
                int mpi_errno;
                if (queue->pending_blocks[k].req_id >= 0) {
                    mpi_errno = wait_for_request(queue, queue->pending_blocks[k].req_id);
                    MPIR_Assert(mpi_errno == MPI_SUCCESS);
                    queue->pending_blocks[k].req_id = -1;
                }
                if (queue->pending_blocks[k].pack_buf) {
                    MPIDU_genq_private_pool_free_cell(MPIR_cga_chunk_pool,
                                                      queue->pending_blocks[k].pack_buf);
                    queue->pending_blocks[k].pack_buf = NULL;
                }
                k++;
            }
            queue->pending_head++;
            if (queue->pending_head == queue->num_pending) {
                queue->pending_head = 0;
                k = 0;
            }
            queue->pending_head_block++;
        }
    }

    /* return the new entry */
    return (block % queue->num_pending) * queue->all_size + root;
}

static int get_pending_req_id(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    return (pending_id == -1) ? -1 : queue->pending_blocks[pending_id].req_id;
}

static void *get_pending_pack_buf(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    return (pending_id == -1) ? NULL : queue->pending_blocks[pending_id].pack_buf;
}

static void add_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_id)
{
    int pending_id = get_new_pending_id(queue, block, root);
    if (queue->coll_type != MPII_CGA_REDUCE) {
        /* simple case - at most a single pending request per block */
        queue->requests[req_id].next_req_id = -1;
        queue->pending_blocks[pending_id].req_id = req_id;
    } else {
        /* reduction may have a single pending recv and multiple pending send */
        if (queue->requests[req_id].op_type == MPII_CGA_OP_RECV) {
            /* prepend */
            queue->requests[req_id].next_req_id = queue->pending_blocks[pending_id].req_id;
            queue->pending_blocks[pending_id].req_id = req_id;
        } else {
            /* there is no dependency between pending sends, just insert after the pending recv (if exist) */
            int pending = queue->pending_blocks[pending_id].req_id;
            if (pending < 0) {
                /* no pending request */
                queue->requests[req_id].next_req_id = -1;
                queue->pending_blocks[pending_id].req_id = req_id;
            } else if (queue->requests[pending].op_type == MPII_CGA_OP_RECV) {
                /* insert after the first pending recv */
                queue->requests[req_id].next_req_id = queue->requests[pending].next_req_id;
                queue->requests[pending].next_req_id = req_id;
            } else {
                /* prepend */
                queue->requests[req_id].next_req_id = queue->pending_blocks[pending_id].req_id;
                queue->pending_blocks[pending_id].req_id = req_id;
            }
        }
    }
}

static int alloc_pending_pack_buf(MPII_cga_request_queue * queue, int block, int root,
                                  void **pack_buf_out)
{
    int pending_id = get_new_pending_id(queue, block, root);

    int mpi_errno;
    mpi_errno = MPIDU_genq_private_pool_force_alloc_cell(MPIR_cga_chunk_pool, pack_buf_out);

    queue->pending_blocks[pending_id].pack_buf = *pack_buf_out;

    return mpi_errno;
}

static void remove_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx)
{
    int pending_id = get_pending_id(queue, block, root);

#define PENDING queue->pending_blocks[pending_id]
    if (pending_id == -1 || PENDING.req_id == -1) {
        /* in bcast and allgather, sends are never added to pending_blocks */
        return;
    } else if (PENDING.req_id == req_idx) {
        PENDING.req_id = queue->requests[req_idx].next_req_id;
    } else {
        /* not the first pending recv. This can happen in a reduce in wait_if_full or
         * MPII_cga_waitall when we need complete a send request not due to dependency
         */
        MPIR_Assert(queue->coll_type == MPII_CGA_OP_RECV);
        int prev = PENDING.req_id;
        while (queue->requests[prev].next_req_id != req_idx) {
            MPIR_Assert(prev >= 0);
            prev = queue->requests[prev].next_req_id;
        }
        /* remove the entry from the linked list */
        queue->requests[prev].next_req_id = queue->requests[req_idx].next_req_id;
    }
#undef PENDING
}

/* ---- routines for request queue ---- */

static int wait_if_full(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->requests[queue->q_head].req) {
        mpi_errno = wait_for_request(queue, queue->q_head);
        MPIR_ERR_CHECK(mpi_errno);
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

    int block = queue->requests[i].block;
    int root = queue->requests[i].root;
    if (queue->requests[i].op_type == MPII_CGA_OP_RECV) {
        /* it's a recv, update pending_blocks */
        remove_pending_req_id(queue, block, root, i);
        if (queue->coll_type == MPII_CGA_BCAST) {
            if (queue->u.bcast.need_pack) {
                mpi_errno = bcast_recv_unpack(queue, block);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else if (queue->coll_type == MPII_CGA_ALLGATHER) {
            if (queue->u.allgather.need_pack) {
                mpi_errno = allgather_recv_unpack(queue, root * queue->num_chunks + block);
                MPIR_ERR_CHECK(mpi_errno);
            }
        } else if (queue->coll_type == MPII_CGA_REDUCE) {
            /* clear all pending sends */
            while (true) {
                int req_id = get_pending_req_id(queue, block, root);
                if (req_id == -1) {
                    break;
                }
                MPIR_Assert(queue->requests[req_id].op_type == MPII_CGA_OP_SEND);
                mpi_errno = wait_for_request(queue, req_id);
                MPIR_ERR_CHECK(mpi_errno);
            }
            mpi_errno = reduce_local(queue, block);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        remove_pending_req_id(queue, block, root, i);
    }
    MPIR_Request_free(queue->requests[i].req);
    queue->requests[i].req = NULL;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- math routines ---- */

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
