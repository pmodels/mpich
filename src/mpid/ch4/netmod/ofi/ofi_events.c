/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_am_events.h"
#include "ofi_events.h"

/* We can use a generic length fi_info.max_err_data returned by fi_getinfo()
 * However, currently we do not use the error data, we set the length to a
 * value that is generally common across the different providers. */
#define MPIDI_OFI_MAX_ERR_DATA_SIZE 64

static int peek_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int peek_empty_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int recv_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int send_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int ssend_ack_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static uintptr_t recv_rbase(MPIDI_OFI_huge_recv_t * recv);
static int chunk_done_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int inject_emu_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int accept_probe_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int dynproc_done_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_isend_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int am_isend_rdma_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int am_isend_pipeline_event(int vni, struct fi_cq_tagged_entry *wc,
                                   MPIR_Request * dont_use_me);
static int am_recv_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_read_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me);

static int peek_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t count = 0;
    MPIR_FUNC_ENTER;
    rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    rreq->status.MPI_ERROR = MPI_SUCCESS;

    if (MPIDI_OFI_HUGE_SEND & wc->tag) {
        MPIDI_OFI_huge_recv_t *list_ptr;
        bool found_msg = false;

        /* If this is a huge message, find the control message on the unexpected list that matches
         * with this and return the size in that. */
        LL_FOREACH(MPIDI_unexp_huge_recv_head, list_ptr) {
            uint64_t context_id = MPIDI_OFI_CONTEXT_MASK & wc->tag;
            uint64_t tag = MPIDI_OFI_TAG_MASK & wc->tag;
            if (list_ptr->remote_info.comm_id == context_id &&
                list_ptr->remote_info.origin_rank == MPIDI_OFI_cqe_get_source(wc, false) &&
                list_ptr->remote_info.tag == tag) {
                count = list_ptr->remote_info.msgsize;
                found_msg = true;
            }
        }
        if (!found_msg) {
            MPIDI_OFI_huge_recv_t *recv_elem;
            MPIDI_OFI_huge_recv_list_t *huge_list_ptr;

            /* Create an element in the posted list that only indicates a peek and will be
             * deleted as soon as it's fulfilled without being matched. */
            recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_COMM);
            MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
            recv_elem->peek = true;
            MPIR_Comm *comm_ptr = rreq->comm;
            recv_elem->comm_ptr = comm_ptr;
            MPIDIU_map_set(MPIDI_OFI_global.huge_recv_counters, rreq->handle, recv_elem,
                           MPL_MEM_BUFFER);

            huge_list_ptr =
                (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*huge_list_ptr), 1, MPL_MEM_COMM);
            MPIR_ERR_CHKANDJUMP(huge_list_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
            recv_elem->remote_info.comm_id = huge_list_ptr->comm_id =
                MPIDI_OFI_CONTEXT_MASK & wc->tag;
            recv_elem->remote_info.origin_rank = huge_list_ptr->rank =
                MPIDI_OFI_cqe_get_source(wc, false);
            recv_elem->remote_info.tag = huge_list_ptr->tag = MPIDI_OFI_TAG_MASK & wc->tag;
            recv_elem->localreq = huge_list_ptr->rreq = rreq;
            recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
            recv_elem->done_fn = MPIDI_OFI_recv_event;
            recv_elem->wc = *wc;
            if (MPIDI_OFI_COMM(comm_ptr).enable_striping) {
                recv_elem->cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
            } else {
                recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;
            }

            LL_APPEND(MPIDI_posted_huge_recv_head, MPIDI_posted_huge_recv_tail, huge_list_ptr);
            goto fn_exit;
        }
    } else {
        /* Otherwise just get the size of the message we've already received. */
        count = wc->len;
    }
    MPIR_STATUS_SET_COUNT(rreq->status, count);
    /* util_id should be the last thing to change in rreq. Reason is
     * we use util_id to indicate peek_event has completed and all the
     * relevant values have been copied to rreq. */
    MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_FOUND);
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int peek_empty_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl;

    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
        case MPIDI_OFI_EVENT_PEEK:
            rreq->status.MPI_ERROR = MPI_SUCCESS;
            /* util_id should be the last thing to change in rreq. Reason is
             * we use util_id to indicate peek_event has completed and all the
             * relevant values have been copied to rreq. */
            MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)),
                                         MPIDI_OFI_PEEK_NOT_FOUND);
            break;

        case MPIDI_OFI_EVENT_ACCEPT_PROBE:
            ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
            ctrl->done = MPIDI_OFI_PEEK_NOT_FOUND;
            break;

        default:
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

