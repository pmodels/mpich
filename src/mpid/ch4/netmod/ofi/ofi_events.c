/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "ofi_events.h"

static int cqe_get_source(struct fi_cq_tagged_entry *wc, bool has_err);
static int peek_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int peek_empty_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int recv_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq, int event_id);
static int recv_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int send_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int ssend_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static uintptr_t recv_rbase(MPIDI_OFI_huge_recv_t * recv);
static int chunk_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int inject_emu_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req);
static int accept_probe_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int dynproc_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_isend_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq);
static int am_recv_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);
static int am_read_event(struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me);
static int am_repost_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq);

static int cqe_get_source(struct fi_cq_tagged_entry *wc, bool has_err)
{
    if (unlikely(has_err)) {
        return wc->data & ((1 << MPIDI_OFI_IDATA_SRC_BITS) - 1);
    }
    return wc->data;
}

static int peek_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t count = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PEEK_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PEEK_EVENT);
    rreq->status.MPI_SOURCE = cqe_get_source(wc, false);
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
                list_ptr->remote_info.origin_rank == cqe_get_source(wc, false) &&
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
            MPIR_Comm *comm_ptr = MPIDIG_context_id_to_comm(MPIDI_OFI_CONTEXT_MASK & wc->tag);
            recv_elem->comm_ptr = comm_ptr;
            MPIDIU_map_set(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters, rreq->handle, recv_elem,
                           MPL_MEM_BUFFER);

            huge_list_ptr =
                (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*huge_list_ptr), 1, MPL_MEM_COMM);
            MPIR_ERR_CHKANDJUMP(huge_list_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
            recv_elem->remote_info.comm_id = huge_list_ptr->comm_id =
                MPIDI_OFI_CONTEXT_MASK & wc->tag;
            recv_elem->remote_info.origin_rank = huge_list_ptr->rank = cqe_get_source(wc, false);
            recv_elem->remote_info.tag = huge_list_ptr->tag = MPIDI_OFI_TAG_MASK & wc->tag;
            recv_elem->localreq = huge_list_ptr->rreq = rreq;
            recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
            recv_elem->done_fn = recv_event;
            recv_elem->wc = *wc;
            recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;

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
    MPIDI_OFI_REQUEST(rreq, util_id) = MPIDI_OFI_PEEK_FOUND;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PEEK_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int peek_empty_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PEEK_EMPTY_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PEEK_EMPTY_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl;

    switch (MPIDI_OFI_REQUEST(rreq, event_id)) {
        case MPIDI_OFI_EVENT_PEEK:
            MPIDI_OFI_REQUEST(rreq, util_id) = MPIDI_OFI_PEEK_NOT_FOUND;
            rreq->status.MPI_ERROR = MPI_SUCCESS;
            break;

        case MPIDI_OFI_EVENT_ACCEPT_PROBE:
            ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
            ctrl->done = MPIDI_OFI_PEEK_NOT_FOUND;
            break;

        default:
            MPIR_Assert(0);
            break;
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PEEK_EMPTY_EVENT);
    return MPI_SUCCESS;
}

static int recv_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq, int event_id)
{
    int mpi_errno = MPI_SUCCESS;
    size_t count;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RECV_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RECV_EVENT);

    rreq->status.MPI_SOURCE = cqe_get_source(wc, true);
    rreq->status.MPI_ERROR = MPIDI_OFI_idata_get_error_bits(wc->data);
    rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
    count = wc->len;
    MPIR_STATUS_SET_COUNT(rreq->status, count);

#ifndef MPIDI_CH4_DIRECT_NETMOD
    int is_cancelled;
    MPIDI_anysrc_try_cancel_partner(rreq, &is_cancelled);
    /* Cancel SHM partner is always successful */
    MPIR_Assert(is_cancelled);
    MPIDI_anysrc_free_partner(rreq);
