/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

static int get_huge(MPIR_Request * rreq);
static int get_huge_issue_read(MPIR_Request * rreq);
static int get_huge_complete(MPIR_Request * rreq);

static int get_huge(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(rreq, huge.remote_info);

    MPI_Aint cur_offset;
    if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
    } else {
        cur_offset = MPIDI_OFI_global.max_msg_size;
    }

    MPI_Aint data_sz = MPIDI_OFI_REQUEST(rreq, util.iov.iov_len);

    if (data_sz < info->msgsize) {
        rreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        info->msgsize = data_sz;
    }

    if (info->msgsize <= cur_offset) {
        /* huge message sent to small recv buffer */
        mpi_errno = get_huge_complete(rreq);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    get_huge_issue_read(rreq);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static uintptr_t recv_rbase(MPIDI_OFI_huge_remote_info_t * remote_info)
{
    if (!MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        return 0;
    } else {
        return (uintptr_t) remote_info->send_buf;
    }
}

static int get_huge_issue_read(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(rreq, huge.remote_info);
    MPIR_Comm *comm = rreq->comm;
    MPIR_FUNC_ENTER;

    MPI_Aint cur_offset, bytesLeft;
    if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
    } else {
        cur_offset = MPIDI_OFI_global.max_msg_size;
    }
    bytesLeft = info->msgsize - cur_offset;

    void *recv_buf = MPIDI_OFI_REQUEST(rreq, util.iov.iov_base);

    MPI_Aint chunk_size;
    if (MPIDI_OFI_COMM(comm).enable_striping) {
        chunk_size = (info->msgsize - MPIDI_OFI_STRIPE_CHUNK_SIZE) / MPIDI_OFI_global.num_nics;
        chunk_size = MPL_MIN(chunk_size, MPIDI_OFI_global.max_msg_size);
    } else {
        chunk_size = MPIDI_OFI_global.max_msg_size;
    }

    int num_chunks = MPL_DIV_ROUNDUP(bytesLeft, chunk_size);

    /* note: this is receiver read from sender */
    int vni_remote = info->vni_src;
    int vni_local = info->vni_dst;

    /* We'll issue multiple fi_read for every chunks. All the chunks will be tracked by a
     * chunks_outstanding counter. */
    /* NOTE: there is a possibility completion happens in between issuing fi_read (due to
     * MPIDI_OFI_CALL_RETRY). Thus we need initialize chunks_outstanding before issuing any
     * chunk */
    /* allocate and initialize cc_ptr. It will be freed by event completion when it reaches 0 */
    MPIR_cc_t *cc_ptr;
    cc_ptr = MPL_malloc(sizeof(MPIR_cc_t), MPL_MEM_OTHER);
    MPIR_cc_set(cc_ptr, num_chunks);

    int issued_chunks = 0;

    int nic = 0;
    while (bytesLeft > 0) {
        int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_local, nic);
        fi_addr_t addr =
            MPIDI_OFI_comm_to_phys(comm, info->origin_rank, nic, vni_local, vni_remote);
        uint64_t remote_key = info->rma_keys[nic];

        MPI_Aint bytesToGet = MPL_MIN(chunk_size, bytesLeft);

        MPIDI_OFI_read_chunk_t *chunk = MPL_malloc(sizeof(MPIDI_OFI_read_chunk_t), MPL_MEM_OTHER);
        chunk->event_id = MPIDI_OFI_EVENT_HUGE_CHUNK_DONE;
        chunk->localreq = rreq;
        chunk->chunks_outstanding = cc_ptr;

        MPIDI_OFI_cntr_incr(comm, vni_local, nic);
        MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[ctx_idx].tx,
                                     (void *) ((char *) recv_buf + cur_offset),
                                     bytesToGet, NULL, addr, recv_rbase(info) + cur_offset,
                                     remote_key, (void *) &chunk->context),
                             vni_local, rdma_readfrom, FALSE);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[nic], bytesToGet);
        if (MPIDI_OFI_COMM(comm).enable_striping) {
            MPIR_T_PVAR_COUNTER_INC(MULTINIC, striped_nic_recvd_bytes_count[nic], bytesToGet);
            /* round-robin to next nic */
            nic = (nic + 1) % MPIDI_OFI_global.num_nics;
        }

        issued_chunks++;
        cur_offset += bytesToGet;
        bytesLeft -= bytesToGet;
    }

    MPIR_Assert(issued_chunks == num_chunks);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_huge_complete(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(rreq, huge.remote_info);

    /* note: it's receiver ack sender */
    int vni_remote = info->vni_src;
    int vni_local = info->vni_dst;

    /* important: save comm_ptr because MPIDI_OFI_recv_event may free the request. */
    MPIR_Comm *comm_ptr = rreq->comm;

    struct fi_cq_tagged_entry wc;
    wc.len = info->msgsize;
    wc.data = info->origin_rank;
    wc.tag = info->tag;
    MPIDI_OFI_recv_event(vni_local, &wc, rreq, MPIDI_OFI_EVENT_GET_HUGE);

    MPIDI_OFI_send_control_t ctrl;
    ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
    ctrl.u.huge_ack.ackreq = info->ackreq;
    mpi_errno = MPIDI_NM_am_send_hdr(info->origin_rank, comm_ptr,
                                     MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                     &ctrl, sizeof(ctrl), vni_local, vni_remote);
    MPIR_ERR_CHECK(mpi_errno);

    MPL_free(info);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* this function called by recv event of a huge message */
int MPIDI_OFI_recv_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Comm *comm_ptr;
    MPIR_FUNC_ENTER;

    bool ready_to_get = false;
    if (MPIDI_OFI_REQUEST(rreq, event_id) != MPIDI_OFI_EVENT_RECV_HUGE) {
        /* huge send recved by a small buffer */
    } else if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        MPIR_Assert(wc->len == MPIDI_OFI_STRIPE_CHUNK_SIZE);
    } else {
        MPIR_Assert(wc->len == MPIDI_OFI_global.max_msg_size);
    }

    comm_ptr = rreq->comm;
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[MPIDI_OFI_REQUEST(rreq, nic_num)],
                            wc->len);
    if (MPIDI_OFI_REQUEST(rreq, huge.remote_info)) {
        /* this is mrecv, we already got remote info */
        ready_to_get = true;
    } else {
        /* Check for remote control info */
        MPIDI_OFI_huge_recv_list_t *list_ptr;
        MPIR_Context_id_t comm_id = comm_ptr->recvcontext_id;
        int rank = MPIDI_OFI_cqe_get_source(wc, false);
        int tag = (MPIDI_OFI_TAG_MASK & wc->tag);

        LL_FOREACH(MPIDI_OFI_global.per_vni[vni].huge_ctrl_head, list_ptr) {
            if (list_ptr->comm_id == comm_id && list_ptr->rank == rank && list_ptr->tag == tag) {
                MPIDI_OFI_REQUEST(rreq, huge.remote_info) = list_ptr->u.info;
                LL_DELETE(MPIDI_OFI_global.per_vni[vni].huge_ctrl_head,
                          MPIDI_OFI_global.per_vni[vni].huge_ctrl_tail, list_ptr);
                MPL_free(list_ptr);
                ready_to_get = true;
                break;
            }
        }
    }

    if (!ready_to_get) {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        list_ptr = (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*list_ptr), 1, MPL_MEM_BUFFER);
        if (!list_ptr)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        list_ptr->comm_id = comm_ptr->recvcontext_id;
        list_ptr->rank = MPIDI_OFI_cqe_get_source(wc, false);
        list_ptr->tag = (MPIDI_OFI_TAG_MASK & wc->tag);
        list_ptr->u.rreq = rreq;

        LL_APPEND(MPIDI_OFI_global.per_vni[vni].huge_recv_head,
                  MPIDI_OFI_global.per_vni[vni].huge_recv_tail, list_ptr);
        /* control handler will finish the recv */
    } else {
        /* proceed to get the huge message */
        mpi_errno = get_huge(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function is called when we receive a huge control message */
int MPIDI_OFI_recv_huge_control(int vni, MPIR_Context_id_t comm_id, int rank, int tag,
                                MPIDI_OFI_huge_remote_info_t * info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_huge_recv_list_t *list_ptr;
    MPIR_Request *rreq = NULL;
    MPIDI_OFI_huge_remote_info_t *info;

    /* need persist the info. It will eventually get freed at recv completion */
    info = MPL_malloc(sizeof(MPIDI_OFI_huge_remote_info_t), MPL_MEM_OTHER);
    MPIR_Assert(info);
    memcpy(info, info_ptr, sizeof(*info));

    /* If there has been a posted receive, search through the list of unmatched
     * receives to find the one that goes with the incoming message. */
    LL_FOREACH(MPIDI_OFI_global.per_vni[vni].huge_recv_head, list_ptr) {
        if (list_ptr->comm_id == comm_id && list_ptr->rank == rank && list_ptr->tag == tag) {
            rreq = list_ptr->u.rreq;
            LL_DELETE(MPIDI_OFI_global.per_vni[vni].huge_recv_head,
                      MPIDI_OFI_global.per_vni[vni].huge_recv_tail, list_ptr);
            MPL_free(list_ptr);
            break;
        }
    }

    if (!rreq) {
        list_ptr = (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(MPIDI_OFI_huge_recv_list_t),
                                                             1, MPL_MEM_OTHER);
        if (!list_ptr) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        list_ptr->comm_id = comm_id;
        list_ptr->rank = rank;
        list_ptr->tag = tag;
        list_ptr->u.info = info;

        LL_APPEND(MPIDI_OFI_global.per_vni[vni].huge_ctrl_head,
                  MPIDI_OFI_global.per_vni[vni].huge_ctrl_tail, list_ptr);
        /* let MPIDI_OFI_recv_huge_event finish the recv */
    } else if (MPIDI_OFI_REQUEST(rreq, kind) == MPIDI_OFI_req_kind__mprobe) {
        /* attach info and finish the mprobe */
        MPIDI_OFI_REQUEST(rreq, huge.remote_info) = info;
        MPIR_STATUS_SET_COUNT(rreq->status, info->msgsize);
        MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_FOUND);
    } else {
        /* attach info and finish recv */
        MPIDI_OFI_REQUEST(rreq, huge.remote_info) = info;
        mpi_errno = get_huge(rreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_peek_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPI_Aint count = 0;
    MPIDI_OFI_huge_recv_list_t *list_ptr;
    bool found_msg = false;

    /* If this is a huge message, find the control message on the unexpected list that matches
     * with this and return the size in that. */
    LL_FOREACH(MPIDI_OFI_global.per_vni[vni].huge_ctrl_head, list_ptr) {
        /* FIXME: fix the type of comm_id */
        MPIR_Context_id_t comm_id = rreq->comm->recvcontext_id;
        int rank = MPIDI_OFI_cqe_get_source(wc, false);
        int tag = (int) (MPIDI_OFI_TAG_MASK & wc->tag);
        if (list_ptr->comm_id == comm_id && list_ptr->rank == rank && list_ptr->tag == tag) {
            count = list_ptr->u.info->msgsize;
            found_msg = true;
            break;
        }
    }
    if (found_msg) {
        if (MPIDI_OFI_REQUEST(rreq, kind) == MPIDI_OFI_req_kind__mprobe) {
            MPIDI_OFI_REQUEST(rreq, huge.remote_info) = list_ptr->u.info;
            LL_DELETE(MPIDI_OFI_global.per_vni[vni].huge_ctrl_head,
                      MPIDI_OFI_global.per_vni[vni].huge_ctrl_tail, list_ptr);
            MPL_free(list_ptr);
        }
        rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
        rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(rreq->status, count);
        /* util_id should be the last thing to change in rreq. Reason is
         * we use util_id to indicate peek_event has completed and all the
         * relevant values have been copied to rreq. */
        MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_FOUND);
    } else if (MPIDI_OFI_REQUEST(rreq, kind) == MPIDI_OFI_req_kind__probe) {
        /* return not found for this probe. User can probe again. */
        MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_NOT_FOUND);
    } else if (MPIDI_OFI_REQUEST(rreq, kind) == MPIDI_OFI_req_kind__mprobe) {
        /* fill the status with wc info. Count is still missing */
        rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
        rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
        rreq->status.MPI_ERROR = MPI_SUCCESS;

        /* post the rreq to list and let control handler handle it */
        MPIDI_OFI_huge_recv_list_t *huge_list_ptr;

        huge_list_ptr =
            (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*huge_list_ptr), 1, MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(huge_list_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");

        huge_list_ptr->comm_id = rreq->comm->recvcontext_id;
        huge_list_ptr->rank = MPIDI_OFI_cqe_get_source(wc, false);
        huge_list_ptr->tag = MPIDI_OFI_TAG_MASK & wc->tag;
        huge_list_ptr->u.rreq = rreq;

        LL_APPEND(MPIDI_OFI_global.per_vni[vni].huge_recv_head,
                  MPIDI_OFI_global.per_vni[vni].huge_recv_tail, huge_list_ptr);
    }


  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_huge_chunk_done_event(int vni, struct fi_cq_tagged_entry *wc, void *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_read_chunk_t *chunk_req = (MPIDI_OFI_read_chunk_t *) req;

    int c;
    MPIR_cc_decr(chunk_req->chunks_outstanding, &c);

    if (c == 0) {
        MPL_free(chunk_req->chunks_outstanding);
        mpi_errno = get_huge_complete(chunk_req->localreq);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPL_free(chunk_req);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
