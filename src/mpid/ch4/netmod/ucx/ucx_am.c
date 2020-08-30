/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ucx_impl.h"

/* circular FIFO queue functions */

#define AM_stack MPIDI_UCX_global.am_ready_queue
#define AM_head  MPIDI_UCX_global.am_ready_head
#define AM_tail  MPIDI_UCX_global.am_ready_tail

MPL_STATIC_INLINE_PREFIX bool am_queue_is_empty(void)
{
    return (AM_head == AM_tail);
}

MPL_STATIC_INLINE_PREFIX void am_queue_push(int idx)
{
    AM_stack[AM_tail] = idx;
    AM_tail = (AM_tail + 1) & MPIDI_UCX_AM_QUEUE_MASK;
}

MPL_STATIC_INLINE_PREFIX int am_queue_pop(void)
{
    int idx = AM_stack[AM_head];
    AM_head = (AM_head + 1) & MPIDI_UCX_AM_QUEUE_MASK;
    return idx;
}

/* declar static functions */

static void am_handle_recv(int idx);
static int am_post_recv(int idx);
static int am_cancel_recv(int idx);
static void am_recv_cb(void *request, ucs_status_t status, ucp_tag_recv_info_t * info);
static void am_recv_payload(int source, void *p_data, size_t data_sz, int payload_seq);

/* exposed functions */

int MPIDI_UCX_am_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* Make sure the circular queue will work */
    MPIR_Assert(MPIDI_UCX_AM_QUEUE_SIZE > MPIDI_UCX_AM_BUF_COUNT);

    /* MPIDI_UCX_am_header_t need observe alignment */
    MPIR_Assert((sizeof(MPIDI_UCX_am_header_t) & 0x7) == 0);

    for (int i = 0; i < MPIDI_UCX_AM_BUF_COUNT; i++) {
        mpi_errno = am_post_recv(i);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_fail:
    return mpi_errno;
}

int MPIDI_UCX_am_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    for (int i = 0; i < MPIDI_UCX_AM_BUF_COUNT; i++) {
        mpi_errno = am_cancel_recv(i);
        MPIR_ERR_CHECK(mpi_errno);
    }
  fn_fail:
    return mpi_errno;
}

void MPIDI_UCX_am_progress(int vni)
{
    if (vni != 0) {
        return;
    }

    while (!am_queue_is_empty()) {
        /* NOTE: additional entries may be added during am_handle_recv */
        am_handle_recv(am_queue_pop());
    }
}

/* static functions */

static void am_handle_recv(int idx)
{
    MPIDI_UCX_am_header_t *msg_hdr = (MPIDI_UCX_am_header_t *) MPIDI_UCX_global.am_bufs[idx];

    void *p_header;
    void *p_data;
    MPI_Aint data_sz = msg_hdr->data_sz;
    MPI_Aint am_hdr_sz = msg_hdr->am_hdr_sz;

    void *temp_buf = NULL;
    if (msg_hdr->pack_type == MPIDI_UCX_AM_PACK_ONE) {
        /* got entire message */
        p_header = msg_hdr + 1;
        p_data = (char *) msg_hdr + sizeof(*msg_hdr) + am_hdr_sz;
    } else if (msg_hdr->pack_type == MPIDI_UCX_AM_PACK_TWO_A) {
        /* got msg_hdr and am_hdr */
        temp_buf = MPL_malloc(data_sz, MPL_MEM_BUFFER);
        MPIR_Assert(temp_buf != NULL);
        am_recv_payload(idx, temp_buf, data_sz, msg_hdr->payload_seq);

        p_header = msg_hdr + 1;
        p_data = temp_buf;
    } else if (msg_hdr->pack_type == MPIDI_UCX_AM_PACK_TWO_B) {
        /* only got msg_hdr */
        temp_buf = MPL_malloc(am_hdr_sz + data_sz, MPL_MEM_BUFFER);
        MPIR_Assert(temp_buf != NULL);
        am_recv_payload(idx, temp_buf, am_hdr_sz + data_sz, msg_hdr->payload_seq);

        p_header = temp_buf;
        p_data = (char *) temp_buf + am_hdr_sz;
    } else {
        /* bad pack_type */
        MPIR_Assert(0);
    }

    /* note: setting is_local, is_async to 0, 0 */
    int handler_id = msg_hdr->handler_id;
    MPIDIG_global.target_msg_cbs[handler_id] (handler_id, p_header, p_data, data_sz, 0, 0, NULL);

    if (temp_buf) {
        MPL_free(temp_buf);
    }

    MPIDI_UCX_ucp_request_free(MPIDI_UCX_global.am_ucp_reqs[idx]);
    int mpi_errno = am_post_recv(idx);
    MPIR_Assert(mpi_errno == MPI_SUCCESS);
}

static void am_recv_payload(int idx, void *p_data, size_t data_sz, int payload_seq)
{
    ucs_status_ptr_t ret;
    int source = MPIDI_UCX_global.am_ucp_reqs[idx]->am.source;
    uint64_t tag = MPIDI_UCX_am_init_data_tag(source, payload_seq);
    ret = ucp_tag_recv_nb(MPIDI_UCX_global.ctx[0].worker,
                          p_data, data_sz, ucp_dt_make_contig(1),
                          tag, MPIDI_UCX_AM_DATA_MASK, MPIDI_UCX_dummy_recv_cb);
    MPIR_Assert(!UCS_PTR_IS_ERR(ret));
    while (!ucp_request_is_completed(ret)) {
        ucp_worker_progress(MPIDI_UCX_global.ctx[0].worker);
    }
    MPIDI_UCX_ucp_request_free(ret);
}

static void am_recv_cb(void *request, ucs_status_t status, ucp_tag_recv_info_t * info)
{
    if (status == UCS_ERR_CANCELED) {
        return;
    }

    MPIR_Assert(!UCS_PTR_IS_ERR(request));

    MPIDI_UCX_ucp_request_t *ucp_req = request;
    ucp_req->am.source = MPIDI_UCX_get_source(info->sender_tag);
    if (!ucp_req->am.is_set) {
        /* idx not set yet, simply flag it and let caller handle it */
        ucp_req->am.is_set = 1;
    } else {
        am_queue_push(ucp_req->am.idx);
    }
}

static int am_post_recv(int idx)
{
    int mpi_errno = MPI_SUCCESS;
    ucs_status_ptr_t ret;
    ret = ucp_tag_recv_nb(MPIDI_UCX_global.ctx[0].worker,
                          MPIDI_UCX_global.am_bufs[idx],
                          MPIDI_UCX_AM_BUF_SIZE, ucp_dt_make_contig(1),
                          MPIDI_UCX_AM_HDR_TAG, MPIDI_UCX_AM_HDR_MASK, am_recv_cb);
    MPIDI_UCX_CHK_REQUEST(ret);

    MPIDI_UCX_global.am_ucp_reqs[idx] = ret;

    MPIDI_UCX_ucp_request_t *ucp_req = (void *) ret;
    ucp_req->am.idx = idx;
    if (ucp_req->am.is_set) {
        /* message already arrived */
        am_queue_push(idx);
    } else {
        ucp_req->am.is_set = 1;
    }

  fn_fail:
    return mpi_errno;
}

static int am_cancel_recv(int idx)
{
    int mpi_errno = MPI_SUCCESS;
    ucp_request_cancel(MPIDI_UCX_global.ctx[0].worker, MPIDI_UCX_global.am_ucp_reqs[idx]);
    MPIDI_UCX_ucp_request_free(MPIDI_UCX_global.am_ucp_reqs[idx]);

    return mpi_errno;
}