#endif
    if ((event_id == MPIDI_OFI_EVENT_RECV_PACK || event_id == MPIDI_OFI_EVENT_GET_HUGE) &&
        (MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer))) {
        MPI_Aint actual_unpack_bytes;
        MPIR_Typerep_unpack(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer), count,
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.buf),
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.count),
                            MPIDI_OFI_REQUEST(rreq, noncontig.pack.datatype), 0,
                            &actual_unpack_bytes);
        MPL_gpu_free_host(MPIDI_OFI_REQUEST(rreq, noncontig.pack.pack_buffer));
        if (actual_unpack_bytes != (MPI_Aint) count) {
            rreq->status.MPI_ERROR =
                MPIR_Err_create_code(MPI_SUCCESS,
                                     MPIR_ERR_RECOVERABLE,
                                     __FUNCTION__, __LINE__, MPI_ERR_TYPE, "**dtypemismatch", 0);
        }
    } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_RECV_NOPACK) &&
               (MPIDI_OFI_REQUEST(rreq, noncontig.nopack))) {
        MPI_Count elements;

        /* Check to see if there are any bytes that don't fit into the datatype basic elements */
        MPIR_Get_elements_x_impl(((MPI_Count *) & count), MPIDI_OFI_REQUEST(rreq, datatype),
                                 &elements);
        if (count)
            MPIR_ERR_SET(rreq->status.MPI_ERROR, MPI_ERR_TYPE, "**dtypemismatch");

        MPL_free(MPIDI_OFI_REQUEST(rreq, noncontig.nopack));
    }

    MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(rreq, datatype));

    /* If synchronous, ack and complete when the ack is done */
    if (unlikely(MPIDI_OFI_is_tag_sync(wc->tag))) {
        uint64_t ss_bits = MPIDI_OFI_init_sendtag(MPIDI_OFI_REQUEST(rreq, util_id),
                                                  rreq->status.MPI_TAG,
                                                  MPIDI_OFI_SYNC_SEND_ACK);
        MPIR_Comm *c = rreq->comm;
        int r = rreq->status.MPI_SOURCE;
        MPIDI_OFI_CALL_RETRY(fi_tinjectdata(MPIDI_OFI_global.ctx[0].tx, NULL /* buf */ ,
                                            0 /* len */ ,
                                            MPIR_Comm_rank(c),
                                            MPIDI_OFI_comm_to_phys(c, r),
                                            ss_bits), tinjectdata, FALSE /* eagain */);
    }

    MPIDIU_request_complete(rreq);

    /* Polling loop will check for truncation */
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RECV_EVENT);
    return mpi_errno;
  fn_fail:
    rreq->status.MPI_ERROR = mpi_errno;
    goto fn_exit;
}

/* If we posted a huge receive, this event gets called to translate the
 * completion queue entry into a get huge event */
