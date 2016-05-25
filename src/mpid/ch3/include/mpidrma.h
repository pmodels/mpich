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
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int send_lock_msg(int dest, int lock_type, MPIR_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_t *lock_pkt = &upkt.lock;
    MPIR_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_LOCK_MSG);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_SEND_LOCK_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
    lock_pkt->target_win_handle = win_ptr->basic_info_table[dest].win_handle;
    lock_pkt->source_win_handle = win_ptr->handle;
    lock_pkt->request_handle = MPI_REQUEST_NULL;
    lock_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (lock_type == MPI_LOCK_SHARED)
        lock_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
    else {
        MPIR_Assert(lock_type == MPI_LOCK_EXCLUSIVE);
        lock_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
    }

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_pkt, sizeof(*lock_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* release the request returned by iStartMsg */
    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_SEND_LOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME send_unlock_msg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int send_unlock_msg(int dest, MPIR_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_unlock_t *unlock_pkt = &upkt.unlock;
    MPIR_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_UNLOCK_MSG);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_SEND_UNLOCK_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    /* Send a lock packet over to the target. wait for the lock_granted
     * reply. Then do all the RMA ops. */

    MPIDI_Pkt_init(unlock_pkt, MPIDI_CH3_PKT_UNLOCK);
    unlock_pkt->target_win_handle = win_ptr->basic_info_table[dest].win_handle;
    unlock_pkt->source_win_handle = win_ptr->handle;
    unlock_pkt->flags = flags;

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, unlock_pkt, sizeof(*unlock_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_SEND_UNLOCK_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_ack_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_lock_ack_pkt(MPIDI_VC_t * vc, MPIR_Win * win_ptr,
                                               MPIDI_CH3_Pkt_flags_t flags,
                                               MPI_Win source_win_handle,
                                               MPI_Request request_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_ack_t *lock_ack_pkt = &upkt.lock_ack;
    MPIR_Request *req = NULL;
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    MPIR_Assert(!(source_win_handle != MPI_WIN_NULL && request_handle != MPI_REQUEST_NULL));

    /* send lock ack packet */
    MPIDI_Pkt_init(lock_ack_pkt, MPIDI_CH3_PKT_LOCK_ACK);
    lock_ack_pkt->source_win_handle = source_win_handle;
    lock_ack_pkt->request_handle = request_handle;
    lock_ack_pkt->target_rank = win_ptr->comm_ptr->rank;
    lock_ack_pkt->flags = flags;

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, VERBOSE,
                     (MPL_DBG_FDEST, "sending lock ack pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_ack_pkt->source_win_handle));

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_ack_pkt, sizeof(*lock_ack_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_ACK_PKT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_lock_op_ack_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_lock_op_ack_pkt(MPIDI_VC_t * vc, MPIR_Win * win_ptr,
                                                  MPIDI_CH3_Pkt_flags_t flags,
                                                  MPI_Win source_win_handle,
                                                  MPI_Request request_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_op_ack_t *lock_op_ack_pkt = &upkt.lock_op_ack;
    MPIR_Request *req = NULL;
    int mpi_errno;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);

    MPIR_Assert(!(source_win_handle != MPI_WIN_NULL && request_handle != MPI_REQUEST_NULL));

    /* send lock ack packet */
    MPIDI_Pkt_init(lock_op_ack_pkt, MPIDI_CH3_PKT_LOCK_OP_ACK);
    lock_op_ack_pkt->source_win_handle = source_win_handle;
    lock_op_ack_pkt->request_handle = request_handle;
    lock_op_ack_pkt->target_rank = win_ptr->comm_ptr->rank;
    lock_op_ack_pkt->flags = flags;

    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_OTHER, VERBOSE,
                     (MPL_DBG_FDEST, "sending lock op ack pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_op_ack_pkt->source_win_handle));

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_op_ack_pkt, sizeof(*lock_op_ack_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_OP_ACK_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Send_ack_pkt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_ack_pkt(MPIDI_VC_t * vc, MPIR_Win * win_ptr,
                                          MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_ack_t *ack_pkt = &upkt.ack;
    MPIR_Request *req;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_ACK_PKT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3I_SEND_ACK_PKT);

    MPIDI_Pkt_init(ack_pkt, MPIDI_CH3_PKT_ACK);
    ack_pkt->source_win_handle = source_win_handle;
    ack_pkt->target_rank = win_ptr->comm_ptr->rank;

    /* Because this is in a packet handler, it is already within a critical section */
    /* MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex); */
    mpi_errno = MPIDI_CH3_iStartMsg(vc, ack_pkt, sizeof(*ack_pkt), &req);
    /* MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex); */
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3I_SEND_ACK_PKT);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME send_decr_at_cnt_msg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int send_decr_at_cnt_msg(int dst, MPIR_Win * win_ptr, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_decr_at_counter_t *decr_at_cnt_pkt = &upkt.decr_at_cnt;
    MPIDI_VC_t *vc;
    MPIR_Request *request = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_DECR_AT_CNT_MSG);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_SEND_DECR_AT_CNT_MSG);

    MPIDI_Pkt_init(decr_at_cnt_pkt, MPIDI_CH3_PKT_DECR_AT_COUNTER);
    decr_at_cnt_pkt->target_win_handle = win_ptr->basic_info_table[dst].win_handle;
    decr_at_cnt_pkt->source_win_handle = win_ptr->handle;
    decr_at_cnt_pkt->flags = flags;

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dst, &vc);

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, decr_at_cnt_pkt, sizeof(*decr_at_cnt_pkt), &request);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    if (mpi_errno != MPI_SUCCESS) {
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (request != NULL) {
        MPIR_Request_free(request);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_SEND_DECR_AT_CNT_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME send_flush_msg
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int send_flush_msg(int dest, MPIR_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_flush_t *flush_pkt = &upkt.flush;
    MPIR_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_SEND_FLUSH_MSG);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_SEND_FLUSH_MSG);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(flush_pkt, MPIDI_CH3_PKT_FLUSH);
    flush_pkt->target_win_handle = win_ptr->basic_info_table[dest].win_handle;
    flush_pkt->source_win_handle = win_ptr->handle;

    MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, flush_pkt, sizeof(*flush_pkt), &req);
    MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
    MPIR_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rma_msg");

    /* Release the request returned by iStartMsg */
    if (req != NULL) {
        MPIR_Request_free(req);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_SEND_FLUSH_MSG);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


/* enqueue an unsatisfied origin in passive target at target side. */
static inline int enqueue_lock_origin(MPIR_Win * win_ptr, MPIDI_VC_t * vc,
                                      MPIDI_CH3_Pkt_t * pkt, void * data,
                                      intptr_t * buflen, MPIR_Request ** reqp)
{
    MPIDI_RMA_Target_lock_entry_t *new_ptr = NULL;
    MPIDI_CH3_Pkt_flags_t flag;
    MPI_Win source_win_handle;
    MPI_Request request_handle;
    int lock_discarded = 0, data_discarded = 0;
    int mpi_errno = MPI_SUCCESS;

    (*reqp) = NULL;

    new_ptr = MPIDI_CH3I_Win_target_lock_entry_alloc(win_ptr, pkt);
    if (new_ptr != NULL) {
        MPIDI_RMA_Target_lock_entry_t **head_ptr =
            (MPIDI_RMA_Target_lock_entry_t **) (&(win_ptr->target_lock_queue_head));
        MPL_DL_APPEND((*head_ptr), new_ptr);
        new_ptr->vc = vc;
    }
    else {
        lock_discarded = 1;
    }

    if (MPIDI_CH3I_RMA_PKT_IS_IMMED_OP(*pkt) || pkt->type == MPIDI_CH3_PKT_LOCK ||
        pkt->type == MPIDI_CH3_PKT_GET) {
        /* return bytes of data processed in this pkt handler */
        (*buflen) = 0;

        if (new_ptr != NULL)
            new_ptr->all_data_recved = 1;

        goto issue_ack;
    }
    else {
        MPI_Aint type_size = 0;
        MPI_Aint type_extent;
        intptr_t recv_data_sz = 0;
        intptr_t buf_size = 0;
        MPIR_Request *req = NULL;
        MPI_Datatype target_dtp;
        int target_count;
        int complete = 0;
        intptr_t data_len;
        MPIDI_CH3_Pkt_flags_t flags;

        /* This is PUT, ACC, GACC, FOP */

        MPIDI_CH3_PKT_RMA_GET_TARGET_DATATYPE((*pkt), target_dtp, mpi_errno);
        MPIDI_CH3_PKT_RMA_GET_TARGET_COUNT((*pkt), target_count, mpi_errno);
        MPIDI_CH3_PKT_RMA_GET_FLAGS((*pkt), flags, mpi_errno);

        MPIR_Datatype_get_extent_macro(target_dtp, type_extent);
        MPIR_Datatype_get_size_macro(target_dtp, type_size);

        if (pkt->type == MPIDI_CH3_PKT_PUT) {
            recv_data_sz = type_size * target_count;
            buf_size = type_extent * target_count;
        }
        else {
            MPI_Aint stream_elem_count;
            MPI_Aint total_len;
            MPI_Op op;

            MPIDI_CH3_PKT_RMA_GET_OP((*pkt), op, mpi_errno);
            if (op != MPI_NO_OP) {
                stream_elem_count = MPIDI_CH3U_SRBuf_size / type_extent;
                total_len = type_size * target_count;
                recv_data_sz = MPL_MIN(total_len, type_size * stream_elem_count);
                buf_size = type_extent * (recv_data_sz / type_size);
            }
        }

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
            MPIR_Assert(pkt->type == MPIDI_CH3_PKT_ACCUMULATE ||
                        pkt->type == MPIDI_CH3_PKT_GET_ACCUM);

            /* Only basic datatype may contain piggyback lock.
             * Thus we do not check extended header type for derived case.*/
            if (pkt->type == MPIDI_CH3_PKT_ACCUMULATE) {
                recv_data_sz += sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);
                buf_size += sizeof(MPIDI_CH3_Ext_pkt_accum_stream_t);
            }
            else {
                recv_data_sz += sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t);
                buf_size += sizeof(MPIDI_CH3_Ext_pkt_get_accum_stream_t);
            }
        }

        if (new_ptr != NULL) {
            if (win_ptr->current_target_lock_data_bytes + buf_size <
                MPIR_CVAR_CH3_RMA_TARGET_LOCK_DATA_BYTES) {
                new_ptr->data = MPL_malloc(buf_size);
            }

            if (new_ptr->data == NULL) {
                /* Note that there are two possible reasons to make new_ptr->data to be NULL:
                 * (1) win_ptr->current_target_lock_data_bytes + buf_size >= MPIR_CVAR_CH3_RMA_TARGET_LOCK_DATA_BYTES;
                 * (2) MPL_malloc(buf_size) failed.
                 * In such cases, we cannot allocate memory for lock data, so we give up
                 * buffering lock data, however, we still buffer lock request.
                 */
                MPIDI_CH3_Pkt_t new_pkt;
                MPIDI_CH3_Pkt_lock_t *lock_pkt = &new_pkt.lock;
                MPI_Win target_win_handle;

                MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE((*pkt), target_win_handle, mpi_errno);

                if (!MPIDI_CH3I_RMA_PKT_IS_READ_OP(*pkt)) {
                    MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE((*pkt), source_win_handle, mpi_errno);
                    request_handle = MPI_REQUEST_NULL;
                }
                else {
                    source_win_handle = MPI_WIN_NULL;
                    MPIDI_CH3_PKT_RMA_GET_REQUEST_HANDLE((*pkt), request_handle, mpi_errno);
                }

                MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
                lock_pkt->target_win_handle = target_win_handle;
                lock_pkt->source_win_handle = source_win_handle;
                lock_pkt->request_handle = request_handle;
                lock_pkt->flags = flags;

                /* replace original pkt with lock pkt */
                new_ptr->pkt = new_pkt;
                new_ptr->all_data_recved = 1;

                data_discarded = 1;
            }
            else {
                win_ptr->current_target_lock_data_bytes += buf_size;
                new_ptr->buf_size = buf_size;
            }
        }

        /* create request to receive upcoming requests */
        req = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
        MPIR_Object_set_ref(req, 1);

        /* fill in area in req that will be used in Receive_data_found() */
        if (lock_discarded || data_discarded) {
            req->dev.drop_data = TRUE;
            req->dev.user_buf = NULL;
            req->dev.user_count = target_count;
            req->dev.datatype = target_dtp;
            req->dev.recv_data_sz = recv_data_sz;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.OnFinal = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.target_lock_queue_entry = new_ptr;

            data_len = *buflen;
            MPIR_Assert(req->dev.recv_data_sz >= 0);
        }
        else {
            req->dev.user_buf = new_ptr->data;
            req->dev.user_count = target_count;
            req->dev.datatype = target_dtp;
            req->dev.recv_data_sz = recv_data_sz;
            req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.OnFinal = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete;
            req->dev.target_lock_queue_entry = new_ptr;

            data_len = *buflen;
            MPIR_Assert(req->dev.recv_data_sz >= 0);
        }

        mpi_errno = MPIDI_CH3U_Receive_data_found(req, data, &data_len, &complete);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);

        /* return bytes of data processed in this pkt handler */
        (*buflen) = data_len;

        if (complete) {
            mpi_errno = MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete(vc, req, &complete);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            if (complete) {
                goto issue_ack;
            }
        }

        (*reqp) = req;
    }

  issue_ack:
    if (pkt->type == MPIDI_CH3_PKT_LOCK) {
        if (lock_discarded)
            flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED;
        else
            flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED;

        MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE((*pkt), source_win_handle, mpi_errno);
        MPIDI_CH3_PKT_RMA_GET_REQUEST_HANDLE((*pkt), request_handle, mpi_errno);

        mpi_errno =
            MPIDI_CH3I_Send_lock_ack_pkt(vc, win_ptr, flag, source_win_handle, request_handle);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        if (lock_discarded)
            flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED;
        else if (data_discarded)
            flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED;
        else
            flag = MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_QUEUED;

        if (!MPIDI_CH3I_RMA_PKT_IS_READ_OP(*pkt)) {
            MPIDI_CH3_PKT_RMA_GET_SOURCE_WIN_HANDLE((*pkt), source_win_handle, mpi_errno);
            request_handle = MPI_REQUEST_NULL;
        }
        else {
            source_win_handle = MPI_WIN_NULL;
            MPIDI_CH3_PKT_RMA_GET_REQUEST_HANDLE((*pkt), request_handle, mpi_errno);
        }

        mpi_errno =
            MPIDI_CH3I_Send_lock_op_ack_pkt(vc, win_ptr, flag, source_win_handle, request_handle);
        if (mpi_errno != MPI_SUCCESS)
            MPIR_ERR_POP(mpi_errno);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int handle_lock_ack(MPIR_Win * win_ptr, int target_rank, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_RMA_Target_t *t = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(win_ptr->states.access_state == MPIDI_RMA_PER_TARGET ||
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
                MPIR_Assert(win_ptr->outstanding_locks >= 0);
            }
            else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
                /* re-send lock request message. */
                mpi_errno = send_lock_msg(target_rank, MPI_LOCK_SHARED, win_ptr);
                if (mpi_errno != MPI_SUCCESS)
                    MPIR_ERR_POP(mpi_errno);
            }
            goto fn_exit;
        }
    }
    else if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_ISSUED) {
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
            win_ptr->outstanding_locks--;
            MPIR_Assert(win_ptr->outstanding_locks >= 0);
            if (win_ptr->outstanding_locks == 0) {
                win_ptr->states.access_state = MPIDI_RMA_LOCK_ALL_GRANTED;

                if (win_ptr->num_targets_with_pending_net_ops) {
                    mpi_errno = MPIDI_CH3I_Win_set_active(win_ptr);
                    if (mpi_errno != MPI_SUCCESS) {
                        MPIR_ERR_POP(mpi_errno);
                    }
                }
            }
        }
        else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
            /* re-send lock request message. */
            mpi_errno = send_lock_msg(target_rank, MPI_LOCK_SHARED, win_ptr);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
        }
        goto fn_exit;
    }

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &t);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    MPIR_Assert(t != NULL);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {
        t->access_state = MPIDI_RMA_LOCK_GRANTED;
        if (t->pending_net_ops_list_head)
            MPIDI_CH3I_Win_set_active(win_ptr);
    }

    if (win_ptr->states.access_state == MPIDI_RMA_LOCK_ALL_GRANTED ||
        t->access_state == MPIDI_RMA_LOCK_GRANTED) {
        if (t->pending_net_ops_list_head == NULL) {
            int made_progress ATTRIBUTE((unused)) = 0;
            mpi_errno =
                MPIDI_CH3I_RMA_Make_progress_target(win_ptr, t->target_rank, &made_progress);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
    }

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED)
        t->access_state = MPIDI_RMA_LOCK_CALLED;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME check_and_set_req_completion
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int check_and_set_req_completion(MPIR_Win * win_ptr, MPIDI_RMA_Target_t * target,
                                               MPIDI_RMA_Op_t * rma_op, int *op_completed)
{
    int i, mpi_errno = MPI_SUCCESS;
    int incomplete_req_cnt = 0;
    MPIR_Request **req = NULL;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_CHECK_AND_SET_REQ_COMPLETION);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_CHECK_AND_SET_REQ_COMPLETION);

    (*op_completed) = FALSE;

    for (i = 0; i < rma_op->reqs_size; i++) {
        if (rma_op->reqs_size == 1)
            req = &(rma_op->single_req);
        else
            req = &(rma_op->multi_reqs[i]);

        if ((*req) == NULL)
            continue;

        if (MPIR_Request_is_complete((*req))) {
            MPIR_Request_free((*req));
            (*req) = NULL;
        }
        else {
            (*req)->dev.request_completed_cb = MPIDI_CH3_Req_handler_rma_op_complete;
            (*req)->dev.source_win_handle = win_ptr->handle;
            (*req)->dev.rma_target_ptr = target;

            incomplete_req_cnt++;

            if (rma_op->ureq != NULL) {
                MPIR_cc_set(&(rma_op->ureq->cc), incomplete_req_cnt);
                (*req)->dev.request_handle = rma_op->ureq->handle;
            }

            MPIR_Request_free((*req));
        }
    }

    if (incomplete_req_cnt == 0) {
        if (rma_op->ureq != NULL) {
            mpi_errno = MPID_Request_complete(rma_op->ureq);
            if (mpi_errno != MPI_SUCCESS) {
                MPIR_ERR_POP(mpi_errno);
            }
        }
        (*op_completed) = TRUE;
    }
    else {
        MPIDI_CH3I_RMA_Active_req_cnt += incomplete_req_cnt;
        target->num_pkts_wait_for_local_completion += incomplete_req_cnt;
    }

    MPIDI_CH3I_RMA_Ops_free_elem(win_ptr, &(target->pending_net_ops_list_head), rma_op);

    if (target->pending_net_ops_list_head == NULL) {
        win_ptr->num_targets_with_pending_net_ops--;
        MPIR_Assert(win_ptr->num_targets_with_pending_net_ops >= 0);
        if (win_ptr->num_targets_with_pending_net_ops == 0) {
            MPIDI_CH3I_Win_set_inactive(win_ptr);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_CHECK_AND_SET_REQ_COMPLETION);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int handle_lock_ack_with_op(MPIR_Win * win_ptr,
                                          int target_rank, MPIDI_CH3_Pkt_flags_t flags)
{
    MPIDI_RMA_Target_t *target = NULL;
    MPIDI_RMA_Op_t *op = NULL;
    MPIDI_CH3_Pkt_flags_t op_flags = MPIDI_CH3_PKT_FLAG_NONE;
    int op_completed ATTRIBUTE((unused)) = FALSE;
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &target);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);
    MPIR_Assert(target != NULL);

    /* Here the next_op_to_issue pointer should still point to the OP piggybacked
     * with LOCK */
    op = target->next_op_to_issue;
    MPIR_Assert(op != NULL);

    MPIDI_CH3_PKT_RMA_GET_FLAGS(op->pkt, op_flags, mpi_errno);
    MPIR_Assert(op_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
                op_flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE);

    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED) {

        if ((op->pkt.type == MPIDI_CH3_PKT_ACCUMULATE || op->pkt.type == MPIDI_CH3_PKT_GET_ACCUM)
            && op->issued_stream_count != ALL_STREAM_UNITS_ISSUED) {
            /* Now we successfully issue out the first stream unit,
             * keep next_op_to_issue still stick to the current op
             * since we need to issue the following stream units. */
            goto fn_exit;
        }

        /* We are done with the current operation, make next_op_to_issue points to the
         * next operation. */
        target->next_op_to_issue = op->next;

        if (target->next_op_to_issue == NULL) {
            if (((target->sync.sync_flag == MPIDI_RMA_SYNC_FLUSH) &&
                 (op_flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)) ||
                ((target->sync.sync_flag == MPIDI_RMA_SYNC_UNLOCK) &&
                 (op_flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))) {
                /* We are done with ending sync, unset target's sync_flag. */
                target->sync.sync_flag = MPIDI_RMA_SYNC_NONE;
            }
        }

        check_and_set_req_completion(win_ptr, target, op, &op_completed);
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_QUEUED_DATA_DISCARDED ||
             flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED) {
        /* We need to re-transmit this operation, so we destroy
         * the internal request and erase all flags in current
         * operation. */
        if (op->reqs_size == 1) {
            MPIR_Assert(op->single_req != NULL);
            MPIR_Request_free(op->single_req);
            op->single_req = NULL;
            op->reqs_size = 0;
        }
        else if (op->reqs_size > 1) {
            MPIR_Assert(op->multi_reqs != NULL && op->multi_reqs[0] != NULL);
            MPIR_Request_free(op->multi_reqs[0]);
            /* free req array in this op */
            MPL_free(op->multi_reqs);
            op->multi_reqs = NULL;
            op->reqs_size = 0;
        }
        MPIDI_CH3_PKT_RMA_ERASE_FLAGS(op->pkt, mpi_errno);

        op->issued_stream_count = 0;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME acquire_local_lock
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int acquire_local_lock(MPIR_Win * win_ptr, int lock_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_ACQUIRE_LOCAL_LOCK);

    MPIR_T_PVAR_TIMER_START(RMA, rma_winlock_getlocallock);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 1) {
        mpi_errno = handle_lock_ack(win_ptr, win_ptr->comm_ptr->rank,
                                    MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }
    else {
        /* Queue the lock information. */
        MPIDI_CH3_Pkt_t pkt;
        MPIDI_CH3_Pkt_lock_t *lock_pkt = &pkt.lock;
        MPIDI_RMA_Target_lock_entry_t *new_ptr = NULL;
        MPIDI_VC_t *my_vc;
        MPIDI_RMA_Target_lock_entry_t **head_ptr =
            (MPIDI_RMA_Target_lock_entry_t **) (&(win_ptr->target_lock_queue_head));

        MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
        lock_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
        if (lock_type == MPI_LOCK_SHARED)
            lock_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED;
        else {
            MPIR_Assert(lock_type == MPI_LOCK_EXCLUSIVE);
            lock_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE;
        }

        new_ptr = MPIDI_CH3I_Win_target_lock_entry_alloc(win_ptr, &pkt);
        if (new_ptr == NULL) {
            mpi_errno = handle_lock_ack(win_ptr, win_ptr->comm_ptr->rank,
                                        MPIDI_CH3_PKT_FLAG_RMA_LOCK_DISCARDED);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            goto fn_exit;
        }
        MPL_DL_APPEND((*head_ptr), new_ptr);
        MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, win_ptr->comm_ptr->rank, &my_vc);
        new_ptr->vc = my_vc;

        new_ptr->all_data_recved = 1;
    }

  fn_exit:
    MPIR_T_PVAR_TIMER_END(RMA, rma_winlock_getlocallock);
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    return mpi_errno;
    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_RMA_Handle_ack
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_RMA_Handle_ack(MPIR_Win * win_ptr, int target_rank)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_RMA_Target_t *t;

    mpi_errno = MPIDI_CH3I_Win_find_target(win_ptr, target_rank, &t);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    t->sync.outstanding_acks--;
    MPIR_Assert(t->sync.outstanding_acks >= 0);

    win_ptr->outstanding_acks--;
    MPIR_Assert(win_ptr->outstanding_acks >= 0);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_accumulate_op
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static inline int do_accumulate_op(void *source_buf, int source_count, MPI_Datatype source_dtp,
                                   void *target_buf, int target_count, MPI_Datatype target_dtp,
                                   MPI_Aint stream_offset, MPI_Op acc_op)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop = NULL;
    MPI_Aint source_dtp_size = 0, source_dtp_extent = 0;
    int is_empty_source = FALSE;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_DO_ACCUMULATE_OP);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_DO_ACCUMULATE_OP);

    /* first Judge if source buffer is empty */
    if (acc_op == MPI_NO_OP)
        is_empty_source = TRUE;

    if (is_empty_source == FALSE) {
        MPIR_Assert(MPIR_DATATYPE_IS_PREDEFINED(source_dtp));
        MPIR_Datatype_get_size_macro(source_dtp, source_dtp_size);
        MPIR_Datatype_get_extent_macro(source_dtp, source_dtp_extent);
    }

    if (HANDLE_GET_KIND(acc_op) == HANDLE_KIND_BUILTIN) {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(acc_op);
    }
    else {
        /* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OP,
                                         "**opnotpredefined", "**opnotpredefined %d", acc_op);
        return mpi_errno;
        /* --END ERROR HANDLING-- */
    }


    if (is_empty_source == TRUE || MPIR_DATATYPE_IS_PREDEFINED(target_dtp)) {
        /* directly apply op if target dtp is predefined dtp OR source buffer is empty */
        MPI_Aint real_stream_offset;
        void *curr_target_buf;

        if (is_empty_source == FALSE) {
            MPIR_Assert(source_dtp == target_dtp);
            real_stream_offset = (stream_offset / source_dtp_size) * source_dtp_extent;
            curr_target_buf = (void *) ((char *) target_buf + real_stream_offset);
        }
        else {
            curr_target_buf = target_buf;
        }

        (*uop) (source_buf, curr_target_buf, &source_count, &source_dtp);
    }
    else {
        /* derived datatype */
        MPIR_Segment *segp;
        DLOOP_VECTOR *dloop_vec;
        MPI_Aint first, last;
        int vec_len, i, count;
        MPI_Aint type_extent, type_size;
        MPI_Datatype type;
        MPIR_Datatype*dtp;
        MPI_Aint curr_len;
        void *curr_loc;
        int accumulated_count;

        segp = MPIR_Segment_alloc();
        /* --BEGIN ERROR HANDLING-- */
        if (!segp) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */
        MPIR_Segment_init(NULL, target_count, target_dtp, segp, 0);
        first = stream_offset;
        last = first + source_count * source_dtp_size;

        MPIR_Datatype_get_ptr(target_dtp, dtp);
        vec_len = dtp->max_contig_blocks * target_count + 1;
        /* +1 needed because Rob says so */
        dloop_vec = (DLOOP_VECTOR *)
            MPL_malloc(vec_len * sizeof(DLOOP_VECTOR));
        /* --BEGIN ERROR HANDLING-- */
        if (!dloop_vec) {
            mpi_errno =
                MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__,
                                     MPI_ERR_OTHER, "**nomem", 0);
            MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);
            return mpi_errno;
        }
        /* --END ERROR HANDLING-- */

        MPIR_Segment_pack_vector(segp, first, &last, dloop_vec, &vec_len);

        type = dtp->basic_type;
        MPIR_Assert(type != MPI_DATATYPE_NULL);

        MPIR_Assert(type == source_dtp);
        type_size = source_dtp_size;
        type_extent = source_dtp_extent;

        i = 0;
        curr_loc = dloop_vec[0].DLOOP_VECTOR_BUF;
        curr_len = dloop_vec[0].DLOOP_VECTOR_LEN;
        accumulated_count = 0;
        while (i != vec_len) {
            if (curr_len < type_size) {
                MPIR_Assert(i != vec_len);
                i++;
                curr_len += dloop_vec[i].DLOOP_VECTOR_LEN;
                continue;
            }

            MPIR_Assign_trunc(count, curr_len / type_size, int);

            (*uop) ((char *) source_buf + type_extent * accumulated_count,
                    (char *) target_buf + MPIR_Ptr_to_aint(curr_loc), &count, &type);

            if (curr_len % type_size == 0) {
                i++;
                if (i != vec_len) {
                    curr_loc = dloop_vec[i].DLOOP_VECTOR_BUF;
                    curr_len = dloop_vec[i].DLOOP_VECTOR_LEN;
                }
            }
            else {
                curr_loc = (void *) ((char *) curr_loc + type_extent * count);
                curr_len -= type_size * count;
            }

            accumulated_count += count;
        }

        MPIR_Segment_free(segp);
        MPL_free(dloop_vec);
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_DO_ACCUMULATE_OP);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


