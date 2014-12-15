/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPID_RMA_H_INCLUDED)
#define MPID_RMA_H_INCLUDED

#include "mpid_rma_types.h"
#include "mpid_rma_oplist.h"
#include "mpid_rma_shm.h"
#include "mpid_rma_issue.h"
#include "mpid_rma_lockqueue.h"

#undef FUNCNAME
#define FUNCNAME send_lock_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_lock_msg(int dest, int lock_type, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_LOCK_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_LOCK_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
    lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    lock_pkt->source_win_handle = win_ptr->handle;
    lock_pkt->lock_type = lock_type;
    lock_pkt->origin_rank = win_ptr->comm_ptr->rank;

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_pkt, sizeof(*lock_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_LOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME send_unlock_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_unlock_msg(int dest, MPID_Win * win_ptr,
                                  MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_unlock_t *unlock_pkt = &upkt.unlock;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_UNLOCK_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_UNLOCK_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    /* Send a lock packet over to the target. wait for the lock_granted
     * reply. Then do all the RMA ops. */

    MPIDI_Pkt_init(unlock_pkt, MPIDI_CH3_PKT_UNLOCK);
    unlock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    unlock_pkt->source_win_handle = win_ptr->handle;
    unlock_pkt->flags = flags;

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, unlock_pkt, sizeof(*unlock_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_UNLOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_ack_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_lock_ack_pkt(MPIDI_VC_t * vc, MPID_Win * win_ptr,
                                               MPIDI_CH3_Pkt_flags_t flags,
                                               MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_ack_t *lock_ack_pkt = &upkt.lock_ack;
    MPID_Request *req = NULL;
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    /* send lock ack packet */
    MPIDI_Pkt_init(lock_ack_pkt, MPIDI_CH3_PKT_LOCK_ACK);
    lock_ack_pkt->source_win_handle = source_win_handle;
    lock_ack_pkt->target_rank = win_ptr->comm_ptr->rank;
    lock_ack_pkt->flags = flags;

    MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE,
                     (MPIU_DBG_FDEST, "sending lock ack pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_ack_pkt->source_win_handle));

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_ack_pkt, sizeof(*lock_ack_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    if (mpi_errno) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_op_ack_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_lock_op_ack_pkt(MPIDI_VC_t * vc, MPID_Win * win_ptr,
                                                  MPIDI_CH3_Pkt_flags_t flags,
                                                  MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_op_ack_t *lock_op_ack_pkt = &upkt.lock_op_ack;
    MPID_Request *req = NULL;
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);

    /* send lock ack packet */
    MPIDI_Pkt_init(lock_op_ack_pkt, MPIDI_CH3_PKT_LOCK_OP_ACK);
    lock_op_ack_pkt->source_win_handle = source_win_handle;
    lock_op_ack_pkt->target_rank = win_ptr->comm_ptr->rank;
    lock_op_ack_pkt->flags = flags;

    MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE,
                     (MPIU_DBG_FDEST, "sending lock op ack pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_op_ack_pkt->source_win_handle));

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_op_ack_pkt, sizeof(*lock_op_ack_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    if (mpi_errno) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_flush_ack_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_flush_ack_pkt(MPIDI_VC_t *vc, MPID_Win *win_ptr,
                                    MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_ack_t *flush_ack_pkt = &upkt.flush_ack;
    MPID_Request *req;
    int mpi_errno=MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_FLUSH_ACK_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_FLUSH_ACK_PKT);

    MPIDI_Pkt_init(flush_ack_pkt, MPIDI_CH3_PKT_FLUSH_ACK);
    flush_ack_pkt->source_win_handle = source_win_handle;
    flush_ack_pkt->target_rank = win_ptr->comm_ptr->rank;

    /* Because this is in a packet handler, it is already within a critical section */	
    /* MPIU_THREAD_CS_ENTER(CH3COMM,vc); */
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_ack_pkt, sizeof(*flush_ack_pkt), &req);
    /* MPIU_THREAD_CS_EXIT(CH3COMM,vc); */
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }

    if (req != NULL)
    {
        MPID_Request_release(req);
    }

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_FLUSH_ACK_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME send_decr_at_cnt_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_decr_at_cnt_msg(int dst, MPID_Win * win_ptr)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_decr_at_counter_t *decr_at_cnt_pkt = &upkt.decr_at_cnt;
    MPIDI_VC_t * vc;
    MPID_Request *request = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_SEND_DECR_AT_CNT_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_DECR_AT_CNT_MSG);

    MPIDI_Pkt_init(decr_at_cnt_pkt, MPIDI_CH3_PKT_DECR_AT_COUNTER);
    decr_at_cnt_pkt->target_win_handle = win_ptr->all_win_handles[dst];

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dst, &vc);

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, decr_at_cnt_pkt,
                                    sizeof(*decr_at_cnt_pkt), &request);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    if (mpi_errno != MPI_SUCCESS) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg" );
    }

    if (request != NULL) {
        MPID_Request_release(request);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_DECR_AT_CNT_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME send_flush_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int send_flush_msg(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_FLUSH_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_FLUSH_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
    flush_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    flush_pkt->source_win_handle = win_ptr->handle;

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_exit:
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_SEND_FLUSH_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* enqueue an unsatisfied origin in passive target at target side. */
static inline int enqueue_lock_origin(MPID_Win *win_ptr, MPIDI_VC_t *vc,
                                      MPIDI_CH3_Pkt_t *pkt,
                                      MPIDI_msg_sz_t *buflen,
                                      MPID_Request **reqp)
{
    MPIDI_RMA_Lock_entry_t *new_ptr = NULL;
    MPIDI_CH3_Pkt_flags_t flag;
    MPI_Win source_win_handle;
    int lock_discarded = 0, data_discarded = 0;
    int mpi_errno = MPI_SUCCESS;

    (*reqp) = NULL;

    new_ptr = MPIDI_CH3I_Win_lock_entry_alloc(win_ptr, pkt);
    if (new_ptr != NULL) {
        MPL_LL_APPEND(win_ptr->lock_queue, win_ptr->lock_queue_tail, new_ptr);
    }
    else {
        lock_discarded = 1;
    }

    if (pkt->type == MPIDI_CH3_PKT_LOCK ||
        pkt->type == MPIDI_CH3_PKT_GET ||
        pkt->type == MPIDI_CH3_PKT_FOP ||
        pkt->type == MPIDI_CH3_PKT_CAS) {

        /* return bytes of data processed in this pkt handler */
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t);

        if (new_ptr != NULL)
            new_ptr->all_data_recved = 1;

        goto issue_ack;
    }
    else {
        MPI_Aint type_size = 0;
        MPIDI_msg_sz_t recv_data_sz = 0;
        MPID_Request *req = NULL;
        MPI_Datatype target_dtp;
        int target_count;
        int immed_len = 0;
        void *immed_data = NULL;
        int complete = 0;
        MPIDI_msg_sz_t data_len;
        char *data_buf = NULL;

        /* This is PUT, ACC, GACC */

        MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE((*pkt), target_dtp, mpi_errno);
        MPIDI_CH3_PKT_RMA_GET_TARGET_COUNT((*pkt), target_count, mpi_errno);

        MPID_Datatype_get_size_macro(target_dtp, type_size);
        recv_data_sz = type_size * target_count;

        if (recv_data_sz <= MPIDI_RMA_IMMED_BYTES) {

            /* return bytes of data processed in this pkt handler */
            (*buflen) = sizeof(MPIDI_CH3_Pkt_t);

            if (new_ptr != NULL)
                new_ptr->all_data_recved = 1;

            goto issue_ack;
        }

        if (new_ptr != NULL) {
            if (win_ptr->current_lock_data_bytes + recv_data_sz
                < MPIR_CVAR_CH3_RMA_LOCK_DATA_BYTES) {
                new_ptr->data = MPIU_Malloc(recv_data_sz);
            }

            if (new_ptr->data == NULL) {
                /* Note that there are two possible reasons to make new_ptr->data to be NULL:
                 * (1) win_ptr->current_lock_data_bytes + recv_data_sz >= MPIR_CVAR_CH3_RMA_LOCK_DATA_BYTES;
                 * (2) MPIU_Malloc(recv_data_sz) failed.
                 * In such cases, we cannot allocate memory for lock data, so we give up
                 * buffering lock data, however, we still buffer lock request.
                 */
                MPIDI_CH3_Pkt_t new_pkt;
                MPIDI_CH3_Pkt_lock_t *lock_pkt = &new_pkt.lock;
                MPI_Win target_win_handle;
                int lock_type, origin_rank;

                MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE((*pkt), target_win_handle, mpi_errno);
                MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE((*pkt), source_win_handle, mpi_errno);
                MPIDI_CH3_PKT_RMA_GET_ORIGIN_RANK((*pkt), origin_rank, mpi_errno);
                MPIDI_CH3_PKT_RMA_GET_LOCK_TYPE((*pkt), lock_type, mpi_errno);

                MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
                lock_pkt->target_win_handle = target_win_handle;
                lock_pkt->source_win_handle = source_win_handle;
                lock_pkt->lock_type = lock_type;
                lock_pkt->origin_rank = origin_rank;

                /* replace original pkt with lock pkt */
                new_ptr->pkt = new_pkt;
                new_ptr->all_data_recved = 1;

                data_discarded = 1;
            }
            else {
                win_ptr->current_lock_data_bytes += recv_data_sz;
                new_ptr->data_size = recv_data_sz;
            }
        }

        /* create request to receive upcoming requests */
        req = MPID_Request_create();
        MPIU_Object_set_ref(req, 1);

        /* fill in area in req that will be used in Receive_data_found() */
        if (lock_discarded || data_discarded) {
            req->dev.drop_data = TRUE;
            req->dev.user_buf = NULL;
            req->dev.user_count = target_count;
            req->dev.datatype = target_dtp;
            req->dev.recv_data_sz = recv_data_sz;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.OnFinal = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.lock_queue_entry = new_ptr;

            MPIDI_CH3_PKT_RMA_GET_IMMED_LEN((*pkt), immed_len, mpi_errno);
            if (immed_len > 0) {
                req->dev.recv_data_sz -= immed_len;
            }
            data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
            data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);
            MPIU_Assert(req->dev.recv_data_sz > 0);
        }
        else {
            req->dev.user_buf = new_ptr->data;
            req->dev.user_count = target_count;
            req->dev.datatype = target_dtp;
            req->dev.recv_data_sz = recv_data_sz;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.OnFinal = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.lock_queue_entry = new_ptr;

            MPIDI_CH3_PKT_RMA_GET_IMMED_LEN((*pkt), immed_len, mpi_errno);
            MPIDI_CH3_PKT_RMA_GET_IMMED_DATA_PTR((*pkt), immed_data, mpi_errno);

            if (immed_len > 0) {
                /* see if we can receive some data from packet header */
                MPIU_Memcpy(req->dev.user_buf, immed_data, (size_t)immed_len);
                req->dev.user_buf = (void*)((char*)req->dev.user_buf + immed_len);
                req->dev.recv_data_sz -= immed_len;
            }
            data_len = *buflen - sizeof(MPIDI_CH3_Pkt_t);
            data_buf = (char *) pkt + sizeof(MPIDI_CH3_Pkt_t);
            MPIU_Assert(req->dev.recv_data_sz > 0);
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data_buf, &data_len, &complete);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* return bytes of data processed in this pkt handler */
        (*buflen) = sizeof(MPIDI_CH3_Pkt_t) + data_len;

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete(vc, req, &complete);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            if (complete) {
                goto issue_ack;
            }
        }

        (*reqp) = req;
    }

 issue_ack:
    MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE((*pkt), source_win_handle, mpi_errno);
    if (pkt->type == MPIDI_CH3_PKT_LOCK) {
        if (lock_discarded) flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED;
        else flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED;

        mpi_errno = MPIDI_CH3I_Send_lock_ack_pkt(vc, win_ptr, flag, source_win_handle);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else {
        if (lock_discarded) flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED;
        else if (data_discarded) flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED;
        else flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED;

        mpi_errno = MPIDI_CH3I_Send_lock_op_ack_pkt(vc, win_ptr, flag, source_win_handle);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int handle_lock_ack(MPID_Win *win_ptr, int target_rank,
                                        MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_RMA_Target_t *t = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED ||
                win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED);

    if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_CALLED) {
        MPIDI_VC_t *orig_vc = NULL, *target_vc = NULL;
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &orig_vc);
        MPIDI_Comm_get_vc(win_ptr->comm_ptr, target_rank, &target_vc);
        if (win_ptr->comm_ptr->rank == target_rank ||
            (win_ptr->shm_allocated == TRUE && orig_vc->node_id == target_vc->node_id)) {
            if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
                win_ptr->outstanding_locks--;
                MPIU_Assert(win_ptr->outstanding_locks >= 0);
            }
            else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
                /* re-send lock request message. */
                mpi_errno = send_lock_msg(target_rank, MPI_LOCK_SHARED, win_ptr);
                if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            goto fn_exit;
        }
    }
    else if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED) {
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
            win_ptr->outstanding_locks--;
            MPIU_Assert(win_ptr->outstanding_locks >= 0);
        }
        else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
            /* re-send lock request message. */
            mpi_errno = send_lock_msg(target_rank, MPI_LOCK_SHARED, win_ptr);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        goto fn_exit;
    }

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &t);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(t != NULL);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED)
        t->access_state = MPIDI_RMA_LOCK_GRANTED;

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED)
        t->access_state = MPIDI_RMA_LOCK_CALLED;

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static inline int adjust_op_piggybacked_with_lock (MPID_Win *win_ptr,
                                                   int target_rank,
                                                   MPIDI_CH3_Pkt_flags_t flags) {
    MPIDI_RMA_Target_t *target = NULL;
    MPIDI_RMA_Op_t *op = NULL;
    MPIDI_CH3_Pkt_flags_t op_flags = MPIDI_CH3_PKT_FLAG_NONE;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &target);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    MPIU_Assert(target != NULL);

    op = target->pending_op_list;
    if (op != NULL) MPIDI_CH3_PKT_RMA_GET_FLAGS(op->pkt, op_flags, mpi_errno);

    if (op_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK) {
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED ||
            flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED) {
            if (!op->request) {
                if (op->ureq) {
                    /* Complete user request and release the ch3 ref */
                    MPID_Request_set_completed(op->ureq);
                    MPID_Request_release(op->ureq);
                }

                MPIDI_CH3I_RMA_Ops_free_elem(win_ptr, &(target->pending_op_list),
                                             &(target->pending_op_list_tail), op);
            }
            else {
                MPIDI_CH3I_RMA_Ops_unlink(&(target->pending_op_list),
                                          &(target->pending_op_list_tail), op);
                if (op->is_dt) {
                    MPIDI_CH3I_RMA_Ops_append(&(target->dt_op_list),
                                              &(target->dt_op_list_tail), op);
                }
                else if (op->pkt.type == MPIDI_CH3_PKT_PUT ||
                         op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE) {
                    MPIDI_CH3I_RMA_Ops_append(&(target->write_op_list),
                                              &(target->write_op_list_tail), op);
                }
                else {
                    MPIDI_CH3I_RMA_Ops_append(&(target->read_op_list),
                                              &(target->read_op_list_tail), op);
                }

                if (op->ureq) {
                    if (MPID_Request_is_complete(op->request)) {
                        /* Complete user request, let cleanup function to release
                           ch3 ref */
                        MPID_Request_set_completed(op->ureq);
                    }
                    else {
                        /* Increase ref for completion handler */
                        MPIU_Object_add_ref(op->ureq);
                        op->request->dev.request_handle = op->ureq->handle;
                        if (op->request->dev.OnDataAvail == NULL) {
                            op->request->dev.OnDataAvail = MPIDI_CH3_ReqHandler_ReqOpsComplete;
                        }
                        op->request->dev.OnFinal = MPIDI_CH3_ReqHandler_ReqOpsComplete;
                    }
                }
            }
        }
        else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED ||
                 flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
            /* We need to re-transmit this operation, so we destroy
               the internal request and erase all flags in current
               operation. */
            if (op->request) {
                MPIDI_CH3_Request_destroy(op->request);
                op->request = NULL;
                win_ptr->active_req_cnt--;
            }
            MPIDI_CH3_PKT_RMA_ERASE_FLAGS(op->pkt, mpi_errno);

            target->next_op_to_issue = op;
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME acquire_local_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int acquire_local_lock(MPID_Win * win_ptr, int lock_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ACQUIRE_LOCAL_LOCK);

    MPIR_T_PVAR_TIMER_START(RMA, rma_winlock_getlocallock);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 1) {
        mpi_errno = handle_lock_ack(win_ptr, win_ptr->comm_ptr->rank,
                                          MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        /* Queue the lock information. */
        MPIDI_CH3_Pkt_t pkt;
        MPIDI_CH3_Pkt_lock_t *lock_pkt = &pkt.lock;
        MPIDI_RMA_Lock_entry_t *new_ptr = NULL;

        MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
        lock_pkt->lock_type = lock_type;
        lock_pkt->origin_rank = win_ptr->comm_ptr->rank;

        new_ptr = MPIDI_CH3I_Win_lock_entry_alloc(win_ptr, &pkt);
        if (new_ptr == NULL) {
            mpi_errno = handle_lock_ack(win_ptr, win_ptr->comm_ptr->rank,
                                              MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            goto fn_exit;
        }
        MPL_LL_APPEND(win_ptr->lock_queue, win_ptr->lock_queue_tail, new_ptr);

        new_ptr->all_data_recved = 1;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_winlock_getlocallock);
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Handle_flush_ack
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Handle_flush_ack(MPID_Win * win_ptr, int target_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *t;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &t);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    t->sync.outstanding_acks--;
    MPIU_Assert(t->sync.outstanding_acks >= 0);

    t->put_acc_issued = 0; /* reset PUT_ACC_FLAG after FLUSH is completed */

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_accumulate_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int do_accumulate_op(void *source_buf, void *target_buf,
                                   int acc_count, MPI_Datatype acc_dtp, MPI_Op acc_op)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop;
    MPIDI_STATE_DECL(MPID_STATE_DO_ACCUMULATE_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_DO_ACCUMULATE_OP);

    if (acc_op == MPI_REPLACE)
    {
        /* simply copy the data */
        mpi_errno = MPIR_Localcopy(source_buf, acc_count, acc_dtp,
                                   target_buf, acc_count, acc_dtp);
        if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(acc_op) == HANDLE_KIND_BUILTIN)
    {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(acc_op);
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", acc_op );
        return mpi_errno;
	/* --END ERROR HANDLING-- */
    }

    if (MPIR_DATATYPE_IS_PREDEFINED(acc_dtp))
    {
        (*uop)(source_buf, target_buf, &acc_count, &acc_dtp);
    }
    else
    {
	/* derived datatype */
        MPID_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, count;
        MPI_Aint type_size;
        MPI_Datatype type;
        MPID_Datatype *dtp;

        segp = MPID_Segment_alloc();
	/* --BEGIN ERROR HANDLING-- */
        if (!segp)
	{
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    MPIDI_FUNC_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */
        MPID_Segment_init(NULL, acc_count,
			  acc_dtp, segp, 0);
        first = 0;
        last  = SEGMENT_IGNORE_LAST;

        MPID_Datatype_get_ptr(acc_dtp, dtp);
        vec_len = dtp->max_contig_blocks * acc_count + 1;
        /* +1 needed because Rob says so */
        dloop_vec = (DLOOP_VECTOR *)
            MPIU_Malloc(vec_len * sizeof(DLOOP_VECTOR));
	/* --BEGIN ERROR HANDLING-- */
        if (!dloop_vec)
	{
            mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0 );
	    MPIDI_FUNC_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
	/* --END ERROR HANDLING-- */

        MPID_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

        type = dtp->eltype;
        MPID_Datatype_get_size_macro(type, type_size);
        for (i=0; i<vec_len; i++)
	{
            MPIU_Assign_trunc(count, (dloop_vec[i].DLOOP_VECTOR_LEN)/type_size, int);
            (*uop)((char *)source_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                   (char *)target_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                   &count, &type);
        }

        MPID_Segment_free(segp);
        MPIU_Free(dloop_vec);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_ACCUMULATE_OP);

    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int check_piggyback_lock(MPID_Win *win_ptr, MPIDI_VC_t *vc,
                                       MPIDI_CH3_Pkt_t *pkt,
                                       MPIDI_msg_sz_t *buflen,
                                       int *acquire_lock_fail,
                                       MPID_Request **reqp) {
    int lock_type;
    MPIDI_CH3_Pkt_flags_t flags;
    int mpi_errno = MPI_SUCCESS;

    (*acquire_lock_fail) = 0;
    (*reqp) = NULL;

    MPIDI_CH3_PKT_RMA_GET_FLAGS((*pkt), flags, mpi_errno);
    MPIDI_CH3_PKT_RMA_GET_LOCK_TYPE((*pkt), lock_type, mpi_errno);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK) {
        if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) {
            /* cannot acquire the lock, queue up this operation. */
            mpi_errno = enqueue_lock_origin(win_ptr, vc, pkt, buflen, reqp);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            (*acquire_lock_fail) = 1;
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static inline int finish_op_on_target(MPID_Win *win_ptr, MPIDI_VC_t *vc,
                                      MPIDI_CH3_Pkt_type_t type,
                                      MPIDI_CH3_Pkt_flags_t flags,
                                      MPI_Win source_win_handle) {
    int mpi_errno = MPI_SUCCESS;

    if (type == MPIDI_CH3_PKT_PUT || type == MPIDI_CH3_PKT_ACCUMULATE) {
        /* This is PUT or ACC */
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK) {
            MPIDI_CH3_Pkt_flags_t pkt_flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
            if ((flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
                (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
                pkt_flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
            mpi_errno = MPIDI_CH3I_Send_lock_op_ack_pkt(vc, win_ptr,
                                                        pkt_flags,
                                                        source_win_handle);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
            if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)) {
                /* If op is piggybacked with both LOCK and FLUSH,
                   we only send LOCK ACK back, do not send FLUSH ACK. */
                mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr,
                                                          source_win_handle);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
            MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
            win_ptr->at_completion_counter--;
            MPIU_Assert(win_ptr->at_completion_counter >= 0);
            /* Signal the local process when the op counter reaches 0. */
            if (win_ptr->at_completion_counter == 0)
                MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
            if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)) {
                /* If op is piggybacked with both LOCK and UNLOCK,
                   we only send LOCK ACK back, do not send FLUSH (UNLOCK) ACK. */
                mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr,
                                                          source_win_handle);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
            mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }
    }
    else {
        /* This is GACC / GET / CAS / FOP */

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
            mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
            win_ptr->at_completion_counter--;
            MPIU_Assert(win_ptr->at_completion_counter >= 0);
            /* Signal the local process when the op counter reaches 0. */
            if (win_ptr->at_completion_counter == 0)
                MPIDI_CH3_Progress_signal_completion();
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int fill_ranks_in_win_grp(MPID_Win *win_ptr, MPID_Group *group_ptr,
                                        int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_grp;
    MPID_Group *win_grp_ptr;
    MPIU_CHKLMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIDI_RMA_FUNC_ENTER(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIU_CHKLMEM_MALLOC(ranks_in_grp, int *, group_ptr->size * sizeof(int),
                        mpi_errno, "ranks_in_grp");
    for (i = 0; i < group_ptr->size; i++) ranks_in_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, group_ptr->size,
                                                ranks_in_grp, win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_RMA_FUNC_EXIT(MPID_STATE_FILL_RANKS_IN_WIN_GRP);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int wait_progress_engine(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    mpi_errno = MPID_Progress_wait(&progress_state);
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS) {
        MPID_Progress_end(&progress_state);
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winnoprogress");
    }
    /* --END ERROR HANDLING-- */
    MPID_Progress_end(&progress_state);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int poke_progress_engine(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Progress_state progress_state;

    MPID_Progress_start(&progress_state);
    mpi_errno = MPID_Progress_poke();
    if (mpi_errno != MPI_SUCCESS)
        MPIU_ERR_POP(mpi_errno);
    MPID_Progress_end(&progress_state);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* MPID_RMA_H_INCLUDED */