static int recv_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;
    MPIR_Comm *comm_ptr;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_RECV_HUGE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_RECV_HUGE_EVENT);

    /* Check that the sender didn't underflow the message by sending less than
     * the huge message threshold. */
    if (wc->len < MPIDI_OFI_global.max_msg_size) {
        return recv_event(wc, rreq, MPIDI_OFI_REQUEST(rreq, event_id));
    }

    comm_ptr = rreq->comm;

    /* Check to see if the tracker is already in the unexpected list.
     * Otherwise, allocate one. */
    {
        MPIDI_OFI_huge_recv_t *list_ptr;

        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "SEARCHING HUGE UNEXPECTED LIST: (%d, %d, %llu)",
                         comm_ptr->context_id, cqe_get_source(wc, false),
                         (MPIDI_OFI_TAG_MASK & wc->tag)));

        LL_FOREACH(MPIDI_unexp_huge_recv_head, list_ptr) {
            if (list_ptr->remote_info.comm_id == comm_ptr->context_id &&
                list_ptr->remote_info.origin_rank == cqe_get_source(wc, false) &&
                list_ptr->remote_info.tag == (MPIDI_OFI_TAG_MASK & wc->tag)) {
                MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                                (MPL_DBG_FDEST, "MATCHED HUGE UNEXPECTED LIST: (%d, %d, %llu, %d)",
                                 comm_ptr->context_id, cqe_get_source(wc, false),
                                 (MPIDI_OFI_TAG_MASK & wc->tag), rreq->handle));

                LL_DELETE(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, list_ptr);

                recv_elem = list_ptr;
                MPIDIU_map_set(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters, rreq->handle, recv_elem,
                               MPL_MEM_COMM);
                break;
            }
        }
    }

    if (recv_elem == NULL) {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        MPL_DBG_MSG_FMT(MPIR_DBG_PT2PT, VERBOSE,
                        (MPL_DBG_FDEST, "CREATING HUGE POSTED ENTRY: (%d, %d, %llu)",
                         comm_ptr->context_id, cqe_get_source(wc, false),
                         (MPIDI_OFI_TAG_MASK & wc->tag)));

        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDIU_map_set(MPIDI_OFI_COMM(comm_ptr).huge_recv_counters, rreq->handle, recv_elem,
                       MPL_MEM_BUFFER);

        list_ptr = (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*list_ptr), 1, MPL_MEM_BUFFER);
        if (!list_ptr)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        list_ptr->comm_id = comm_ptr->context_id;
        list_ptr->rank = cqe_get_source(wc, false);
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
    recv_elem->done_fn = recv_event;
    recv_elem->wc = *wc;
    MPIDI_OFI_get_huge_event(NULL, (MPIR_Request *) recv_elem);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_RECV_HUGE_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int MPIDI_OFI_send_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq, int event_id)
{
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_SEND_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_SEND_EVENT);

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        if ((event_id == MPIDI_OFI_EVENT_SEND_PACK) &&
            (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer))) {
            MPL_gpu_free_host(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
        } else if (MPIDI_OFI_ENABLE_PT2PT_NOPACK && (event_id == MPIDI_OFI_EVENT_SEND_NOPACK))
            MPL_free(MPIDI_OFI_REQUEST(sreq, noncontig.nopack));

        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIR_Request_free(sreq);
    }
    /* c != 0, ssend */
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_SEND_EVENT);
    return MPI_SUCCESS;
}

static int send_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_HUGE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SEND_HUGE_EVENT);

    MPIR_cc_decr(sreq->cc_ptr, &c);

    if (c == 0) {
        MPIR_Comm *comm;
        void *ptr;
        struct fid_mr *huge_send_mr;

        comm = sreq->comm;

        /* Look for the memory region using the sreq handle */
        ptr = MPIDIU_map_lookup(MPIDI_OFI_COMM(comm).huge_send_counters, sreq->handle);
        MPIR_Assert(ptr != MPIDIU_MAP_NOT_FOUND);

        huge_send_mr = (struct fid_mr *) ptr;

        /* Send a cleanup message to the receivier and clean up local
         * resources. */
        /* Clean up the local counter */
        MPIDIU_map_erase(MPIDI_OFI_COMM(comm).huge_send_counters, sreq->handle);

        /* Clean up the memory region */
        if (!MPIDI_OFI_ENABLE_MR_PROV_KEY) {
            uint64_t key = fi_mr_key(huge_send_mr);
            MPIDI_OFI_mr_key_free(key);
        }
        MPIDI_OFI_CALL(fi_close(&huge_send_mr->fid), mr_unreg);

        if (MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer)) {
            MPL_gpu_free_host(MPIDI_OFI_REQUEST(sreq, noncontig.pack.pack_buffer));
        }

        MPIR_Datatype_release_if_not_builtin(MPIDI_OFI_REQUEST(sreq, datatype));
        MPIR_Request_free(sreq);
    }
    /* c != 0, ssend */
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SEND_HUGE_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int ssend_ack_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno;
    MPIDI_OFI_ssendack_request_t *req = (MPIDI_OFI_ssendack_request_t *) sreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SSEND_ACK_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_SSEND_ACK_EVENT);
    mpi_errno =
        MPIDI_OFI_send_event(NULL, req->signal_req, MPIDI_OFI_REQUEST(req->signal_req, event_id));

    MPL_free(req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_SSEND_ACK_EVENT);
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