static inline int check_piggyback_lock(MPIR_Win * win_ptr, MPIDI_VC_t * vc,
                                       MPIDI_CH3_Pkt_t * pkt, void * data,
                                       intptr_t * buflen,
                                       int *acquire_lock_fail, MPIR_Request ** reqp)
{
    int lock_type;
    MPIDI_CH3_Pkt_flags_t flags;
    int mpi_errno = MPI_SUCCESS;

    (*acquire_lock_fail) = 0;
    (*reqp) = NULL;

    MPIDI_CH3_PKT_RMA_GET_FLAGS((*pkt), flags, mpi_errno);
    if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED || flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED)
            lock_type = MPI_LOCK_SHARED;
        else {
            MPIR_Assert(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE);
            lock_type = MPI_LOCK_EXCLUSIVE;
        }

        if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) {
            /* cannot acquire the lock, queue up this operation. */
            mpi_errno = enqueue_lock_origin(win_ptr, vc, pkt, data, buflen, reqp);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            (*acquire_lock_fail) = 1;
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline int finish_op_on_target(MPIR_Win * win_ptr, MPIDI_VC_t * vc,
                                      int has_response_data,
                                      MPIDI_CH3_Pkt_flags_t flags, MPI_Win source_win_handle)
{
    int mpi_errno = MPI_SUCCESS;

    if (!has_response_data) {
        /* This is PUT or ACC */
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
            flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE) {
            MPIDI_CH3_Pkt_flags_t pkt_flags = MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
            if ((flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) || (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
                pkt_flags |= MPIDI_CH3_PKT_FLAG_RMA_ACK;
            MPIR_Assert(source_win_handle != MPI_WIN_NULL);
            mpi_errno = MPIDI_CH3I_Send_lock_op_ack_pkt(vc, win_ptr,
                                                        pkt_flags,
                                                        source_win_handle, MPI_REQUEST_NULL);
            if (mpi_errno != MPI_SUCCESS)
                MPIR_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
            if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
                  flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)) {
                /* If op is piggybacked with both LOCK and FLUSH,
                 * we only send LOCK ACK back, do not send FLUSH ACK. */
                mpi_errno = MPIDI_CH3I_Send_ack_pkt(vc, win_ptr, source_win_handle);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
            MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
            win_ptr->at_completion_counter--;
            MPIR_Assert(win_ptr->at_completion_counter >= 0);
            /* Signal the local process when the op counter reaches 0. */
            if (win_ptr->at_completion_counter == 0)
                MPIDI_CH3_Progress_signal_completion();
        }
        if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
            if (!(flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_SHARED ||
                  flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK_EXCLUSIVE)) {
                /* If op is piggybacked with both LOCK and UNLOCK,
                 * we only send LOCK ACK back, do not send FLUSH (UNLOCK) ACK. */
                mpi_errno = MPIDI_CH3I_Send_ack_pkt(vc, win_ptr, source_win_handle);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
            }
            mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }
    }
    else {
        /* This is GACC / GET / CAS / FOP */

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK) {
            mpi_errno = MPIDI_CH3I_Release_lock(win_ptr);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIDI_CH3_Progress_signal_completion();
        }

        if (flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
            win_ptr->at_completion_counter--;
            MPIR_Assert(win_ptr->at_completion_counter >= 0);
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


static inline int fill_ranks_in_win_grp(MPIR_Win * win_ptr, MPIR_Group * group_ptr,
                                        int *ranks_in_win_grp)
{
    int mpi_errno = MPI_SUCCESS;
    int i, *ranks_in_grp;
    MPIR_Group *win_grp_ptr;
    MPIR_CHKLMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIR_FUNC_VERBOSE_RMA_ENTER(MPID_STATE_FILL_RANKS_IN_WIN_GRP);

    MPIR_CHKLMEM_MALLOC(ranks_in_grp, int *, group_ptr->size * sizeof(int),
                        mpi_errno, "ranks_in_grp");
    for (i = 0; i < group_ptr->size; i++)
        ranks_in_grp[i] = i;

    mpi_errno = MPIR_Comm_group_impl(win_ptr->comm_ptr, &win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_translate_ranks_impl(group_ptr, group_ptr->size,
                                                ranks_in_grp, win_grp_ptr, ranks_in_win_grp);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

    mpi_errno = MPIR_Group_free_impl(win_grp_ptr);
    if (mpi_errno != MPI_SUCCESS)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_RMA_EXIT(MPID_STATE_FILL_RANKS_IN_WIN_GRP);
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
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winnoprogress");
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
        MPIR_ERR_POP(mpi_errno);
    MPID_Progress_end(&progress_state);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

static inline void MPIDI_CH3_ExtPkt_Accum_get_stream(MPIDI_CH3_Pkt_flags_t flags,
                                                     int is_derived_dt, void *ext_hdr_ptr,
                                                     MPI_Aint * stream_offset)
{
    if ((flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) && is_derived_dt) {
        MPIR_Assert(ext_hdr_ptr != NULL);
        (*stream_offset) =
            ((MPIDI_CH3_Ext_pkt_accum_stream_derived_t *) ext_hdr_ptr)->stream_offset;
    }
    else if (flags & MPIDI_CH3_PKT_FLAG_RMA_STREAM) {
        MPIR_Assert(ext_hdr_ptr != NULL);
        (*stream_offset) = ((MPIDI_CH3_Ext_pkt_accum_stream_t *) ext_hdr_ptr)->stream_offset;
    }
}

static inline void MPIDI_CH3_ExtPkt_Gaccum_get_stream(MPIDI_CH3_Pkt_flags_t flags,
                                                      int is_derived_dt, void *ext_hdr_ptr,
                                                      MPI_Aint * stream_offset)
{
    /* We do not check packet match here, because error must have already been
     * reported at header init time (on origin) and at packet receive time (on target).  */
    MPIDI_CH3_ExtPkt_Accum_get_stream(flags, is_derived_dt, ext_hdr_ptr, stream_offset);
}

#endif /* MPID_RMA_H_INCLUDED */
