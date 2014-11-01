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

int MPIDI_CH3I_Issue_rma_op(MPIDI_RMA_Op_t * op_ptr, MPID_Win * win_ptr,
                            MPIDI_CH3_Pkt_flags_t flags);

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

    MPIU_Assert(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_CALLED);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    MPIDI_Pkt_init(lock_pkt, MPIDI_CH3_PKT_LOCK);
    lock_pkt->target_win_handle = win_ptr->all_win_handles[dest];
    lock_pkt->source_win_handle = win_ptr->handle;
    lock_pkt->lock_type = lock_type;
    lock_pkt->origin_rank = win_ptr->comm_ptr->rank;

    win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_REQUESTED;
    win_ptr->targets[dest].remote_lock_mode = lock_type;

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
static inline int send_unlock_msg(int dest, MPID_Win * win_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_unlock_t *unlock_pkt = &upkt.unlock;
    MPID_Request *req = NULL;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_SEND_UNLOCK_MSG);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_SEND_UNLOCK_MSG);

    MPIU_Assert(win_ptr->targets[dest].remote_lock_state == MPIDI_CH3_WIN_LOCK_GRANTED);

    MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr, dest, &vc);

    /* Send a lock packet over to the target. wait for the lock_granted
     * reply. Then do all the RMA ops. */

    MPIDI_Pkt_init(unlock_pkt, MPIDI_CH3_PKT_UNLOCK);
    unlock_pkt->target_win_handle = win_ptr->all_win_handles[dest];

    /* Reset the local state of the target to unlocked */
    win_ptr->targets[dest].remote_lock_state = MPIDI_CH3_WIN_LOCK_NONE;

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
#define FUNCNAME MPIDI_CH3I_Send_lock_granted_pkt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPIDI_CH3I_Send_lock_granted_pkt(MPIDI_VC_t * vc, MPID_Win * win_ptr, MPI_Win source_win_handle)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_lock_granted_t *lock_granted_pkt = &upkt.lock_granted;
    MPID_Request *req = NULL;
    int mpi_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

    /* send lock granted packet */
    MPIDI_Pkt_init(lock_granted_pkt, MPIDI_CH3_PKT_LOCK_GRANTED);
    lock_granted_pkt->source_win_handle = source_win_handle;
    lock_granted_pkt->target_rank = win_ptr->comm_ptr->rank;

    MPIU_DBG_MSG_FMT(CH3_OTHER, VERBOSE,
                     (MPIU_DBG_FDEST, "sending lock granted pkt on vc=%p, source_win_handle=%#08x",
                      vc, lock_granted_pkt->source_win_handle));

    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, lock_granted_pkt, sizeof(*lock_granted_pkt), &req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);
    if (mpi_errno) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    if (req != NULL) {
        MPID_Request_release(req);
    }

  fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_SEND_LOCK_GRANTED_PKT);

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
#define FUNCNAME acquire_local_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int acquire_local_lock(MPID_Win * win_ptr, int lock_type)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_ACQUIRE_LOCAL_LOCK);
    MPIDI_RMA_FUNC_ENTER(MPID_STATE_ACQUIRE_LOCAL_LOCK);

    /* poke the progress engine until the local lock is granted */
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) {
        MPID_Progress_state progress_state;

        MPID_Progress_start(&progress_state);
        while (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, lock_type) == 0) {
            mpi_errno = MPID_Progress_wait(&progress_state);
            /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS) {
                MPID_Progress_end(&progress_state);
                MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**winnoprogress");
            }
            /* --END ERROR HANDLING-- */
        }
        MPID_Progress_end(&progress_state);
    }

    win_ptr->targets[win_ptr->comm_ptr->rank].remote_lock_state = MPIDI_CH3_WIN_LOCK_GRANTED;
    win_ptr->targets[win_ptr->comm_ptr->rank].remote_lock_mode = lock_type;

  fn_exit:
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

    if (t == NULL) {
        win_ptr->outstanding_unlocks--;
        MPIU_Assert(win_ptr->outstanding_unlocks >= 0);
    }
    else {
        t->sync.outstanding_acks--;
        MPIU_Assert(t->sync.outstanding_acks >= 0);
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME do_accumulate_op
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int do_accumulate_op(MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint true_lb, true_extent;
    MPI_User_function *uop;
    MPIDI_STATE_DECL(MPID_STATE_DO_ACCUMULATE_OP);

    MPIDI_FUNC_ENTER(MPID_STATE_DO_ACCUMULATE_OP);

    MPIU_Assert(rreq->dev.final_user_buf != NULL);

    if (rreq->dev.op == MPI_REPLACE)
    {
        /* simply copy the data */
        mpi_errno = MPIR_Localcopy(rreq->dev.final_user_buf, rreq->dev.user_count,
                                   rreq->dev.datatype,
                                   rreq->dev.real_user_buf,
                                   rreq->dev.user_count,
                                   rreq->dev.datatype);
        if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(rreq->dev.op) == HANDLE_KIND_BUILTIN)
    {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(rreq->dev.op);
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", rreq->dev.op );
        return mpi_errno;
	/* --END ERROR HANDLING-- */
    }

    if (MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype))
    {
        (*uop)(rreq->dev.final_user_buf, rreq->dev.real_user_buf,
               &(rreq->dev.user_count), &(rreq->dev.datatype));
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
        MPID_Segment_init(NULL, rreq->dev.user_count,
			  rreq->dev.datatype, segp, 0);
        first = 0;
        last  = SEGMENT_IGNORE_LAST;

        MPID_Datatype_get_ptr(rreq->dev.datatype, dtp);
        vec_len = dtp->max_contig_blocks * rreq->dev.user_count + 1;
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
            (*uop)((char *)rreq->dev.final_user_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                   (char *)rreq->dev.real_user_buf + MPIU_PtrToAint(dloop_vec[i].DLOOP_VECTOR_BUF),
                   &count, &type);
        }

        MPID_Segment_free(segp);
        MPIU_Free(dloop_vec);
    }

 fn_exit:
    /* free the temporary buffer */
    MPIR_Type_get_true_extent_impl(rreq->dev.datatype, &true_lb, &true_extent);
    MPIU_Free((char *) rreq->dev.final_user_buf + true_lb);

    MPIDI_FUNC_EXIT(MPID_STATE_DO_ACCUMULATE_OP);

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