int MPIDI_OFI_get_huge_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv_elem = (MPIDI_OFI_huge_recv_t *) req;
    uint64_t remote_key;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_GET_HUGE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_GET_HUGE_EVENT);

    if (recv_elem->localreq && recv_elem->cur_offset != 0) {    /* If this is true, then the message has a posted
                                                                 * receive already and we'll be able to find the
                                                                 * struct describing the transfer. */
        /* Subtract one max_msg_size because we send the first chunk via a regular message instead of the memory region */
        size_t bytesSent = recv_elem->cur_offset - MPIDI_OFI_global.max_msg_size;
        size_t bytesLeft =
            recv_elem->remote_info.msgsize - bytesSent - MPIDI_OFI_global.max_msg_size;
        size_t bytesToGet =
            (bytesLeft <=
             MPIDI_OFI_global.max_msg_size) ? bytesLeft : MPIDI_OFI_global.max_msg_size;

        if (bytesToGet == 0ULL) {
            MPIDI_OFI_send_control_t ctrl;
            /* recv_elem->localreq may be freed during done_fn.
             * Need to backup the handle here for later use with MPIDIU_map_erase. */
            uint64_t key_to_erase = recv_elem->localreq->handle;
            recv_elem->wc.len = recv_elem->cur_offset;
            recv_elem->done_fn(&recv_elem->wc, recv_elem->localreq, recv_elem->event_id);
            ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
            mpi_errno =
                MPIDI_OFI_do_control_send(&ctrl, NULL, 0, recv_elem->remote_info.origin_rank,
                                          recv_elem->comm_ptr, recv_elem->remote_info.ackreq);
            MPIR_ERR_CHECK(mpi_errno);

            MPIDIU_map_erase(MPIDI_OFI_COMM(recv_elem->comm_ptr).huge_recv_counters, key_to_erase);
            MPL_free(recv_elem);

            goto fn_exit;
        }

        remote_key = recv_elem->remote_info.rma_key;

        MPIDI_OFI_cntr_incr();
        MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[0].tx,        /* endpoint     */
                                     (void *) ((uintptr_t) recv_elem->wc.buf + recv_elem->cur_offset),  /* local buffer */
                                     bytesToGet,        /* bytes        */
                                     NULL,      /* descriptor   */
                                     MPIDI_OFI_comm_to_phys(recv_elem->comm_ptr, recv_elem->remote_info.origin_rank),   /* Destination  */
                                     recv_rbase(recv_elem) + recv_elem->cur_offset,     /* remote maddr */
                                     remote_key,        /* Key          */
                                     (void *) &recv_elem->context), rdma_readfrom,      /* Context */
                             FALSE);
        recv_elem->cur_offset += bytesToGet;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_GET_HUGE_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int chunk_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int c;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CHUNK_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_CHUNK_DONE_EVENT);

    MPIDI_OFI_chunk_request *creq = (MPIDI_OFI_chunk_request *) req;
    MPIR_cc_decr(creq->parent->cc_ptr, &c);

    if (c == 0)
        MPIR_Request_free(creq->parent);

    MPL_free(creq);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_CHUNK_DONE_EVENT);
    return MPI_SUCCESS;
}

static int inject_emu_event(struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int incomplete;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_INJECT_EMU_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_INJECT_EMU_EVENT);

    MPIR_cc_decr(req->cc_ptr, &incomplete);

    if (!incomplete) {
        MPL_free(MPIDI_OFI_REQUEST(req, util.inject_buf));
        MPIR_Request_free(req);
        MPL_atomic_fetch_sub_int(&MPIDI_OFI_global.am_inflight_inject_emus, 1);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_INJECT_EMU_EVENT);
    return MPI_SUCCESS;
}