/* If we posted a huge receive, this event gets called to translate the
 * completion queue entry into a get huge event */
static int recv_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;
    MPIR_Comm *comm_ptr;
    MPIR_FUNC_ENTER;

    bool ready_to_get = false;
    /* Check that the sender didn't underflow the message by sending less than
     * the huge message threshold. When striping is enabled underflow occurs if
     * the sender sends < MPIDI_OFI_STRIPE_CHUNK_SIZE through the huge message protocol
     * or < MPIDI_OFI_global.stripe_threshold through normal send */
    if (((wc->len < MPIDI_OFI_STRIPE_CHUNK_SIZE ||
          (wc->len > MPIDI_OFI_STRIPE_CHUNK_SIZE && wc->len < MPIDI_OFI_global.stripe_threshold)) &&
         MPIDI_OFI_COMM(rreq->comm).enable_striping) ||
        (wc->len < MPIDI_OFI_global.max_msg_size && !MPIDI_OFI_COMM(rreq->comm).enable_striping)) {
        return MPIDI_OFI_recv_event(vni, wc, rreq, MPIDI_OFI_REQUEST(rreq, event_id));
    }

    comm_ptr = rreq->comm;
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[MPIDI_OFI_REQUEST(rreq, nic_num)],
                            wc->len);
    /* Check to see if the tracker is already in the unexpected list.
     * Otherwise, allocate one. */
    {
        MPIDI_OFI_huge_recv_t *list_ptr;

        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "SEARCHING HUGE UNEXPECTED LIST: (%d, %d, %llu)",
                         comm_ptr->context_id, MPIDI_OFI_cqe_get_source(wc, false),
                         (MPIDI_OFI_TAG_MASK & wc->tag)));

        LL_FOREACH(MPIDI_unexp_huge_recv_head, list_ptr) {
            if (list_ptr->remote_info.comm_id == comm_ptr->context_id &&
                list_ptr->remote_info.origin_rank == MPIDI_OFI_cqe_get_source(wc, false) &&
                list_ptr->remote_info.tag == (MPIDI_OFI_TAG_MASK & wc->tag)) {
                MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                                (MPL_DBG_FDEST, "MATCHED HUGE UNEXPECTED LIST: (%d, %d, %llu, %d)",
                                 comm_ptr->context_id, MPIDI_OFI_cqe_get_source(wc, false),
                                 (MPIDI_OFI_TAG_MASK & wc->tag), rreq->handle));

                LL_DELETE(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, list_ptr);

                recv_elem = list_ptr;
                MPIDIU_map_set(MPIDI_OFI_global.huge_recv_counters, rreq->handle, recv_elem,
                               MPL_MEM_COMM);
                break;
            }
        }
    }

    if (recv_elem) {
        ready_to_get = true;
    } else {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "CREATING HUGE POSTED ENTRY: (%d, %d, %llu)",
                         comm_ptr->context_id, MPIDI_OFI_cqe_get_source(wc, false),
                         (MPIDI_OFI_TAG_MASK & wc->tag)));

        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDIU_map_set(MPIDI_OFI_global.huge_recv_counters, rreq->handle, recv_elem,
                       MPL_MEM_BUFFER);

        list_ptr = (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*list_ptr), 1, MPL_MEM_BUFFER);
        if (!list_ptr)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        list_ptr->comm_id = comm_ptr->context_id;
        list_ptr->rank = MPIDI_OFI_cqe_get_source(wc, false);
        list_ptr->tag = (MPIDI_OFI_TAG_MASK & wc->tag);
        list_ptr->rreq = rreq;

        LL_APPEND(MPIDI_posted_huge_recv_head, MPIDI_posted_huge_recv_tail, list_ptr);
    }

    /* Plug the information for the huge event into the receive request and go
     * to the MPIDI_OFI_get_huge_event function. */
    recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv_elem->peek = false;
    recv_elem->comm_ptr = comm_ptr;
    recv_elem->localreq = rreq;
    recv_elem->done_fn = MPIDI_OFI_recv_event;
    recv_elem->wc = *wc;
    if (MPIDI_OFI_COMM(comm_ptr).enable_striping) {
        recv_elem->cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
    } else {
        recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;
    }
    if (ready_to_get) {
        MPIDI_OFI_get_huge_event(vni, NULL, (MPIR_Request *) recv_elem);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int send_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c, num_nics;
    MPIR_FUNC_ENTER;

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        MPIR_Comm *comm;
        void *ptr;
        struct fid_mr **huge_send_mrs;

        comm = sreq->comm;
        num_nics = MPIDI_OFI_COMM(comm).enable_striping ? MPIDI_OFI_global.num_nics : 1;
        /* Look for the memory region using the sreq handle */
        ptr = MPIDIU_map_lookup(MPIDI_OFI_global.huge_send_counters, sreq->handle);
        MPIR_Assert(ptr != MPIDIU_MAP_NOT_FOUND);

        huge_send_mrs = (struct fid_mr **) ptr;

        /* Send a cleanup message to the receivier and clean up local
         * resources. */
        /* Clean up the local counter */
        MPIDIU_map_erase(MPIDI_OFI_global.huge_send_counters, sreq->handle);

        /* Clean up the memory region */
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            for (int i = 0; i < num_nics; i++) {
                uint64_t key = fi_mr_key(huge_send_mrs[i]);
                MPIDI_OFI_mr_key_free(MPIDI_OFI_LOCAL_MR_KEY, key);
            }
        }

        for (int i = 0; i < num_nics; i++) {
            MPIDI_OFI_CALL(fi_close(&huge_send_mrs[i]->fid), mr_unreg);
        }
        MPL_free(huge_send_mrs);

        if (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer)) {
            MPIR_gpu_free_host(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
        }

        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIDI_CH4_REQUEST_FREE(sreq);
    }
    /* c != 0, ssend */
  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ssend_ack_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno;
    MPIDI_OFI_ssendack_request_t *req = (MPIDI_OFI_ssendack_request_t *) sreq;
    MPIR_FUNC_ENTER;
    mpi_errno =
        MPIDI_OFI_send_event(vni, NULL, req->signal_req,
                             MPIDI_OFI_REQUEST(req->signal_req, event_id));

    MPL_free(req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static uintptr_t recv_rbase(MPIDI_OFI_huge_recv_t * recv_elem)
{
    if (!MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        return 0;
    } else {
        return recv_elem->remote_info.send_buf;
    }
}

/* Note: MPIDI_OFI_get_huge_event is invoked from three places --
 * 1. In recv_huge_event, when recv buffer is matched and first chunk received, and
 *    when control message (with remote info) has also been received.
 * 2. In MPIDI_OFI_get_huge, as a callback when control message is received, and
 *    when first chunk has been matched and received.
 *
 * recv_huge_event will fill the local request information, and the control message
 * callback will fill the remote (sender) information. Lastly --
 *
 * 3. As the event function when RDMA read (issued here) completes.
 */
int MPIDI_OFI_get_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv_elem = (MPIDI_OFI_huge_recv_t *) req;
    uint64_t remote_key;
    size_t bytesLeft, bytesToGet;
    MPIR_FUNC_ENTER;

    void *recv_buf = MPIDI_OFI_REQUEST(recv_elem->localreq, util.iov.iov_base);

    if (MPIDI_OFI_COMM(recv_elem->comm_ptr).enable_striping) {
        /* Subtract one stripe_chunk_size because we send the first chunk via a regular message
         * instead of the memory region */
        recv_elem->stripe_size = (recv_elem->remote_info.msgsize - MPIDI_OFI_STRIPE_CHUNK_SIZE)
            / MPIDI_OFI_global.num_nics;        /* striping */

        if (recv_elem->stripe_size > MPIDI_OFI_global.max_msg_size) {
            recv_elem->stripe_size = MPIDI_OFI_global.max_msg_size;
        }
        if (recv_elem->chunks_outstanding)
            recv_elem->chunks_outstanding--;
        bytesLeft = recv_elem->remote_info.msgsize - recv_elem->cur_offset;
        bytesToGet = (bytesLeft <= recv_elem->stripe_size) ? bytesLeft : recv_elem->stripe_size;
    } else {
        /* Subtract one max_msg_size because we send the first chunk via a regular message
         * instead of the memory region */
        bytesLeft = recv_elem->remote_info.msgsize - recv_elem->cur_offset;
        bytesToGet = (bytesLeft <= MPIDI_OFI_global.max_msg_size) ?
            bytesLeft : MPIDI_OFI_global.max_msg_size;
    }
    if (bytesToGet == 0ULL && recv_elem->chunks_outstanding == 0) {
        MPIDI_OFI_send_control_t ctrl;
        /* recv_elem->localreq may be freed during done_fn.
         * Need to backup the handle here for later use with MPIDIU_map_erase. */
        uint64_t key_to_erase = recv_elem->localreq->handle;
        recv_elem->wc.len = recv_elem->cur_offset;
        recv_elem->done_fn(vni, &recv_elem->wc, recv_elem->localreq, recv_elem->event_id);
        ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
        mpi_errno =
            MPIDI_OFI_do_control_send(&ctrl, NULL, 0, recv_elem->remote_info.origin_rank,
                                      recv_elem->comm_ptr, recv_elem->remote_info.ackreq);
        MPIR_ERR_CHECK(mpi_errno);

        MPIDIU_map_erase(MPIDI_OFI_global.huge_recv_counters, key_to_erase);
        MPL_free(recv_elem);

        goto fn_exit;
    }

    int nic = 0;
    int vni_src = recv_elem->remote_info.vni_src;
    int vni_dst = recv_elem->remote_info.vni_dst;
    if (MPIDI_OFI_COMM(recv_elem->comm_ptr).enable_striping) {  /* if striping enabled */
        MPIDI_OFI_cntr_incr(recv_elem->comm_ptr, vni_src, nic);
        if (recv_elem->cur_offset >= MPIDI_OFI_STRIPE_CHUNK_SIZE && bytesLeft > 0) {
            for (nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
                int ctx_idx = MPIDI_OFI_get_ctx_index(recv_elem->comm_ptr, vni_dst, nic);
                remote_key = recv_elem->remote_info.rma_keys[nic];

                bytesLeft = recv_elem->remote_info.msgsize - recv_elem->cur_offset;
                if (bytesLeft <= 0) {
                    break;
                }
                bytesToGet =
                    (bytesLeft <= recv_elem->stripe_size) ? bytesLeft : recv_elem->stripe_size;

                MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[ctx_idx].tx, (void *) ((char *) recv_buf + recv_elem->cur_offset),    /* local buffer */
                                             bytesToGet,        /* bytes */
                                             NULL,      /* descriptor */
                                             MPIDI_OFI_comm_to_phys(recv_elem->comm_ptr, recv_elem->remote_info.origin_rank, nic, vni_dst, vni_src), recv_rbase(recv_elem) + recv_elem->cur_offset,     /* remote maddr */
                                             remote_key,        /* Key */
                                             (void *) &recv_elem->context), nic,        /* Context */
                                     rdma_readfrom, FALSE);
                MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[nic], bytesToGet);
                MPIR_T_PVAR_COUNTER_INC(MULTINIC, striped_nic_recvd_bytes_count[nic], bytesToGet);
                recv_elem->cur_offset += bytesToGet;
                recv_elem->chunks_outstanding++;
            }
        }
    } else {
        int ctx_idx = MPIDI_OFI_get_ctx_index(recv_elem->comm_ptr, vni_src, nic);
        remote_key = recv_elem->remote_info.rma_keys[nic];
        MPIDI_OFI_cntr_incr(recv_elem->comm_ptr, vni_src, nic);
        MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[ctx_idx].tx,  /* endpoint     */
                                     (void *) ((char *) recv_buf + recv_elem->cur_offset),      /* local buffer */
                                     bytesToGet,        /* bytes        */
                                     NULL,      /* descriptor   */
                                     MPIDI_OFI_comm_to_phys(recv_elem->comm_ptr, recv_elem->remote_info.origin_rank, nic, vni_src, vni_dst),    /* Destination  */
                                     recv_rbase(recv_elem) + recv_elem->cur_offset,     /* remote maddr */
                                     remote_key,        /* Key          */
                                     (void *) &recv_elem->context), vni_src, rdma_readfrom,     /* Context */
                             FALSE);
        MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[nic], bytesToGet);
        recv_elem->cur_offset += bytesToGet;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int chunk_done_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int c;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_chunk_request *creq = (MPIDI_OFI_chunk_request *) req;
    MPIR_cc_decr(creq->parent->cc_ptr, &c);

    if (c == 0)
        MPIDI_CH4_REQUEST_FREE(creq->parent);

    MPL_free(creq);
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int inject_emu_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int incomplete;
    MPIR_FUNC_ENTER;

    MPIR_cc_decr(req->cc_ptr, &incomplete);

    if (!incomplete) {
        MPL_free(MPIDI_OFI_REQUEST(req, util.inject_buf));
        MPIDI_CH4_REQUEST_FREE(req);
        MPL_atomic_fetch_sub_int(&MPIDI_OFI_global.am_inflight_inject_emus, 1);
    }

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int accept_probe_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->source = MPIDI_OFI_cqe_get_source(wc, false);
    ctrl->tag = MPIDI_OFI_init_get_tag(wc->tag);
    ctrl->msglen = wc->len;
    ctrl->done = MPIDI_OFI_PEEK_FOUND;
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int dynproc_done_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_ENTER;
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->done++;
    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

