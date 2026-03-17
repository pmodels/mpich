/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "circ_graph.h"

/* This runtime uses a block-based dependency tracking.
 *
 * For now, we support two types of operations: broadcast and reduction.
 *
 * For each block, the pattern for broadcast is:
 *     [recv] -> send -> send -> ...
 * That is, it only recv once and all sends are issued after the recv. It is an inefficient
 * algorithm if it redunatantly recv the same data twice; and you can't send the data that you
 * don't have.
 *
 * For reduction, the pattern is:
 *     recv -> recv -> ... -> [send]
 * That is, you only send at last after you collected all the data and performed local reduction.
 * Otherwise, the algorithm is inefficient or inconsistent.
 */

static MPI_Aint calc_chunks(MPI_Aint data_size, MPI_Aint chunk_size, MPI_Aint * last_msg_size_out);

static int get_pending_id(MPII_cga_request_queue * queue, int block, int root);
static int get_pending_req_id(MPII_cga_request_queue * queue, int block, int root);
static void add_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx);
static void remove_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx);

/* if queue->need_pack, we allocate genq packbuf.
 * - for bcast, packbuf is stored in pending_blocks[].persist_packbuf, and freed with pending block
 * - for reduce, packbuf is stored in the requests[].packbuf, and freed with the request
 */
static void *get_persist_packbuf(MPII_cga_request_queue * queue, int block, int root);
static bool is_persist_packbuf_loaded(MPII_cga_request_queue * queue, int block, int root);
static void add_persist_packbuf(MPII_cga_request_queue * queue, int block, int root, void *packbuf);
static void set_persist_packbuf_loaded(MPII_cga_request_queue * queue, int block, int root);
static int alloc_packbuf(MPII_cga_request_queue * queue, void **packbuf_out);
/* free buffers depend on where they are stored */
static void clear_request(MPII_cga_request_queue * queue, int req_id);
static void clear_pending(MPII_cga_request_queue * queue, int pending_id);

static int reduce_local(MPII_cga_request_queue * queue, int block, void *buf);

static int issue_nb_op(MPII_cga_request_queue * queue, enum MPII_cga_op_type op_type,
                       int peer_rank, int block, int root, bool * flag);
static int issue_pack(MPII_cga_request_queue * queue, int block, int root,
                      void *packbuf, MPIR_gpu_req * areq);
static int issue_unpack(MPII_cga_request_queue * queue, int block, int root,
                        void *packbuf, void *tmpbuf, MPIR_gpu_req * areq);
static int issue_isend_contig(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, MPIR_Request ** req);
static int issue_isend_packed(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *packbuf, MPIR_Request ** req);
static int issue_irecv_contig(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *tmpbuf, MPIR_Request ** req);
static int issue_irecv_packed(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *packbuf, MPIR_Request ** req);

static int test_new_pending_block(MPII_cga_request_queue * queue, int block, int root, bool * flag);

/* return true all pending recvs on the same block are COMPLETED */
static int clear_pending_recvs(MPII_cga_request_queue * queue, int cur_req_id, bool * flag);
/* return true if the pending requests of the same op_type and peer_rank are ISSUED. */
static int check_pending_ops(MPII_cga_request_queue * queue, int cur_req_id, bool * flag);

/* test and make progress */
static int test_for_request(MPII_cga_request_queue * queue, int i, bool * is_done);
static int progress_for_request(MPII_cga_request_queue * queue, int i);

/* define DEBUG to enable more detailed progress debugging */

#ifdef DEBUG
/* debug routines */
static void debug_queue(MPII_cga_request_queue * queue);

#undef DEBUG_PROGRESS_HOOK
#define DEBUG_PROGREE_HOOK \
    do { \
        debug_queue(queue); \
        MPL_backtrace_show(stdout); \
    } while (0)
#endif