int MPIDI_OFI_rma_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * in_req)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_RMA_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_RMA_DONE_EVENT);

    MPIDI_OFI_win_request_t *req = (MPIDI_OFI_win_request_t *) in_req;
    MPIDI_OFI_win_request_complete(req);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_RMA_DONE_EVENT);
    return MPI_SUCCESS;
}

static int accept_probe_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACCEPT_PROBE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_ACCEPT_PROBE_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->source = cqe_get_source(wc, false);
    ctrl->tag = MPIDI_OFI_init_get_tag(wc->tag);
    ctrl->msglen = wc->len;
    ctrl->done = MPIDI_OFI_PEEK_FOUND;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_ACCEPT_PROBE_EVENT);
    return MPI_SUCCESS;
}

static int dynproc_done_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DYNPROC_DONE_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DYNPROC_DONE_EVENT);
    MPIDI_OFI_dynamic_process_request_t *ctrl = (MPIDI_OFI_dynamic_process_request_t *) rreq;
    ctrl->done++;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DYNPROC_DONE_EVENT);
    return MPI_SUCCESS;
}

static int am_isend_event(struct fi_cq_tagged_entry *wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *msg_hdr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_AM_ISEND_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_AM_ISEND_EVENT);

    msg_hdr = &MPIDI_OFI_AMREQUEST_HDR(sreq, msg_hdr);
    MPID_Request_complete(sreq);        /* FIXME: Should not call MPIDI in NM ? */

    switch (msg_hdr->am_type) {
        case MPIDI_AMTYPE_LMT_ACK:
        case MPIDI_AMTYPE_LMT_REQ:
            goto fn_exit;

        default:
            break;
    }

    MPL_gpu_free_host(MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer));
    MPIDI_OFI_AMREQUEST_HDR(sreq, pack_buffer) = NULL;

    mpi_errno = MPIDIG_global.origin_cbs[msg_hdr->handler_id] (sreq);

    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_AM_ISEND_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_recv_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_am_header_t *am_hdr;
    MPIDI_OFI_am_unordered_msg_t *uo_msg = NULL;
    fi_addr_t fi_src_addr;
    uint16_t expected_seqno, next_seqno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_AM_RECV_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_AM_RECV_EVENT);

    am_hdr = (MPIDI_OFI_am_header_t *) wc->buf;

    expected_seqno = MPIDI_OFI_am_get_next_recv_seqno(am_hdr->fi_src_addr);
    if (am_hdr->seqno != expected_seqno) {
        /* This message came earlier than the one that we were expecting.
         * Put it in the queue to process it later. */
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, TERSE,
                        (MPL_DBG_FDEST,
                         "Expected seqno=%d but got %d (am_type=%d addr=%" PRIx64 "). "
                         "Enqueueing it to the queue.\n",
                         expected_seqno, am_hdr->seqno, am_hdr->am_type, am_hdr->fi_src_addr));
        mpi_errno = MPIDI_OFI_am_enqueue_unordered_msg(am_hdr);
        MPIR_ERR_CHECK(mpi_errno);
        goto fn_exit;
    }

    /* Received an expected message */
  repeat:
    fi_src_addr = am_hdr->fi_src_addr;
    next_seqno = am_hdr->seqno + 1;
    switch (am_hdr->am_type) {
        case MPIDI_AMTYPE_SHORT_HDR:
            mpi_errno = MPIDI_OFI_handle_short_am_hdr(am_hdr, am_hdr + 1 /* payload */);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        case MPIDI_AMTYPE_SHORT:
            mpi_errno = MPIDI_OFI_handle_short_am(am_hdr);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        case MPIDI_AMTYPE_LMT_REQ:
            mpi_errno = MPIDI_OFI_handle_long_am(am_hdr);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        case MPIDI_AMTYPE_LMT_ACK:
            mpi_errno = MPIDI_OFI_handle_lmt_ack(am_hdr);

            MPIR_ERR_CHECK(mpi_errno);

            break;

        default:
            MPIR_Assert(0);
    }

    /* For the first iteration (=in case we can process the message just received
     * from OFI immediately), uo_msg is NULL, so freeing it is no-op.
     * Otherwise, free it here before getting another uo_msg. */
    MPL_free(uo_msg);

    /* See if we can process other messages in the queue */
    if ((uo_msg = MPIDI_OFI_am_claim_unordered_msg(fi_src_addr, next_seqno)) != NULL) {
        am_hdr = &uo_msg->am_hdr;
        goto repeat;
    }

    /* Record the next expected sequence number from fi_src_addr */
    MPIDI_OFI_am_set_next_recv_seqno(fi_src_addr, next_seqno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_AM_RECV_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_read_event(struct fi_cq_tagged_entry *wc, MPIR_Request * dont_use_me)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIDI_OFI_am_request_t *ofi_req;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_AM_READ_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_AM_READ_EVENT);

    ofi_req = MPL_container_of(wc->op_context, MPIDI_OFI_am_request_t, context);
    rreq = (MPIR_Request *) ofi_req->req_hdr->rreq_ptr;

    if (ofi_req->req_hdr->lmt_type == MPIDI_OFI_AM_LMT_IOV) {
        ofi_req->req_hdr->lmt_u.lmt_cntr--;
        if (ofi_req->req_hdr->lmt_u.lmt_cntr) {
            goto fn_exit;
        }
    } else if (ofi_req->req_hdr->lmt_type == MPIDI_OFI_AM_LMT_UNPACK) {
        int done = MPIDI_OFI_am_lmt_unpack_event(rreq);
        if (!done) {
            goto fn_exit;
        }
    }

    mpi_errno = MPIDI_OFI_dispatch_ack(MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).src_rank,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).context_id,
                                       MPIDI_OFI_AMREQUEST_HDR(rreq, lmt_info).sreq_ptr,
                                       MPIDI_AMTYPE_LMT_ACK);

    MPIR_ERR_CHECK(mpi_errno);

    MPIDIG_REQUEST(rreq, req->target_cmpl_cb) (rreq);
    MPID_Request_complete(rreq);
  fn_exit:
    MPIDIU_release_buf((void *) ofi_req);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_AM_READ_EVENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int am_repost_event(struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_AM_REPOST_EVENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_AM_REPOST_EVENT);

    mpi_errno = MPIDI_OFI_repost_buffer(wc->op_context, rreq);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_AM_REPOST_EVENT);
    return mpi_errno;
}