static int am_isend_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;

    MPIR_FUNC_ENTER;

    msg_hdr = &MPIDI_OFI_AM_SREQ_HDR(sreq, msg_hdr);
    MPID_Request_complete(sreq);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vni[vni].pack_buf_pool,
                                      MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer));
    MPIDI_OFI_AM_SREQ_HDR(sreq, pack_buffer) = NULL;
    mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_isend_rdma_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPID_Request_complete(sreq);

    /* RDMA_READ will perform origin side completion when ACK arrives */

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

static int am_isend_pipeline_event(int vni, struct fi_cq_tagged_entry *wc,
                                   MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;
    MPIDI_OFI_am_send_pipeline_request_t *ofi_req;
    MPIR_Request *sreq = NULL;

    MPIR_FUNC_ENTER;

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_send_pipeline_request_t, context);
    msg_hdr = ofi_req->msg_hdr;
    sreq = ofi_req->sreq;
    MPID_Request_complete(sreq);        /* FIXME: Should not call MPIDI in NM ? */

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vni[vni].pack_buf_pool,
                                      ofi_req->pack_buffer);

    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vni[vni].am_hdr_buf_pool, ofi_req);

    int is_done = MPIDIG_am_send_async_finish_seg(sreq);

    if (is_done) {
        mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);
    }

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_recv_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *am_hdr;
    MPIDI_OFI_am_unordered_msg_t *uo_msg = NULL;
    fi_addr_t fi_src_addr;
    uint16_t expected_seqno, next_seqno;
    MPIR_FUNC_ENTER;

    void *orig_buf = wc->buf;   /* needed in case we will copy the header for alignment fix */
    am_hdr = (MPIDI_OFI_am_header_t *) wc->buf;

