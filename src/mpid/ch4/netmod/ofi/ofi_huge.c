/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

/* this function called by recv event of a huge message */
int MPIDI_OFI_recv_huge_event(int vni, struct fi_cq_tagged_entry *wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;
    MPIR_Comm *comm_ptr;
    MPIR_FUNC_ENTER;

    bool ready_to_get = false;
    if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        MPIR_Assert(wc->len == MPIDI_OFI_STRIPE_CHUNK_SIZE);
    } else {
        MPIR_Assert(wc->len == MPIDI_OFI_global.max_msg_size);
    }

    comm_ptr = rreq->comm;
    MPIR_T_PVAR_COUNTER_INC(MULTINIC, nic_recvd_bytes_count[MPIDI_OFI_REQUEST(rreq, nic_num)],
                            wc->len);
    /* Check to see if the tracker is already in the unexpected list.
     * Otherwise, allocate one. */
    {
        MPIDI_OFI_huge_recv_t *list_ptr;

        LL_FOREACH(MPIDI_unexp_huge_recv_head, list_ptr) {
            if (list_ptr->remote_info.comm_id == comm_ptr->context_id &&
                list_ptr->remote_info.origin_rank == MPIDI_OFI_cqe_get_source(wc, false) &&
                list_ptr->remote_info.tag == (MPIDI_OFI_TAG_MASK & wc->tag)) {
                LL_DELETE(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, list_ptr);

                recv_elem = list_ptr;
                MPIDI_OFI_REQUEST(rreq, huge_info.recv_elem) = recv_elem;
                break;
            }
        }
    }

    if (recv_elem) {
        ready_to_get = true;
    } else {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        MPIDI_OFI_REQUEST(rreq, huge_info.recv_elem) = recv_elem;

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

/* This function is called when we receive a huge control message */
int MPIDI_OFI_recv_huge_control(MPIDI_OFI_huge_remote_info_t * info)
{
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    bool ready_to_get = false;

    /* If there has been a posted receive, search through the list of unmatched
     * receives to find the one that goes with the incoming message. */
    {
        MPIDI_OFI_huge_recv_list_t *list_ptr;

        LL_FOREACH(MPIDI_posted_huge_recv_head, list_ptr) {
            if (list_ptr->comm_id == info->comm_id &&
                list_ptr->rank == info->origin_rank && list_ptr->tag == info->tag) {
                LL_DELETE(MPIDI_posted_huge_recv_head, MPIDI_posted_huge_recv_tail, list_ptr);

                recv_elem = MPIDI_OFI_REQUEST(rreq, huge_info.recv_elem);

                /* If this is a "peek" element for an MPI_Probe, it shouldn't be matched. Grab the
                 * important information and remove the element from the list. */
                if (recv_elem->peek) {
                    MPIR_STATUS_SET_COUNT(recv_elem->localreq->status, info->msgsize);
                    MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(recv_elem->localreq, util_id)),
                                                 MPIDI_OFI_PEEK_FOUND);
                    MPL_free(recv_elem);
                    recv_elem = NULL;
                }

                MPL_free(list_ptr);
                break;
            }
        }
    }

    if (recv_elem) {
        ready_to_get = true;
    } else {
        /* Put the struct describing the transfer on an unexpected list to be retrieved later */
        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_COMM);
        if (!recv_elem)
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");

        LL_APPEND(MPIDI_unexp_huge_recv_head, MPIDI_unexp_huge_recv_tail, recv_elem);
    }

    recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv_elem->remote_info = *info;
    recv_elem->next = NULL;
    if (ready_to_get) {
        MPIDI_OFI_get_huge_event(info->vni_dst, NULL, (MPIR_Request *) recv_elem);
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
    if (found_msg) {
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
        /* post the rreq to list and let control handler handle it */
        MPIDI_OFI_huge_recv_t *recv_elem;
        MPIDI_OFI_huge_recv_list_t *huge_list_ptr;

        /* Create an element in the posted list that only indicates a peek and will be
         * deleted as soon as it's fulfilled without being matched. */
        recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        recv_elem->peek = true;
        MPIR_Comm *comm_ptr = rreq->comm;
        recv_elem->comm_ptr = comm_ptr;
        MPIDI_OFI_REQUEST(rreq, huge_info.recv_elem) = recv_elem;

        huge_list_ptr =
            (MPIDI_OFI_huge_recv_list_t *) MPL_calloc(sizeof(*huge_list_ptr), 1, MPL_MEM_COMM);
        MPIR_ERR_CHKANDJUMP(huge_list_ptr == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        recv_elem->remote_info.comm_id = huge_list_ptr->comm_id = MPIDI_OFI_CONTEXT_MASK & wc->tag;
        recv_elem->remote_info.origin_rank = huge_list_ptr->rank =
            MPIDI_OFI_cqe_get_source(wc, false);
        recv_elem->remote_info.tag = huge_list_ptr->tag = MPIDI_OFI_TAG_MASK & wc->tag;
        recv_elem->localreq = huge_list_ptr->rreq = rreq;
        recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
        recv_elem->wc = *wc;
        if (MPIDI_OFI_COMM(comm_ptr).enable_striping) {
            recv_elem->cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
        } else {
            recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;
        }

        LL_APPEND(MPIDI_posted_huge_recv_head, MPIDI_posted_huge_recv_tail, huge_list_ptr);

        /* FIXME: we don't have the correct count so it wrong to return FOUND here */
        rreq->status.MPI_SOURCE = MPIDI_OFI_cqe_get_source(wc, false);
        rreq->status.MPI_TAG = MPIDI_OFI_init_get_tag(wc->tag);
        rreq->status.MPI_ERROR = MPI_SUCCESS;
        MPIR_STATUS_SET_COUNT(rreq->status, count);
        /* util_id should be the last thing to change in rreq. Reason is
         * we use util_id to indicate peek_event has completed and all the
         * relevant values have been copied to rreq. */
        MPL_atomic_release_store_int(&(MPIDI_OFI_REQUEST(rreq, util_id)), MPIDI_OFI_PEEK_FOUND);
    }


  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static uintptr_t recv_rbase(MPIDI_OFI_huge_recv_t * recv_elem)
{
    if (!MPIDI_OFI_ENABLE_MR_VIRT_ADDRESS) {
        return 0;
    } else {
        return (uintptr_t) recv_elem->remote_info.send_buf;
    }
}

/* Note: MPIDI_OFI_get_huge_event is invoked from three places --
 * 1. In MPIDI_OFI_recv_huge_event, when recv buffer is matched and first chunk received, and
 *    when control message (with remote info) has also been received.
 * 2. In MPIDI_OFI_recv_huge_control, as a callback when control message is received, and
 *    when first chunk has been matched and received.
 *
 * MPIDI_OFI_recv_huge_event will fill the local request information, and
 * MPIDI_OFI_recv_huge_control will fill the remote (sender) information. Lastly --
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
    MPI_Aint data_sz = MPIDI_OFI_REQUEST(recv_elem->localreq, util.iov.iov_len);
    if (recv_elem->remote_info.msgsize > data_sz) {
        recv_elem->localreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        recv_elem->remote_info.msgsize = data_sz;
    }

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
        /* recv_elem->localreq may be freed during MPIDI_OFI_recv_event.
         * Need to backup the handle here for later use with MPIDIU_map_erase. */
        uint64_t key_to_erase = recv_elem->localreq->handle;
        recv_elem->wc.len = recv_elem->cur_offset;
        MPIDI_OFI_recv_event(recv_elem->remote_info.vni_dst, &recv_elem->wc, recv_elem->localreq,
                             recv_elem->event_id);
        ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
        ctrl.u.huge_ack.ackreq = recv_elem->remote_info.ackreq;
        /* note: it's receiver ack sender */
        int vni_remote = recv_elem->remote_info.vni_src;
        int vni_local = recv_elem->remote_info.vni_dst;
        mpi_errno = MPIDI_NM_am_send_hdr(recv_elem->remote_info.origin_rank, recv_elem->comm_ptr,
                                         MPIDI_OFI_INTERNAL_HANDLER_CONTROL,
                                         &ctrl, sizeof(ctrl), vni_local, vni_remote);
        MPIR_ERR_CHECK(mpi_errno);

        MPL_free(recv_elem);

        goto fn_exit;
    }

    int vni_src = recv_elem->remote_info.vni_src;
    int vni_dst = recv_elem->remote_info.vni_dst;
    if (MPIDI_OFI_COMM(recv_elem->comm_ptr).enable_striping) {  /* if striping enabled */
        if (recv_elem->cur_offset >= MPIDI_OFI_STRIPE_CHUNK_SIZE && bytesLeft > 0) {
            for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
                int ctx_idx = MPIDI_OFI_get_ctx_index(recv_elem->comm_ptr, vni_dst, nic);
                remote_key = recv_elem->remote_info.rma_keys[nic];

                bytesLeft = recv_elem->remote_info.msgsize - recv_elem->cur_offset;
                if (bytesLeft <= 0) {
                    break;
                }
                bytesToGet =
                    (bytesLeft <= recv_elem->stripe_size) ? bytesLeft : recv_elem->stripe_size;

                /* FIXME: Can we issue concurrent fi_read with the same context? */
                MPIDI_OFI_cntr_incr(recv_elem->comm_ptr, vni_src, nic);
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
        int nic = 0;
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