int MPIDI_OFI_dispatch_function(struct fi_cq_tagged_entry *wc, MPIR_Request * req)
{
    int mpi_errno = MPI_SUCCESS;

    if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_SEND)) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = MPIDI_OFI_send_event(wc, req, MPIDI_OFI_EVENT_SEND);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RECV)) {
        /* Passing the event_id as a parameter; do not need to load it from the
         * request object each time the send_event handler is invoked */
        mpi_errno = recv_event(wc, req, MPIDI_OFI_EVENT_RECV);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_RMA_DONE)) {
        mpi_errno = MPIDI_OFI_rma_done_event(wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_SEND)) {
        mpi_errno = am_isend_event(wc, req);
        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_RECV)) {
        if (wc->flags & FI_RECV)
            mpi_errno = am_recv_event(wc, req);

        if (unlikely(wc->flags & FI_MULTI_RECV))
            mpi_errno = am_repost_event(wc, req);

        goto fn_exit;
    } else if (likely(MPIDI_OFI_REQUEST(req, event_id) == MPIDI_OFI_EVENT_AM_READ)) {
        mpi_errno = am_read_event(wc, req);
        goto fn_exit;
    } else if (unlikely(1)) {
        switch (MPIDI_OFI_REQUEST(req, event_id)) {
            case MPIDI_OFI_EVENT_PEEK:
                mpi_errno = peek_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_RECV_HUGE:
                mpi_errno = recv_huge_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_RECV_PACK:
                mpi_errno = recv_event(wc, req, MPIDI_OFI_EVENT_RECV_PACK);
                break;

            case MPIDI_OFI_EVENT_RECV_NOPACK:
                mpi_errno = recv_event(wc, req, MPIDI_OFI_EVENT_RECV_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SEND_HUGE:
                mpi_errno = send_huge_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_SEND_PACK:
                mpi_errno = MPIDI_OFI_send_event(wc, req, MPIDI_OFI_EVENT_SEND_PACK);
                break;

            case MPIDI_OFI_EVENT_SEND_NOPACK:
                mpi_errno = MPIDI_OFI_send_event(wc, req, MPIDI_OFI_EVENT_SEND_NOPACK);
                break;

            case MPIDI_OFI_EVENT_SSEND_ACK:
                mpi_errno = ssend_ack_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_GET_HUGE:
                mpi_errno = MPIDI_OFI_get_huge_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_CHUNK_DONE:
                mpi_errno = chunk_done_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_INJECT_EMU:
                mpi_errno = inject_emu_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_DYNPROC_DONE:
                mpi_errno = dynproc_done_event(wc, req);
                break;

            case MPIDI_OFI_EVENT_ACCEPT_PROBE:
                mpi_errno = accept_probe_event(wc, req);
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

int MPIDI_OFI_get_buffered(struct fi_cq_tagged_entry *wc, ssize_t num)
{
    int rc = 0;

    if ((MPIDI_OFI_global.cq_buffered_static_head != MPIDI_OFI_global.cq_buffered_static_tail) ||
        (NULL != MPIDI_OFI_global.cq_buffered_dynamic_head)) {
        /* If the static list isn't empty, do so first */
        if (MPIDI_OFI_global.cq_buffered_static_head != MPIDI_OFI_global.cq_buffered_static_tail) {
            wc[0] =
                MPIDI_OFI_global.cq_buffered_static_list[MPIDI_OFI_global.
                                                         cq_buffered_static_tail].cq_entry;
            MPIDI_OFI_global.cq_buffered_static_tail =
                (MPIDI_OFI_global.cq_buffered_static_tail + 1) % MPIDI_OFI_NUM_CQ_BUFFERED;
        }
        /* If there's anything in the dynamic list, it goes second. */
        else if (NULL != MPIDI_OFI_global.cq_buffered_dynamic_head) {
            MPIDI_OFI_cq_list_t *cq_list_entry = MPIDI_OFI_global.cq_buffered_dynamic_head;
            LL_DELETE(MPIDI_OFI_global.cq_buffered_dynamic_head,
                      MPIDI_OFI_global.cq_buffered_dynamic_tail, cq_list_entry);
            wc[0] = cq_list_entry->cq_entry;
            MPL_free(cq_list_entry);
        }

        rc = 1;
    }

    return rc;
}

int MPIDI_OFI_handle_cq_entries(struct fi_cq_tagged_entry *wc, ssize_t num)
{
    int i, mpi_errno = MPI_SUCCESS;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);

    for (i = 0; i < num; i++) {
        req = MPIDI_OFI_context_to_request(wc[i].op_context);
        mpi_errno = MPIDI_OFI_dispatch_function(&wc[i], req);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ENTRIES);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDI_OFI_handle_cq_error(int vni_idx, ssize_t ret)
{
    int mpi_errno = MPI_SUCCESS;
    struct fi_cq_err_entry e;
    MPIR_Request *req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR);

    switch (ret) {
        case -FI_EAVAIL:
            fi_cq_readerr(MPIDI_OFI_global.ctx[vni_idx].cq, &e, 0);

            switch (e.err) {
                case FI_ETRUNC:
                    req = MPIDI_OFI_context_to_request(e.op_context);

                    switch (req->kind) {
                        case MPIR_REQUEST_KIND__SEND:
                            mpi_errno = MPIDI_OFI_dispatch_function(NULL, req);
                            break;

                        case MPIR_REQUEST_KIND__RECV:
                            mpi_errno =
                                MPIDI_OFI_dispatch_function((struct fi_cq_tagged_entry *) &e, req);
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
                    peek_empty_event(NULL, req);
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
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_HANDLE_CQ_ERROR);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
