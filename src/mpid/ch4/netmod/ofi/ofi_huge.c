/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpidimpl.h>
#include "ofi_impl.h"
#include "ofi_events.h"

static int get_huge(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(rreq, huge.remote_info);
    MPIDI_OFI_huge_recv_t *recv_elem = NULL;

    recv_elem = (MPIDI_OFI_huge_recv_t *) MPL_calloc(sizeof(*recv_elem), 1, MPL_MEM_BUFFER);
    MPIR_ERR_CHKANDJUMP(recv_elem == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
    recv_elem->event_id = MPIDI_OFI_EVENT_GET_HUGE;
    recv_elem->localreq = rreq;
    if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
        recv_elem->cur_offset = MPIDI_OFI_STRIPE_CHUNK_SIZE;
    } else {
        recv_elem->cur_offset = MPIDI_OFI_global.max_msg_size;
    }
    MPIDI_OFI_get_huge_event(info->vni_dst, NULL, (MPIR_Request *) recv_elem);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static int get_huge_complete(MPIDI_OFI_huge_recv_t * recv_elem)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(recv_elem->localreq, huge.remote_info);

    /* note: it's receiver ack sender */
    int vni_remote = info->vni_src;
    int vni_local = info->vni_dst;

    struct fi_cq_tagged_entry wc;
    wc.len = recv_elem->cur_offset;
    wc.data = info->origin_rank;
    wc.tag = info->tag;
    MPIDI_OFI_recv_event(vni_local, &wc, recv_elem->localreq, MPIDI_OFI_EVENT_GET_HUGE);

    MPIDI_OFI_send_control_t ctrl;
    ctrl.type = MPIDI_OFI_CTRL_HUGEACK;
    ctrl.u.huge_ack.ackreq = info->ackreq;
    mpi_errno = MPIDI_NM_am_send_hdr(info->origin_rank, comm,
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
    if (MPIDI_OFI_COMM(rreq->comm).enable_striping) {
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
        int comm_id = comm_ptr->context_id;
        int rank = MPIDI_OFI_cqe_get_source(wc, false);
        int tag = (MPIDI_OFI_TAG_MASK & wc->tag);

        LL_FOREACH(MPIDI_huge_ctrl_head, list_ptr) {
            if (list_ptr->comm_id == comm_id && list_ptr->rank == rank && list_ptr->tag == tag) {
                MPIDI_OFI_REQUEST(rreq, huge.remote_info) = list_ptr->u.info;
                LL_DELETE(MPIDI_huge_ctrl_head, MPIDI_huge_ctrl_tail, list_ptr);
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

        list_ptr->comm_id = comm_ptr->context_id;
        list_ptr->rank = MPIDI_OFI_cqe_get_source(wc, false);
        list_ptr->tag = (MPIDI_OFI_TAG_MASK & wc->tag);
        list_ptr->u.rreq = rreq;

        LL_APPEND(MPIDI_huge_recv_head, MPIDI_huge_recv_tail, list_ptr);
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
int MPIDI_OFI_recv_huge_control(int comm_id, int rank, int tag,
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
    LL_FOREACH(MPIDI_huge_recv_head, list_ptr) {
        if (list_ptr->comm_id == comm_id && list_ptr->rank == rank && list_ptr->tag == tag) {
            rreq = list_ptr->u.rreq;
            LL_DELETE(MPIDI_huge_recv_head, MPIDI_huge_recv_tail, list_ptr);
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

        LL_APPEND(MPIDI_huge_ctrl_head, MPIDI_huge_ctrl_tail, list_ptr);
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
    LL_FOREACH(MPIDI_huge_ctrl_head, list_ptr) {
        /* FIXME: fix the type of comm_id */
        uint64_t context_id = MPIDI_OFI_CONTEXT_MASK & wc->tag;
        int rank = MPIDI_OFI_cqe_get_source(wc, false);
        int tag = (int) (MPIDI_OFI_TAG_MASK & wc->tag);
        if (list_ptr->comm_id == context_id && list_ptr->rank == rank && list_ptr->tag == tag) {
            count = list_ptr->u.info->msgsize;
            found_msg = true;
            break;
        }
    }
    if (found_msg) {
        if (MPIDI_OFI_REQUEST(rreq, kind) == MPIDI_OFI_req_kind__mprobe) {
            MPIDI_OFI_REQUEST(rreq, huge.remote_info) = list_ptr->u.info;
            LL_DELETE(MPIDI_huge_ctrl_head, MPIDI_huge_ctrl_tail, list_ptr);
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

        huge_list_ptr->comm_id = MPIDI_OFI_CONTEXT_MASK & wc->tag;
        huge_list_ptr->rank = MPIDI_OFI_cqe_get_source(wc, false);
        huge_list_ptr->tag = MPIDI_OFI_TAG_MASK & wc->tag;
        huge_list_ptr->u.rreq = rreq;

        LL_APPEND(MPIDI_huge_recv_head, MPIDI_huge_recv_tail, huge_list_ptr);
    }


  fn_exit:
    MPIR_FUNC_EXIT;
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
    MPIDI_OFI_huge_remote_info_t *info = MPIDI_OFI_REQUEST(recv_elem->localreq, huge.remote_info);
    MPIR_Comm *comm = recv_elem->localreq->comm;
    uint64_t remote_key;
    size_t bytesLeft, bytesToGet;
    MPIR_FUNC_ENTER;

    void *recv_buf = MPIDI_OFI_REQUEST(recv_elem->localreq, util.iov.iov_base);
    MPI_Aint data_sz = MPIDI_OFI_REQUEST(recv_elem->localreq, util.iov.iov_len);
    if (info->msgsize > data_sz) {
        recv_elem->localreq->status.MPI_ERROR = MPI_ERR_TRUNCATE;
        info->msgsize = data_sz;
    }

    if (MPIDI_OFI_COMM(comm).enable_striping) {
        /* Subtract one stripe_chunk_size because we send the first chunk via a regular message
         * instead of the memory region */
        recv_elem->stripe_size = (info->msgsize - MPIDI_OFI_STRIPE_CHUNK_SIZE)
            / MPIDI_OFI_global.num_nics;        /* striping */

        if (recv_elem->stripe_size > MPIDI_OFI_global.max_msg_size) {
            recv_elem->stripe_size = MPIDI_OFI_global.max_msg_size;
        }
        if (recv_elem->chunks_outstanding)
            recv_elem->chunks_outstanding--;
        bytesLeft = info->msgsize - recv_elem->cur_offset;
        bytesToGet = (bytesLeft <= recv_elem->stripe_size) ? bytesLeft : recv_elem->stripe_size;
    } else {
        /* Subtract one max_msg_size because we send the first chunk via a regular message
         * instead of the memory region */
        bytesLeft = info->msgsize - recv_elem->cur_offset;
        bytesToGet = (bytesLeft <= MPIDI_OFI_global.max_msg_size) ?
            bytesLeft : MPIDI_OFI_global.max_msg_size;
    }
    if (bytesToGet == 0ULL && recv_elem->chunks_outstanding == 0) {
        mpi_errno = get_huge_complete(recv_elem);
        MPIR_ERR_CHECK(mpi_errno);
        MPL_free(recv_elem);
        goto fn_exit;
    }

    int vni_src = info->vni_src;
    int vni_dst = info->vni_dst;
    if (MPIDI_OFI_COMM(comm).enable_striping) { /* if striping enabled */
        if (recv_elem->cur_offset >= MPIDI_OFI_STRIPE_CHUNK_SIZE && bytesLeft > 0) {
            for (int nic = 0; nic < MPIDI_OFI_global.num_nics; nic++) {
                int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_dst, nic);
                remote_key = info->rma_keys[nic];

                bytesLeft = info->msgsize - recv_elem->cur_offset;
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
                                             MPIDI_OFI_comm_to_phys(comm, info->origin_rank, nic, vni_dst, vni_src), recv_rbase(info) + recv_elem->cur_offset,  /* remote maddr */
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
        int ctx_idx = MPIDI_OFI_get_ctx_index(comm, vni_src, nic);
        remote_key = info->rma_keys[nic];
        MPIDI_OFI_cntr_incr(comm, vni_src, nic);
        MPIDI_OFI_CALL_RETRY(fi_read(MPIDI_OFI_global.ctx[ctx_idx].tx,  /* endpoint     */
                                     (void *) ((char *) recv_buf + recv_elem->cur_offset),      /* local buffer */
                                     bytesToGet,        /* bytes        */
                                     NULL,      /* descriptor   */
                                     MPIDI_OFI_comm_to_phys(comm, info->origin_rank, nic, vni_src, vni_dst),    /* Destination  */
                                     recv_rbase(info) + recv_elem->cur_offset,  /* remote maddr */
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