/* Routines for managing non-blocking send/recv of chunks  */
static int init_request_queue_common(MPII_cga_request_queue * queue,
                                     int q_len, int num_pending, MPI_Aint num_chunks, int all_size,
                                     MPI_Aint chunk_size, MPI_Aint last_chunk_size,
                                     bool inverse_order,
                                     void *buf, MPI_Aint count, MPI_Datatype datatype,
                                     MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    int dt_contig;
    MPIR_Datatype_is_contig(datatype, &dt_contig);

    MPIR_GPU_query_pointer_attr(buf, &queue->attr);
    queue->need_pack = (!dt_contig || MPL_gpu_attr_is_dev(&queue->attr));

    queue->comm = comm;
    queue->coll_attr = coll_attr;
    queue->num_chunks = num_chunks;
    queue->all_size = all_size;
    queue->chunk_size = chunk_size;
    queue->last_chunk_size = last_chunk_size;
    queue->buf = buf;
    queue->count = count;
    queue->datatype = datatype;
    queue->inverse_order = inverse_order;

    mpi_errno = MPIR_Sched_next_tag(comm, &queue->tag);
    MPIR_ERR_CHECK(mpi_errno);

    /* circular array of pending blocks
     *     block2pending(n): n % num_pending
     *     pending2block(i): pending_head_block + (i - pending_head) % num_pending
     */
    if (num_pending > num_chunks) {
        num_pending = num_chunks;
    }
    queue->num_pending = num_pending;

    if (!queue->inverse_order) {
        queue->pending_head_block = 0;
    } else {
        queue->pending_head_block = queue->num_chunks - 1;
    }
    queue->pending_head = queue->pending_head_block % queue->num_pending;

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
        queue->pending_blocks[i].persist_packbuf = NULL;
    }

    for (int i = 0; i < q_len; i++) {
        queue->requests[i].op_stage = MPII_CGA_STAGE_NULL;
        queue->requests[i].packbuf = NULL;
        queue->requests[i].tmpbuf = NULL;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_init_bcast_queue(MPII_cga_request_queue * queue, int num_pending,
                              void *buf, MPI_Aint count, MPI_Datatype datatype,
                              MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno;

    int chunk_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    int q_len = MPIR_CVAR_CIRC_GRAPH_Q_LEN;

    /* minimum q_len is 2 to avoid circular dependency issues within a round */
    if (q_len < 2) {
        q_len = 2;
    }

    MPI_Aint type_size, data_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    data_size = count * type_size;

    MPI_Aint last_chunk_size;
    MPI_Aint num_chunks = calc_chunks(data_size, chunk_size, &last_chunk_size);
    bool inverse_order = false;

    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, 1,
                                          chunk_size, last_chunk_size, inverse_order,
                                          buf, count, datatype, comm, coll_attr);

    queue->coll_type = MPII_CGA_BCAST;
    queue->buf_extent = 0;

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

    MPI_Aint type_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);

    MPI_Aint last_chunk_size;
    MPI_Aint num_chunks = calc_chunks(count * type_size, chunk_size, &last_chunk_size);
    int all_size = comm_size;
    bool inverse_order = false;

    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, all_size,
                                          chunk_size, last_chunk_size, inverse_order,
                                          buf, count, datatype, comm, coll_attr);

    queue->coll_type = MPII_CGA_BCAST;

    MPI_Aint extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    queue->buf_extent = count * extent;

    return mpi_errno;
}