#ifdef NEEDS_STRICT_ALIGNMENT
    /* FI_MULTI_RECV may pack the message at lesser alignment, copy the header
     * when that's the case */
#define MAX_HDR_SIZE 256        /* need accommodate MPIDI_AMTYPE_RDMA_READ */
    /* if has_alignment_copy is 0 and the message contains extended header, the
     * header needs to be copied out for alignment to access */
    int has_alignment_copy = 0;
    char temp[MAX_HDR_SIZE] MPL_ATTR_ALIGNED(MAX_ALIGNMENT);
    if ((intptr_t) am_hdr & (MAX_ALIGNMENT - 1)) {
        int temp_size = MAX_HDR_SIZE;
        if (temp_size > wc->len) {
            temp_size = wc->len;
        }
        memcpy(temp, orig_buf, temp_size);
        am_hdr = (void *) temp;
        /* confirm it (in case MPL_ATTR_ALIGNED didn't work) */
        MPIR_Assert(((intptr_t) am_hdr & (MAX_ALIGNMENT - 1)) == 0);
    }
#endif

    expected_seqno = MPIDI_OFI_am_get_next_recv_seqno(vni, am_hdr->fi_src_addr);
    if (am_hdr->seqno != expected_seqno) {
        /* This message came earlier than the one that we were expecting.
         * Put it in the queue to process it later. */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                        (MPL_DBG_FDEST,
                         "Expected seqno=%d but got %d (am_type=%d addr=%" PRIx64 "). "
                         "Enqueueing it to the queue.\n",
                         expected_seqno, am_hdr->seqno, am_hdr->am_type, am_hdr->fi_src_addr));
        mpi_errno = MPIDI_OFI_am_enqueue_unordered_msg(vni, orig_buf);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Received an expected message */
  fn_repeat:
    fi_src_addr = am_hdr->fi_src_addr;
    next_seqno = am_hdr->seqno + 1;

    void *p_data;
    switch (am_hdr->am_type) {
        case MPIDI_AMTYPE_SHORT_HDR:
            mpi_errno = MPIDI_OFI_handle_short_am_hdr(am_hdr, am_hdr + 1 /* payload */);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        case MPIDI_AMTYPE_SHORT:
            /* payload always in orig_buf */
            p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
            mpi_errno = MPIDI_OFI_handle_short_am(am_hdr, am_hdr + 1, p_data);

            MPIR_ERR_CHECK(mpi_errno);

            break;
        case MPIDI_AMTYPE_PIPELINE:
            p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
            mpi_errno = MPIDI_OFI_handle_pipeline(am_hdr, am_hdr + 1, p_data);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIDI_AMTYPE_RDMA_READ:
            {
                /* buffer always copied together (there is no payload, just LMT header) */
#ifdef NEEDS_STRICT_ALIGNMENT
                MPIDI_OFI_lmt_msg_payload_t temp_rdma_lmt_msg;
                if (!has_alignment_copy) {
                    memcpy(&temp_rdma_lmt_msg,
                           (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz,
                           sizeof(MPIDI_OFI_lmt_msg_payload_t));
                    p_data = (void *) &temp_rdma_lmt_msg;
                } else
#endif
                {
                    p_data = (char *) orig_buf + sizeof(*am_hdr) + am_hdr->am_hdr_sz;
                }
                mpi_errno = MPIDI_OFI_handle_rdma_read(am_hdr, am_hdr + 1,
                                                       (MPIDI_OFI_lmt_msg_payload_t *) p_data);

                MPIR_ERR_CHECK(mpi_errno);

                break;
            }
        default:
            MPIR_Assert(0);
    }

    /* For the first iteration (=in case we can process the message just received
     * from OFI immediately), uo_msg is NULL, so freeing it is no-op.
     * Otherwise, free it here before getting another uo_msg. */
    MPL_free(uo_msg);

    /* See if we can process other messages in the queue */
    if ((uo_msg = MPIDI_OFI_am_claim_unordered_msg(vni, fi_src_addr, next_seqno)) != NULL) {
        am_hdr = &uo_msg->am_hdr;
        orig_buf = am_hdr;
#ifdef NEEDS_STRICT_ALIGNMENT
        /* alignment is ensured for this unordered message as it copies to a temporary buffer
         * in MPIDI_OFI_am_enqueue_unordered_msg */
        has_alignment_copy = 1;
#endif
        goto fn_repeat;
    }

    /* Record the next expected sequence number from fi_src_addr */
    MPIDI_OFI_am_set_next_recv_seqno(vni, fi_src_addr, next_seqno);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_read_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_OFI_am_request_t *ofi_req;

    MPIR_FUNC_ENTER;

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_request_t, context);
    rreq = (MPIR_Request *) ofi_req->rreq_hdr->rreq_ptr;

    if (ofi_req->rreq_hdr->lmt_type == MPIDI_OFI_AM_LMT_IOV) {
        ofi_req->rreq_hdr->lmt_u.lmt_cntr--;
        if (ofi_req->rreq_hdr->lmt_u.lmt_cntr) {
            goto fn_exit;
        }
    } else if (ofi_req->rreq_hdr->lmt_type == MPIDI_OFI_AM_LMT_UNPACK) {
        int done = MPIDI_OFI_am_lmt_unpack_event(rreq);
        if (!done) {
            goto fn_exit;
        }
    }

    MPIR_Comm *comm;
    if (rreq->kind == MPIR_REQUEST_KIND__RMA) {
        comm = rreq->u.rma.win->comm_ptr;
    } else {
        comm = rreq->comm;
    }
    MPIDI_OFI_lmt_msg_payload_t *lmt_info = (void *) MPIDI_OFI_AM_RREQ_HDR(rreq, am_hdr_buf);
    mpi_errno = MPIDI_OFI_do_am_rdma_read_ack(lmt_info->src_rank, comm, lmt_info->sreq_ptr);

    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    MPID_Request_complete(rreq);
  fn_exit:
    MPIDU_genq_private_pool_free_cell(MPIDI_OFI_global.per_vni[vni].am_hdr_buf_pool, ofi_req);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_dispatch_function(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_SEND) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = MPIDI_OFI_send_event(vni, wc, req, MPIDI_OFI_EVENT_SEND);
        goto fn_exit;
    } else if (MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RECV) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = MPIDI_OFI_recv_event(vni, wc, req, MPIDI_OFI_EVENT_RECV);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND)) {
        mpi_errno = am_isend_event(vni, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND_RDMA)) {
        mpi_errno = am_isend_rdma_event(vni, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND_PIPELINE)) {
        mpi_errno = am_isend_pipeline_event(vni, wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_RECV)) {
        if (wc->flags & FI_RECV)
            mpi_errno = am_recv_event(vni, wc, req);

        if (unlikely(wc->flags & FI_MULTI_RECV)) {
            MPIDI_OFI_am_repost_request_t *am = (MPIDI_OFI_am_repost_request_t *) req;
            mpi_errno = MPIDI_OFI_am_repost_buffer(vni, am->index);
        }

        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_READ)) {
        mpi_errno = am_read_event(vni, wc, req);
        goto fn_exit;
    } else if (unlikely(1)) {
        switch (MPIDI_OFI_REQUEST(req, event_id)) {
            case MPIDI_OFI_EVENT_PEEK:
                mpi_errno = peek_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_RECV_HUGE:
                mpi_errno = recv_huge_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_RECV_PACK:
                mpi_errno = MPIDI_OFI_recv_event(vni, wc, req, MPIDI_OFI_EVENT_RECV_PACK);
                break;

            case MPIDI_OFI_EVENT_RECV_NOPACK:
                mpi_errno = MPIDI_OFI_recv_event(vni, wc, req, MPIDI_OFI_EVENT_RECV_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SEND_HUGE:
                mpi_errno = send_huge_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_SEND_PACK:
                mpi_errno = MPIDI_OFI_send_event(vni, wc, req, MPIDI_OFI_EVENT_SEND_PACK);
                break;

            case MPIDI_OFI_EVENT_SEND_NOPACK:
                mpi_errno = MPIDI_OFI_send_event(vni, wc, req, MPIDI_OFI_EVENT_SEND_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SSEND_ACK:
                mpi_errno = ssend_ack_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_GET_HUGE:
                mpi_errno = MPIDI_OFI_get_huge_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_CHUNK_DONE:
                mpi_errno = chunk_done_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_INJECT_EMU:
                mpi_errno = inject_emu_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_DYNPROC_DONE:
                mpi_errno = dynproc_done_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_ACCEPT_PROBE:
                mpi_errno = accept_probe_event(vni, wc, req);
                break;

            case MPIDI_OFI_EVENT_ABORT:
            default:
                mpi_errno = MPI_SUCCESS;
                MPIR_Assert(0);
                break;
        }
    }

  fn_exit:
    return mpi_errno;
}

int MPIDI_OFI_handle_cq_error(int vni, int nic, ssize_t ret)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_err_entry e;
    char err_data[MPIDI_OFI_MAX_ERR_DATA_SIZE];
    MPIR_Request *req;
    ssize_t ret_cqerr;
    MPIR_FUNC_ENTER;

    int ctx_idx = MPIDI_OFI_get_ctx_index(NULL, vni, nic);
    switch (ret) {
        case -FI_EAVAIL:
            /* Provide separate error buffer for each thread. This makes the
             * call to fi_cq_readerr threadsafe. If we don't provide the buffer,
             * OFI passes an internal buffer to the threads, which can lead to
             * the threads sharing the buffer. */
            e.err_data = err_data;
            e.err_data_size = sizeof(err_data);
            ret_cqerr = fi_cq_readerr(MPIDI_OFI_global.ctx[ctx_idx].cq, &e, 0);
            /* The error was already consumed, most likely by another thread,
             *  possible in case of lockless MT model */
            if (ret_cqerr == -FI_EAGAIN)
                break;

            switch (e.err) {
                case FI_ETRUNC:
                    req = MPIDI_OFI_context_to_request(e.op_context);

                    switch (req->kind) {
                        case MPIR_REQUEST_KIND__SEND:
                            mpi_errno = MPIDI_OFI_dispatch_function(vni, NULL, req);
                            break;

                        case MPIR_REQUEST_KIND__RECV:
                            mpi_errno =
                                MPIDI_OFI_dispatch_function(vni, (struct fi_cq_tagged_entry *) &e,
                                                            req);
                            req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                            break;

                        default:
                            MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                                      "**ofid_poll %s %d %s %s", __SHORT_FILE__,
                                                      __LINE__, __func__, fi_strerror(e.err));
                    }

                    break;

                case FI_ECANCELED:
                    req = MPIDI_OFI_context_to_request(e.op_context);
                    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(req, datatype));
                    MPIR_STATUS_SET_CANCEL_BIT(req->status, TRUE);
                    break;

                case FI_ENOMSG:
                    req = MPIDI_OFI_context_to_request(e.op_context);
                    mpi_errno = peek_empty_event(vni, NULL, req);
                    break;

                default:
                    MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                              "**ofid_poll %s %d %s %s", __SHORT_FILE__,
                                              __LINE__, __func__, fi_strerror(e.err));
                    break;
            }

            break;

        default:
            MPIR_ERR_SETFATALANDJUMP4(mpi_errno, MPI_ERR_OTHER, "**ofid_poll",
                                      "**ofid_poll %s %d %s %s", __SHORT_FILE__, __LINE__,
                                      __func__, fi_strerror(errno));
            break;
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