int MPII_cga_init_reduce_queue(MPII_cga_request_queue * queue, int num_pending,
                               void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                               MPI_Op op, MPIR_Comm * comm, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint chunk_size = MPIR_CVAR_CIRC_GRAPH_CHUNK_SIZE;
    int q_len = MPIR_CVAR_CIRC_GRAPH_Q_LEN;

    /* minimum q_len is 2 */
    if (q_len < 2) {
        q_len = 2;
    }

    MPI_Aint type_size;
    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Assert(type_size > 0);

    MPI_Aint num_chunks, chunk_count, last_chunk_count, last_chunk_size;
    if (chunk_size == 0) {
        num_chunks = 1;
        chunk_count = count;
        last_chunk_count = count;
    } else {
        /* reduction chunks have to contain whole datatypes */
        chunk_count = chunk_size / type_size;

        num_chunks = count / chunk_count;
        last_chunk_count = count % chunk_count;
        if (last_chunk_count == 0) {
            last_chunk_count = chunk_count;
        } else {
            num_chunks++;
        }
    }
    chunk_size = chunk_count * type_size;
    last_chunk_size = last_chunk_count * type_size;

    MPI_Aint extent, true_lb, true_extent;
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    bool inverse_order = true;
    mpi_errno = init_request_queue_common(queue, q_len, num_pending, num_chunks, 1,
                                          chunk_size, last_chunk_size, inverse_order,
                                          recvbuf, count, datatype, comm, coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

    queue->coll_type = MPII_CGA_REDUCE;
    queue->u.reduce.op = op;
    queue->u.reduce.type_size = type_size;
    queue->u.reduce.type_extent = extent;
    queue->u.reduce.true_lb = true_lb;
    queue->u.reduce.chunk_extent = chunk_count * extent;
    if (!queue->need_pack) {
        /* datatype must be contig, no pack_buf, need tmpbuf to receive chunk data */
        queue->u.reduce.tmpbuf_size = chunk_size;
    } else if (type_size == true_extent && type_size == extent) {
        /* datatype is contig, skip tmpbuf, we can do reduce from pack_buf */
        queue->u.reduce.tmpbuf_size = 0;
    } else {
        /* non-contig case, we need both pack_buf and tmpbuf */
        queue->u.reduce.tmpbuf_size = (chunk_count - 1) * extent + true_extent;
    }

  fn_fail:
    return mpi_errno;
}

int MPII_cga_switch_coll_type(MPII_cga_request_queue * queue, enum MPII_cga_type coll_type)
{
    /* for now, we only support switching from REDUCE to BCAST (as an ALLREDUCE composition) */
    MPIR_Assert(queue->coll_type == MPII_CGA_REDUCE && coll_type == MPII_CGA_BCAST);
    queue->coll_type = MPII_CGA_BCAST;
    queue->inverse_order = false;
    /* at this point, pending_blocks must be tracking 0, 1, ..., num_pending-1 */
    MPIR_Assert(queue->pending_head_block == -1);
    queue->pending_head_block = queue->num_pending;
    queue->pending_head = 0;

    return MPI_SUCCESS;
}

#define GET_BLOCK_SIZE(block)     (((block) == queue->num_chunks - 1) ? queue->last_chunk_size : queue->chunk_size)

/* ---- bcast ---- */

int MPII_cga_bcast_isend(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_SEND, peer_rank, block, 0, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_bcast_irecv(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_RECV, peer_rank, block, 0, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_bcast_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_bcast_isend(queue, block, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_bcast_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_bcast_irecv(queue, block, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- allgather ---- */

int MPII_cga_allgather_isend(MPII_cga_request_queue * queue, int block, int root, int peer_rank,
                             bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_SEND, peer_rank, block, root, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_allgather_irecv(MPII_cga_request_queue * queue, int block, int root, int peer_rank,
                             bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_RECV, peer_rank, block, root, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_allgather_send(MPII_cga_request_queue * queue, int block, int root, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_allgather_isend(queue, block, root, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_allgather_recv(MPII_cga_request_queue * queue, int block, int root, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_allgather_irecv(queue, block, root, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- reduce ---- */

int MPII_cga_reduce_isend(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_SEND, peer_rank, block, 0, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_reduce_irecv(MPII_cga_request_queue * queue, int block, int peer_rank, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = issue_nb_op(queue, MPII_CGA_OP_RECV, peer_rank, block, 0, flag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_reduce_send(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_reduce_isend(queue, block, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_reduce_recv(MPII_cga_request_queue * queue, int block, int peer_rank)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_reduce_irecv(queue, block, peer_rank, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int reduce_local(MPII_cga_request_queue * queue, int block, void *buf)
{
    int mpi_errno = MPI_SUCCESS;

    void *buf_in = (char *) buf - queue->u.reduce.true_lb;
    void *buf_inout = (char *) queue->buf + block * queue->u.reduce.chunk_extent;
    MPI_Aint count = GET_BLOCK_SIZE(block) / queue->u.reduce.type_size;

    mpi_errno = MPIR_Reduce_local(buf_in, buf_inout, count, queue->datatype, queue->u.reduce.op);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ---- final completion ---- */

int MPII_cga_waitall(MPII_cga_request_queue * queue)
{
    int mpi_errno = MPI_SUCCESS;

    bool flag = false;
    DEBUG_PROGRESS_START;
    while (!flag) {
        mpi_errno = MPII_cga_testall(queue, &flag);
        MPIR_ERR_CHECK(mpi_errno);

        MPID_Progress_test(NULL);
        DEBUG_PROGRESS_CHECK;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPII_cga_testall(MPII_cga_request_queue * queue, bool * is_done)
{
    int mpi_errno = MPI_SUCCESS;

    /* complete all requests */
    int j = queue->q_head;
    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[j].op_stage != MPII_CGA_STAGE_NULL) {
            bool flag;
            mpi_errno = test_for_request(queue, j, &flag);
            MPIR_ERR_CHECK(mpi_errno);
            if (!flag) {
                goto fn_cont;
            }
        }
        j++;
        if (j == queue->q_len) {
            j = 0;
        }
    }

    /* free all genq blocks */
    for (int i = 0; i < queue->num_pending; i++) {
        clear_pending(queue, i);
    }

    /* free the memory */
    MPL_free(queue->pending_blocks);
    MPL_free(queue->requests);

    *is_done = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_cont:
    *is_done = false;
    return mpi_errno;
}

/* internal routines */

static int issue_nb_op(MPII_cga_request_queue * queue, enum MPII_cga_op_type op_type,
                       int peer_rank, int block, int root, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    /* if the requests slot is busy, we need wait */
    if (queue->requests[queue->q_head].op_stage != MPII_CGA_STAGE_NULL) {
        mpi_errno = test_for_request(queue, queue->q_head, flag);
        MPIR_ERR_CHECK(mpi_errno);
        if (!*flag) {
            goto fn_exit;
        }
    }

    /* if we run out of pending in order to track this block, we wait */
    if (get_pending_id(queue, block, root) == -1) {
        mpi_errno = test_new_pending_block(queue, block, root, flag);
        MPIR_ERR_CHECK(mpi_errno);
        if (!*flag) {
            goto fn_exit;
        }
    }

    int req_id = queue->q_head;
#define REQi queue->requests[req_id]
    REQi.op_type = op_type;
    REQi.coll_type = queue->coll_type;
    REQi.peer_rank = peer_rank;
    REQi.block = block;
    REQi.root = root;
    REQi.op_stage = MPII_CGA_STAGE_START;
    REQi.issued = false;

    REQi.tmpbuf = NULL;
    if (op_type == MPII_CGA_OP_RECV && REQi.coll_type == MPII_CGA_REDUCE) {
        /* reduce need recv (or unpack) into a tmp buffer before reduce_local */
        if (queue->u.reduce.tmpbuf_size > 0) {
            REQi.tmpbuf = MPL_malloc(queue->u.reduce.tmpbuf_size, MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!REQi.tmpbuf, mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
    }
#undef REQi

    queue->q_head = (queue->q_head + 1) % queue->q_len;
    add_pending_req_id(queue, block, root, req_id);

    *flag = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int issue_unpack(MPII_cga_request_queue * queue, int block, int root,
                        void *packbuf, void *tmpbuf, MPIR_gpu_req * areq)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);

    void *dst_buf;
    MPI_Aint offset;
    if (tmpbuf) {
        /* reduce need unpack into a tmpbuf before reduce_local */
        dst_buf = (char *) tmpbuf - queue->u.reduce.true_lb;
        offset = 0;
    } else {
        if (root == 0) {
            dst_buf = queue->buf;
        } else {
            dst_buf = (char *) queue->buf + root * queue->buf_extent;
        }
        offset = block * queue->chunk_size;
    }

    int engine_type = MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;  /* TODO: add a cvar */
    mpi_errno = MPIR_Ilocalcopy_gpu(packbuf, nbytes, MPIR_BYTE_INTERNAL, 0, NULL,
                                    dst_buf, queue->count, queue->datatype, offset, &queue->attr,
                                    engine_type, 1, areq);

    return mpi_errno;
}

static int issue_pack(MPII_cga_request_queue * queue, int block, int root,
                      void *packbuf, MPIR_gpu_req * areq)
{
    int mpi_errno = MPI_SUCCESS;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);
    void *src_buf = queue->buf;
    if (root > 0) {
        src_buf = (char *) queue->buf + root * queue->buf_extent;
    }

    int engine_type = MPL_GPU_ENGINE_TYPE_COPY_HIGH_BANDWIDTH;  /* TODO: add a cvar */
    MPI_Aint offset = block * queue->chunk_size;
    mpi_errno = MPIR_Ilocalcopy_gpu(src_buf, queue->count, queue->datatype, offset, &queue->attr,
                                    packbuf, nbytes, MPIR_BYTE_INTERNAL, 0, NULL, engine_type, 1,
                                    areq);

    return mpi_errno;
}

static int issue_isend_contig(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, MPIR_Request ** req)
{
    int mpi_errno;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);
    MPI_Aint offset = block * queue->chunk_size;
    const void *send_buf;
    if (root == 0) {
        send_buf = (char *) queue->buf + offset;
    } else {
        send_buf = (char *) queue->buf + root * queue->buf_extent + offset;
    }
    mpi_errno = MPIC_Isend(send_buf, nbytes, MPIR_BYTE_INTERNAL, peer_rank, queue->tag, queue->comm,
                           req, queue->coll_attr);
    return mpi_errno;
}

static int issue_isend_packed(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *packbuf, MPIR_Request ** req)
{
    int mpi_errno;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);
    mpi_errno = MPIC_Isend(packbuf, nbytes, MPIR_BYTE_INTERNAL, peer_rank, queue->tag, queue->comm,
                           req, queue->coll_attr);
    return mpi_errno;
}

static int issue_irecv_contig(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *tmpbuf, MPIR_Request ** req)
{
    int mpi_errno;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);
    MPI_Aint offset = block * queue->chunk_size;
    void *recv_buf;
    if (tmpbuf) {
        /* reduce recv into a tmp buffer */
        recv_buf = tmpbuf;
    } else {
        if (root == 0) {
            recv_buf = (char *) queue->buf + offset;
        } else {
            recv_buf = (char *) queue->buf + root * queue->buf_extent + offset;
        }
    }

    mpi_errno = MPIC_Irecv(recv_buf, nbytes, MPIR_BYTE_INTERNAL, peer_rank, queue->tag, queue->comm,
                           req);

    return mpi_errno;
}

static int issue_irecv_packed(MPII_cga_request_queue * queue, int block, int root,
                              int peer_rank, void *packbuf, MPIR_Request ** req)
{
    int mpi_errno;

    MPI_Aint nbytes = GET_BLOCK_SIZE(block);
    mpi_errno = MPIC_Irecv(packbuf, nbytes, MPIR_BYTE_INTERNAL, peer_rank, queue->tag, queue->comm,
                           req);
    return mpi_errno;
}

/* ---- routines for pending_blocks ---- */

static int get_pending_id(MPII_cga_request_queue * queue, int block, int root)
{
    if (!queue->inverse_order) {
        /* forward, HEAD is the larges block: [..., i-2, i-1, i(HEAD), i-N+1, ...] */
        if (block < queue->pending_head_block &&
            block >= queue->pending_head_block - queue->num_pending) {
            return (block % queue->num_pending) * queue->all_size + root;
        } else {
            return -1;
        }
    } else {
        /* inverse, HEAD is the lowest block: [..., i+N-1, i(HEAD), i+1, i+2, ...] */
        if (block > queue->pending_head_block &&
            block <= queue->pending_head_block + queue->num_pending) {
            return (block % queue->num_pending) * queue->all_size + root;
        } else {
            return -1;
        }
    }
}

/* check whether we have room for tracking a new pending block */
static int test_new_pending_block(MPII_cga_request_queue * queue, int block, int root, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    /* move pending_head num_beyond positions, starting at pending_head */
    int num_beyond;
    if (!queue->inverse_order) {
        num_beyond = block - queue->pending_head_block + 1;
    } else {
        num_beyond = queue->pending_head_block - block + 1;
    }

    /* if block is beyond pending_head, vacate earlier blocks */
    if (num_beyond > 0) {
        int k = queue->pending_head * queue->all_size;
        for (int i = 0; i < num_beyond; i++) {
            for (int j = 0; j < queue->all_size; j++) {
#define PENDING queue->pending_blocks[k + j]
                while (PENDING.req_id >= 0) {
                    mpi_errno = test_for_request(queue, PENDING.req_id, flag);
                    MPIR_ERR_CHECK(mpi_errno);
                    if (!*flag) {
                        goto fn_exit;
                    }
                }
                clear_pending(queue, k + j);
#undef PENDING
            }
            if (!queue->inverse_order) {
                queue->pending_head++;
                k += queue->all_size;
                if (queue->pending_head == queue->num_pending) {
                    queue->pending_head = 0;
                    k = 0;
                }
                queue->pending_head_block++;
            } else {
                if (queue->pending_head > 0) {
                    queue->pending_head--;
                    k -= queue->all_size;
                } else {
                    queue->pending_head = queue->num_pending - 1;
                    k = queue->pending_head * queue->all_size;
                }
                queue->pending_head_block--;
            }
        }
    }
    *flag = true;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_pending_req_id(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    return (pending_id == -1) ? -1 : queue->pending_blocks[pending_id].req_id;
}

static void *get_persist_packbuf(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);
    return queue->pending_blocks[pending_id].persist_packbuf;
}

static bool is_persist_packbuf_loaded(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);
    MPIR_Assert(queue->pending_blocks[pending_id].persist_packbuf);
    return queue->pending_blocks[pending_id].persist_packbuf_loaded;
}

static void add_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_id)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);

    int pending = queue->pending_blocks[pending_id].req_id;
    if (pending < 0) {
        /* no pending request, just add this */
        queue->pending_blocks[pending_id].req_id = req_id;
        queue->requests[req_id].next_req_id = -1;
    } else {
        /* append this request */
        while (queue->requests[pending].next_req_id >= 0) {
            pending = queue->requests[pending].next_req_id;
        }
        queue->requests[pending].next_req_id = req_id;
        queue->requests[req_id].next_req_id = -1;
    }
}

static void add_persist_packbuf(MPII_cga_request_queue * queue, int block, int root, void *packbuf)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);
    queue->pending_blocks[pending_id].persist_packbuf = packbuf;
    queue->pending_blocks[pending_id].persist_packbuf_loaded = false;
}

static void set_persist_packbuf_loaded(MPII_cga_request_queue * queue, int block, int root)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);
    MPIR_Assert(queue->pending_blocks[pending_id].persist_packbuf);
    queue->pending_blocks[pending_id].persist_packbuf_loaded = true;
}

static int alloc_packbuf(MPII_cga_request_queue * queue, void **packbuf_out)
{
    int mpi_errno = MPI_SUCCESS;
    if (MPIR_cga_chunk_pool) {
        mpi_errno = MPIDU_genq_private_pool_force_alloc_cell(MPIR_cga_chunk_pool, packbuf_out);
    } else {
        *packbuf_out = MPL_malloc(queue->chunk_size, MPL_MEM_COLL);
        if (!(*packbuf_out)) {
            MPIR_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
    }
    return mpi_errno;
}

static void clear_request(MPII_cga_request_queue * queue, int req_id)
{
#define REQi queue->requests[req_id]
    if (REQi.coll_type == MPII_CGA_REDUCE) {
        if (REQi.packbuf) {
            if (MPIR_cga_chunk_pool) {
                MPIDU_genq_private_pool_free_cell(MPIR_cga_chunk_pool, REQi.packbuf);
            } else {
                MPL_free(REQi.packbuf);
            }
            REQi.packbuf = NULL;
        }
        if (REQi.tmpbuf) {
            MPL_free(REQi.tmpbuf);
            REQi.tmpbuf = NULL;
        }
    }
#undef REQi
}

static void clear_pending(MPII_cga_request_queue * queue, int pending_id)
{
#define PENDING queue->pending_blocks[pending_id]
    if (PENDING.persist_packbuf) {
        if (MPIR_cga_chunk_pool) {
            MPIDU_genq_private_pool_free_cell(MPIR_cga_chunk_pool, PENDING.persist_packbuf);
        } else {
            MPL_free(PENDING.persist_packbuf);
        }
        PENDING.persist_packbuf = NULL;
    }
#undef PENDING
}

static void remove_pending_req_id(MPII_cga_request_queue * queue, int block, int root, int req_idx)
{
    int pending_id = get_pending_id(queue, block, root);
    MPIR_Assert(pending_id >= 0);

#define PENDING queue->pending_blocks[pending_id]
    if (PENDING.req_id == req_idx) {
        PENDING.req_id = queue->requests[req_idx].next_req_id;
    } else {
        int prev = PENDING.req_id;
        while (queue->requests[prev].next_req_id != req_idx) {
            prev = queue->requests[prev].next_req_id;
            MPIR_Assert(prev >= 0);
        }
        /* remove the entry from the linked list */
        queue->requests[prev].next_req_id = queue->requests[req_idx].next_req_id;
    }
#undef PENDING
}

/* ---- tests for progress ---- */

static int clear_pending_recvs(MPII_cga_request_queue * queue, int cur_req_id, bool * flag)
{
    int block = queue->requests[cur_req_id].block;
    int root = queue->requests[cur_req_id].root;

    int req_id = get_pending_req_id(queue, block, root);
    while (true) {
        MPIR_Assert(req_id >= 0);
        if (req_id == cur_req_id) {
            *flag = true;
            break;
        }
        if (queue->requests[req_id].op_type == MPII_CGA_OP_RECV) {
            *flag = false;
            break;
        }
        req_id = queue->requests[req_id].next_req_id;
    }

    return MPI_SUCCESS;
}

static int check_pending_ops(MPII_cga_request_queue * queue, int cur_req_id, bool * flag)
{
    int mpi_errno = MPI_SUCCESS;

    /* send or recv */
    int op_type = queue->requests[cur_req_id].op_type;
    int peer_rank = queue->requests[cur_req_id].peer_rank;

    int req_id = cur_req_id;
    while (true) {
        /* check previous requests until hit q_head */
        if (req_id == queue->q_head) {
            break;
        }
        req_id -= 1;
        if (req_id < 0) {
            req_id = queue->q_len - 1;
        }
        /* The goal is to ensure all sends or recvs to the same rank are issued in order.
         * Thus, we check that no previous send (or recv) are in a stage before issuing.
         */
        if (queue->requests[req_id].op_stage != MPII_CGA_STAGE_NULL &&
            queue->requests[req_id].op_type == op_type &&
            queue->requests[req_id].peer_rank == peer_rank) {
            if (op_type == MPII_CGA_OP_SEND) {
                if (queue->requests[req_id].op_stage == MPII_CGA_STAGE_START ||
                    queue->requests[req_id].op_stage == MPII_CGA_STAGE_COPY) {
                    *flag = false;
                    goto fn_exit;
                }
            } else {    /* MPII_CGA_OP_RECV */
                if (queue->requests[req_id].op_stage == MPII_CGA_STAGE_START) {
                    *flag = false;
                    goto fn_exit;
                }
            }
        }
    }

    *flag = true;

  fn_exit:
    return mpi_errno;
}

/* -- test_for_request: main function that makes progress ---- */

#define TEST_PENDING(FUNC) \
    do { \
        bool flag; \
        mpi_errno = FUNC; \
        MPIR_ERR_CHECK(mpi_errno); \
        if (!flag) { \
            goto fn_cont; \
        } \
    } while (0)

static int test_for_request(MPII_cga_request_queue * queue, int req_id, bool * is_done)
{
    int mpi_errno = MPI_SUCCESS;

    if (queue->requests[req_id].op_stage == MPII_CGA_STAGE_NULL) {
        *is_done = true;
        goto fn_exit;
    }

    /* try make progress */
    for (int i = 0; i < queue->q_len; i++) {
        if (queue->requests[i].op_stage != MPII_CGA_STAGE_NULL) {
            mpi_errno = progress_for_request(queue, i);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    *is_done = (queue->requests[req_id].op_stage == MPII_CGA_STAGE_NULL);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int progress_for_request(MPII_cga_request_queue * queue, int i)
{
    int mpi_errno = MPI_SUCCESS;

#define REQi queue->requests[i]
    int block = REQi.block;
    int root = REQi.root;
    while (true) {
        /* depend on op_stage, test and transition to next stage, goto fn_cont when we stuck in progress */
        if (REQi.op_stage == MPII_CGA_STAGE_START) {
            if (REQi.op_type == MPII_CGA_OP_SEND) {
                /* send need clear previous recvs or the data is incorrect */
                TEST_PENDING(clear_pending_recvs(queue, i, &flag));
                if (queue->need_pack) {
                    void *pack_buf = NULL;
                    bool new_pack_buf = false;
                    if (REQi.coll_type == MPII_CGA_REDUCE) {
                        /* reduce can't reuse pack buffer */
                        mpi_errno = alloc_packbuf(queue, &pack_buf);
                        MPIR_ERR_CHECK(mpi_errno);

                        REQi.packbuf = pack_buf;
                        new_pack_buf = true;
                    } else {    /* MPII_CGA_BCAST */
                        pack_buf = get_persist_packbuf(queue, block, root);
                        if (!pack_buf) {
                            mpi_errno = alloc_packbuf(queue, &pack_buf);
                            MPIR_ERR_CHECK(mpi_errno);

                            REQi.packbuf = pack_buf;
                            add_persist_packbuf(queue, block, root, pack_buf);
                            new_pack_buf = true;
                        } else {
                            /* make sure the packbuf is loaded */
                            if (!is_persist_packbuf_loaded(queue, block, root)) {
                                goto fn_cont;
                            }
                            REQi.packbuf = pack_buf;
                        }
                    }
                    if (new_pack_buf) {
                        mpi_errno = issue_pack(queue, block, root, pack_buf, &REQi.u.async_req);
                        MPIR_ERR_CHECK(mpi_errno);
                        REQi.op_stage = MPII_CGA_STAGE_COPY;
                    } else {
                        /* make sure all sends are in order */
                        TEST_PENDING(check_pending_ops(queue, i, &flag));
                        mpi_errno = issue_isend_packed(queue, block, root, REQi.peer_rank,
                                                       REQi.packbuf, &REQi.u.req);
                        MPIR_ERR_CHECK(mpi_errno);
                        REQi.issued = true;
                        REQi.op_stage = MPII_CGA_STAGE_REQUEST;
                    }
                } else {
                    TEST_PENDING(check_pending_ops(queue, i, &flag));
                    mpi_errno = issue_isend_contig(queue, block, root, REQi.peer_rank, &REQi.u.req);
                    MPIR_ERR_CHECK(mpi_errno);
                    REQi.issued = true;
                    REQi.op_stage = MPII_CGA_STAGE_REQUEST;
                }
            } else {    /* MPII_CGA_OP_RECV */
                if (queue->need_pack) {
                    /* make sure all recvs are in order */
                    TEST_PENDING(check_pending_ops(queue, i, &flag));
                    void *pack_buf;
                    mpi_errno = alloc_packbuf(queue, &pack_buf);
                    MPIR_ERR_CHECK(mpi_errno);

                    REQi.packbuf = pack_buf;
                    if (REQi.coll_type == MPII_CGA_BCAST) {
                        add_persist_packbuf(queue, block, root, pack_buf);
                    }

                    mpi_errno = issue_irecv_packed(queue, block, root, REQi.peer_rank,
                                                   pack_buf, &REQi.u.req);
                } else {
                    /* make sure all recvs are in order */
                    /* NOTE: no need for clear_pending_recvs.
                     *       Bcast only have 1 recv each block. Reduce recv into tmpbuf. */
                    TEST_PENDING(check_pending_ops(queue, i, &flag));
                    mpi_errno = issue_irecv_contig(queue, block, root, REQi.peer_rank,
                                                   REQi.tmpbuf, &REQi.u.req);
                }
                MPIR_ERR_CHECK(mpi_errno);
                REQi.issued = true;
                REQi.op_stage = MPII_CGA_STAGE_REQUEST;
            }
        } else if (REQi.op_stage == MPII_CGA_STAGE_COPY) {
            int done;
            MPIR_async_test(&REQi.u.async_req, &done);
            if (!done) {
                goto fn_cont;
            }

            /* -- transition -- */
            if (REQi.op_type == MPII_CGA_OP_SEND) {
                /* make sure all sends are in order */
                TEST_PENDING(check_pending_ops(queue, i, &flag));
                if (REQi.coll_type == MPII_CGA_BCAST) {
                    set_persist_packbuf_loaded(queue, block, root);
                }
                mpi_errno = issue_isend_packed(queue, block, root, REQi.peer_rank,
                                               REQi.packbuf, &REQi.u.req);
                MPIR_ERR_CHECK(mpi_errno);
                REQi.issued = true;
                REQi.op_stage = MPII_CGA_STAGE_REQUEST;
            } else {
                if (REQi.coll_type == MPII_CGA_REDUCE) {
                    /* can't have multiple reduce into the same buffer */
                    TEST_PENDING(clear_pending_recvs(queue, i, &flag));
                    /* blocking reduce will be done in the next loop.
                     * TODO: issue nonblocking reduce here */
                    REQi.op_stage = MPII_CGA_STAGE_REDUCE;
                } else {
                    goto fn_complete;
                }
            }
        } else if (REQi.op_stage == MPII_CGA_STAGE_REQUEST) {
            if (!MPIR_Request_is_complete(REQi.u.req)) {
                goto fn_cont;
            }

            /* -- transition -- */
            MPIR_Assert(REQi.u.req->status.MPI_ERROR == MPI_SUCCESS);
            MPIR_Request_free(REQi.u.req);

            if (REQi.op_type == MPII_CGA_OP_SEND) {
                goto fn_complete;
            } else {
                if (queue->need_pack) {
                    if (REQi.coll_type == MPII_CGA_BCAST) {
                        set_persist_packbuf_loaded(queue, block, root);
                    }
                    if (REQi.coll_type == MPII_CGA_REDUCE && !REQi.tmpbuf) {
                        /* contig (gpu) case, we can directly reduce from pack_buf */
                        REQi.op_stage = MPII_CGA_STAGE_REDUCE;
                    } else {
                        /* no need for checking dependency. See earlier comment */
                        mpi_errno = issue_unpack(queue, block, root,
                                                 REQi.packbuf, REQi.tmpbuf, &REQi.u.async_req);
                        MPIR_ERR_CHECK(mpi_errno);
                        REQi.op_stage = MPII_CGA_STAGE_COPY;
                    }
                } else {
                    if (REQi.coll_type == MPII_CGA_REDUCE) {
                        REQi.op_stage = MPII_CGA_STAGE_REDUCE;
                    } else {
                        goto fn_complete;
                    }
                }
            }
        } else if (REQi.op_stage == MPII_CGA_STAGE_REDUCE) {
            /* can't have multiple reduce into the same buffer */
            TEST_PENDING(clear_pending_recvs(queue, i, &flag));

            void *buf = REQi.tmpbuf ? REQi.tmpbuf : REQi.packbuf;
            mpi_errno = reduce_local(queue, block, buf);
            MPIR_ERR_CHECK(mpi_errno);

            goto fn_complete;
        } else {
            MPIR_Assert(0);
        }
    }
#undef REQi

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
  fn_cont:
    return mpi_errno;
  fn_complete:
    remove_pending_req_id(queue, block, root, i);
    queue->requests[i].op_stage = MPII_CGA_STAGE_NULL;
    clear_request(queue, i);
    return mpi_errno;
}

/* ---- debug routines ---- */

#ifdef DEBUG

static const char *op_type_str(int op_type)
{
    switch (op_type) {
        case MPII_CGA_OP_SEND:
            return "SEND";
        case MPII_CGA_OP_RECV:
            return "RECV";
        default:
            return "UNKNOWN";
    }
}

static const char *op_stage_str(int op_stage)
{
    switch (op_stage) {
        case MPII_CGA_STAGE_NULL:
            return "NULL";
        case MPII_CGA_STAGE_START:
            return "START";
        case MPII_CGA_STAGE_COPY:
            return "COPY";
        case MPII_CGA_STAGE_REQUEST:
            return "REQUEST";
        case MPII_CGA_STAGE_REDUCE:
            return "REDUCE";
        default:
            return "UNKNOWN";
    }
}

static void debug_queue(MPII_cga_request_queue * queue)
{
    int j = queue->q_head;
#define REQi queue->requests[j]
    for (int i = 0; i < queue->q_len; i++) {
        if (REQi.op_stage != MPII_CGA_STAGE_NULL) {
            printf(" cga request %d: op_type=%s, op_stage=%s, block=%d, peer=%d\n", i,
                   op_type_str(REQi.op_type), op_stage_str(REQi.op_stage),
                   REQi.block, REQi.peer_rank);
        }
        j++;
        if (j == queue->q_len) {
            j = 0;
        }
    }
#undef REQi
}

#endif /* DEBUG */

/* ---- math routines ---- */

static MPI_Aint calc_chunks(MPI_Aint buf_size, MPI_Aint chunk_size, MPI_Aint * last_msg_size_out)
{
    MPI_Aint n;
    MPI_Aint last_msg_size;

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
