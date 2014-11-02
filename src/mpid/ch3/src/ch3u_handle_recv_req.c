/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

static int create_derived_datatype(MPID_Request * rreq, MPID_Datatype ** dtp);
static int do_simple_accumulate(MPIDI_PT_single_op *single_op);
static int do_simple_get(MPID_Win *win_ptr, MPIDI_Win_lock_queue *lock_queue);

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3U_Handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3U_Handle_recv_req(MPIDI_VC_t * vc, MPID_Request * rreq, 
			       int * complete)
{
    static int in_routine ATTRIBUTE((unused)) = FALSE;
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);

    MPIU_Assert(in_routine == FALSE);
    in_routine = TRUE;

    reqFn = rreq->dev.OnDataAvail;
    if (!reqFn) {
	MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_RECV);
	MPIDI_CH3U_Request_complete(rreq);
	*complete = TRUE;
    }
    else {
        mpi_errno = reqFn( vc, rreq, complete );
    }

    in_routine = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3U_HANDLE_RECV_REQ);
    return mpi_errno;
}

/* ----------------------------------------------------------------------- */
/* Here are the functions that implement the actions that are taken when 
 * data is available for a receive request (or other completion operations)
 * These include "receive" requests that are part of the RMA implementation.
 *
 * The convention for the names of routines that are called when data is
 * available is
 *    MPIDI_CH3_ReqHandler_<type>( MPIDI_VC_t *, MPID_Request *, int * )
 * as in 
 *    MPIDI_CH3_ReqHandler_...
 *
 * ToDo: 
 *    We need a way for each of these functions to describe what they are,
 *    so that given a pointer to one of these functions, we can retrieve
 *    a description of the routine.  We may want to use a static string 
 *    and require the user to maintain thread-safety, at least while
 *    accessing the string.
 */
/* ----------------------------------------------------------------------- */
int MPIDI_CH3_ReqHandler_RecvComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
				       MPID_Request *rreq, 
				       int *complete )
{
    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_PutRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PutRecvComplete( MPIDI_VC_t *vc,
                                          MPID_Request *rreq,
                                          int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
        mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr, rreq->dev.source_win_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIDI_CH3_Progress_signal_completion();
    }
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
        win_ptr->at_completion_counter--;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);
        /* Signal the local process when the op counter reaches 0. */
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(vc, win_ptr, TRUE, rreq->dev.flags,
                                               rreq->dev.source_win_handle);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);
    return MPI_SUCCESS;

    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_AccumRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_AccumRecvComplete( MPIDI_VC_t *vc,
                                            MPID_Request *rreq,
                                            int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* accumulate data from tmp_buf into user_buf */
    mpi_errno = do_accumulate_op(rreq);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) {
        mpi_errno = MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr, rreq->dev.source_win_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPIDI_CH3_Progress_signal_completion();
    }
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_DECR_AT_COUNTER) {
        win_ptr->at_completion_counter--;
        MPIU_Assert(win_ptr->at_completion_counter >= 0);
        /* Signal the local process when the op counter reaches 0. */
        if (win_ptr->at_completion_counter == 0)
            MPIDI_CH3_Progress_signal_completion();
    }

    mpi_errno = MPIDI_CH3_Finish_rma_op_target(vc, win_ptr, TRUE, rreq->dev.flags,
                                               rreq->dev.source_win_handle);
    if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }

    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);
    return MPI_SUCCESS;

    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GaccumRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GaccumRecvComplete( MPIDI_VC_t *vc,
                                             MPID_Request *rreq,
                                             int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPI_Aint type_size;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &upkt.get_accum_resp;
    MPID_Request *resp_req;
    MPID_IOV iov[MPID_IOV_LIMIT];
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP);
    get_accum_resp_pkt->request_handle = rreq->dev.resp_request_handle;
    get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_accum_resp_pkt->source_win_handle = rreq->dev.source_win_handle;
    get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;

    MPID_Datatype_get_size_macro(rreq->dev.datatype, type_size);

    /* Copy data into a temporary buffer */
    resp_req = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(resp_req == NULL, mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    MPIU_Object_set_ref(resp_req, 1);

    MPIU_CHKPMEM_MALLOC(resp_req->dev.user_buf, void *, rreq->dev.user_count * type_size,
                        mpi_errno, "GACC resp. buffer");

    if (MPIR_DATATYPE_IS_PREDEFINED(rreq->dev.datatype)) {
        MPIU_Memcpy(resp_req->dev.user_buf, rreq->dev.real_user_buf,
                    rreq->dev.user_count * type_size);
    } else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint last = type_size * rreq->dev.user_count;

        MPIU_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment");
        MPID_Segment_init(rreq->dev.real_user_buf, rreq->dev.user_count, rreq->dev.datatype, seg, 0);
        MPID_Segment_pack(seg, 0, &last, resp_req->dev.user_buf);
        MPID_Segment_free(seg);
    }

    resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumLikeSendComplete;
    resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumLikeSendComplete;
    resp_req->dev.target_win_handle = rreq->dev.target_win_handle;
    resp_req->dev.flags = rreq->dev.flags;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_accum_resp_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*get_accum_resp_pkt);

    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)resp_req->dev.user_buf;
    iov[1].MPID_IOV_LEN = type_size*rreq->dev.user_count;

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iSendv(vc, resp_req, iov, 2);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);

    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    /* Mark get portion as handled */
    rreq->dev.resp_request_handle = MPI_REQUEST_NULL;

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* accumulate data from tmp_buf into user_buf */
    mpi_errno = do_accumulate_op(rreq);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }
    
    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
 fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);
    return MPI_SUCCESS;

    /* --BEGIN ERROR HANDLING-- */
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PutDerivedDTRecvComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)),
						   MPID_Request *rreq, 
						   int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);
                
    /* create derived datatype */
    create_derived_datatype(rreq, &new_dtp);
    
    /* update request to get the data */
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_PUT_RESP);
    rreq->dev.datatype = new_dtp->handle;
    rreq->dev.recv_data_sz = new_dtp->size * rreq->dev.user_count; 
    
    rreq->dev.datatype_ptr = new_dtp;
    /* this will cause the datatype to be freed when the
       request is freed. free dtype_info here. */
    MPIU_Free(rreq->dev.dtype_info);
    
    rreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(rreq->dev.user_buf,
		      rreq->dev.user_count,
		      rreq->dev.datatype,
		      rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = rreq->dev.recv_data_sz;
    
    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
			    "**ch3|loadrecviov");
    }
    if (!rreq->dev.OnDataAvail) 
	rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_PutRecvComplete;
    
    *complete = FALSE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_AccumDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_AccumDerivedDTRecvComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)),
						     MPID_Request *rreq, 
						     int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMDERIVEDDTRECVCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMDERIVEDDTRECVCOMPLETE);
    
    /* create derived datatype */
    create_derived_datatype(rreq, &new_dtp);
    
    /* update new request to get the data */
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_ACCUM_RESP);
    
    /* first need to allocate tmp_buf to recv the data into */
    
    MPIR_Type_get_true_extent_impl(new_dtp->handle, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(new_dtp->handle, extent); 
    
    tmp_buf = MPIU_Malloc(rreq->dev.user_count * 
			  (MPIR_MAX(extent,true_extent)));  
    if (!tmp_buf) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
		    rreq->dev.user_count * MPIR_MAX(extent,true_extent));
    }
    
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    rreq->dev.user_buf = tmp_buf;
    rreq->dev.final_user_buf = rreq->dev.user_buf;
    rreq->dev.datatype = new_dtp->handle;
    rreq->dev.recv_data_sz = new_dtp->size *
	rreq->dev.user_count; 
    rreq->dev.datatype_ptr = new_dtp;
    /* this will cause the datatype to be freed when the
       request is freed. free dtype_info here. */
    MPIU_Free(rreq->dev.dtype_info);
    
    rreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(rreq->dev.user_buf,
		      rreq->dev.user_count,
		      rreq->dev.datatype,
		      rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = rreq->dev.recv_data_sz;
    
    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
			    "**ch3|loadrecviov");
    }
    if (!rreq->dev.OnDataAvail)
	rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_AccumRecvComplete;
    
    *complete = FALSE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GaccumDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GaccumDerivedDTRecvComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)),
                                                      MPID_Request *rreq,
                                                      int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPI_Aint true_lb, true_extent, extent;
    void *tmp_buf;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMDERIVEDDTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMDERIVEDDTRECVCOMPLETE);

    /* create derived datatype */
    create_derived_datatype(rreq, &new_dtp);

    /* update new request to get the data */
    MPIDI_Request_set_type(rreq, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);

    /* first need to allocate tmp_buf to recv the data into */

    MPIR_Type_get_true_extent_impl(new_dtp->handle, &true_lb, &true_extent);
    MPID_Datatype_get_extent_macro(new_dtp->handle, extent);

    tmp_buf = MPIU_Malloc(rreq->dev.user_count *
			  (MPIR_MAX(extent,true_extent)));
    if (!tmp_buf) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %d",
		    rreq->dev.user_count * MPIR_MAX(extent,true_extent));
    }

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);

    rreq->dev.user_buf = tmp_buf;
    rreq->dev.final_user_buf = rreq->dev.user_buf;
    rreq->dev.datatype = new_dtp->handle;
    rreq->dev.recv_data_sz = new_dtp->size *
	rreq->dev.user_count;
    rreq->dev.datatype_ptr = new_dtp;
    /* this will cause the datatype to be freed when the
       request is freed. free dtype_info here. */
    MPIU_Free(rreq->dev.dtype_info);

    rreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(rreq->dev.user_buf,
		      rreq->dev.user_count,
		      rreq->dev.datatype,
		      rreq->dev.segment_ptr, 0);
    rreq->dev.segment_first = 0;
    rreq->dev.segment_size = rreq->dev.recv_data_sz;

    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,
			    "**ch3|loadrecviov");
    }
    if (!rreq->dev.OnDataAvail)
	rreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumRecvComplete;

    *complete = FALSE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_GetDerivedDTRecvComplete( MPIDI_VC_t *vc,
						   MPID_Request *rreq, 
						   int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Datatype *new_dtp = NULL;
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
    MPID_Request * sreq;
    MPID_Win *win_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);
                
    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* create derived datatype */
    create_derived_datatype(rreq, &new_dtp);
    MPIU_Free(rreq->dev.dtype_info);
    
    /* create request for sending data */
    sreq = MPID_Request_create();
    MPIU_ERR_CHKANDJUMP(sreq == NULL, mpi_errno,MPI_ERR_OTHER,"**nomemreq");
    
    sreq->kind = MPID_REQUEST_SEND;
    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_RESP);
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.user_buf = rreq->dev.user_buf;
    sreq->dev.user_count = rreq->dev.user_count;
    sreq->dev.datatype = new_dtp->handle;
    sreq->dev.datatype_ptr = new_dtp;
    sreq->dev.target_win_handle = rreq->dev.target_win_handle;
    sreq->dev.source_win_handle = rreq->dev.source_win_handle;
    sreq->dev.flags = rreq->dev.flags;
    
    MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
    get_resp_pkt->request_handle = rreq->dev.request_handle;    
    get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_resp_pkt->source_win_handle = rreq->dev.source_win_handle;
    get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH)
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    
    sreq->dev.segment_ptr = MPID_Segment_alloc( );
    MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");

    MPID_Segment_init(sreq->dev.user_buf,
		      sreq->dev.user_count,
		      sreq->dev.datatype,
		      sreq->dev.segment_ptr, 0);
    sreq->dev.segment_first = 0;
    sreq->dev.segment_size = new_dtp->size * sreq->dev.user_count;

    /* Because this is in a packet handler, it is already within a critical section */	
    /* MPIU_THREAD_CS_ENTER(CH3COMM,vc); */
    mpi_errno = vc->sendNoncontig_fn(vc, sreq, get_resp_pkt, sizeof(*get_resp_pkt));
    /* MPIU_THREAD_CS_EXIT(CH3COMM,vc); */
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        MPID_Request_release(sreq);
        sreq = NULL;
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }
    /* --END ERROR HANDLING-- */
    
    /* mark receive data transfer as complete and decrement CC in receive 
       request */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
    
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_GETDERIVEDDTRECVCOMPLETE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_SinglePutAccumComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_SinglePutAccumComplete( MPIDI_VC_t *vc, 
						 MPID_Request *rreq, 
						 int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Win *win_ptr;
    MPIDI_Win_lock_queue *lock_queue_entry, *curr_ptr, **curr_ptr_ptr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_SINGLEPUTACCUMCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_SINGLEPUTACCUMCOMPLETE);

    /* received all the data for single lock-put(accum)-unlock 
       optimization where the lock was not acquired in 
       ch3u_handle_recv_pkt. Try to acquire the lock and do the 
       operation. */
    
    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);
    
    lock_queue_entry = rreq->dev.lock_queue_entry;
    
    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, 
					lock_queue_entry->lock_type) == 1)
    {
	
	if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PT_SINGLE_PUT) {
	    /* copy the data over */
	    mpi_errno = MPIR_Localcopy(rreq->dev.user_buf,
				       rreq->dev.user_count,
				       rreq->dev.datatype,
				       lock_queue_entry->pt_single_op->addr,
				       lock_queue_entry->pt_single_op->count,
				       lock_queue_entry->pt_single_op->datatype);
	}
	else {
	    if (win_ptr->shm_allocated == TRUE)
		MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
	    mpi_errno = do_simple_accumulate(lock_queue_entry->pt_single_op);
	    if (win_ptr->shm_allocated == TRUE)
		MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
	}
	
	if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}
	
	/* free lock_queue_entry including data buffer and remove 
	   it from the queue. */
	curr_ptr = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	curr_ptr_ptr = (MPIDI_Win_lock_queue **) &(win_ptr->lock_queue);
	while (curr_ptr != lock_queue_entry) {
	    curr_ptr_ptr = &(curr_ptr->next);
	    curr_ptr = curr_ptr->next;
	}                    
	*curr_ptr_ptr = curr_ptr->next;
	
        mpi_errno = MPIDI_CH3_Finish_rma_op_target(vc, win_ptr, TRUE,
                                                   lock_queue_entry->pt_single_op->flags,
                                                   lock_queue_entry->source_win_handle);

	MPIU_Free(lock_queue_entry->pt_single_op->data);
	MPIU_Free(lock_queue_entry->pt_single_op);
	MPIU_Free(lock_queue_entry);
	
        if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
    }
    else {
	/* could not acquire lock. mark data recd as 1 */
	lock_queue_entry->pt_single_op->data_recd = 1;
    }
    
    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_SINGLEPUTACCUMCOMPLETE);
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackUEBufComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackUEBufComplete( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
					      MPID_Request *rreq, 
					      int *complete )
{
    int recv_pending;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);
    
    MPIDI_Request_decr_pending(rreq);
    MPIDI_Request_check_pending(rreq, &recv_pending);
    if (!recv_pending)
    { 
	if (rreq->dev.recv_data_sz > 0)
	{
	    MPIDI_CH3U_Request_unpack_uebuf(rreq);
	    MPIU_Free(rreq->dev.tmpbuf);
	}
    }
    else
    {
	/* The receive has not been posted yet.  MPID_{Recv/Irecv}() 
	   is responsible for unpacking the buffer. */
    }
    
    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;
    
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKUEBUFCOMPLETE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackSRBufComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackSRBufComplete( MPIDI_VC_t *vc, 
					      MPID_Request *rreq, 
					      int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);

    MPIDI_CH3U_Request_unpack_srbuf(rreq);

    if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_PUT_RESP)
    {
	mpi_errno = MPIDI_CH3_ReqHandler_PutRecvComplete(
	    vc, rreq, complete );
    }
    else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP)
    {
	mpi_errno = MPIDI_CH3_ReqHandler_AccumRecvComplete(
	    vc, rreq, complete );
    }
    else if (MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RESP)
    {
	mpi_errno = MPIDI_CH3_ReqHandler_GaccumRecvComplete(
	    vc, rreq, complete );
    }
    else {
	/* mark data transfer as complete and decrement CC */
	MPIDI_CH3U_Request_complete(rreq);
	*complete = TRUE;
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFCOMPLETE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_UnpackSRBufReloadIOV( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
					      MPID_Request *rreq, 
					      int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);

    MPIDI_CH3U_Request_unpack_srbuf(rreq);
    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|loadrecviov" );
    }
    *complete = FALSE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_UNPACKSRBUFRELOADIOV);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_ReqHandler_ReloadIOV
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_ReloadIOV( MPIDI_VC_t *vc ATTRIBUTE((unused)), 
				    MPID_Request *rreq, int *complete )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);

    mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
    if (mpi_errno != MPI_SUCCESS) {
	MPIU_ERR_SETFATALANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|loadrecviov");
    }
    *complete = FALSE;
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_RELOADIOV);
    return mpi_errno;
}

/* ----------------------------------------------------------------------- */
/* ----------------------------------------------------------------------- */

#undef FUNCNAME
#define FUNCNAME create_derived_datatype
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int create_derived_datatype(MPID_Request *req, MPID_Datatype **dtp)
{
    MPIDI_RMA_dtype_info *dtype_info;
    MPID_Datatype *new_dtp;
    int mpi_errno=MPI_SUCCESS;
    MPI_Aint ptrdiff;
    MPIDI_STATE_DECL(MPID_STATE_CREATE_DERIVED_DATATYPE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_CREATE_DERIVED_DATATYPE);

    dtype_info = req->dev.dtype_info;

    /* allocate new datatype object and handle */
    new_dtp = (MPID_Datatype *) MPIU_Handle_obj_alloc(&MPID_Datatype_mem);
    if (!new_dtp) {
	MPIU_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_OTHER,"**nomem","**nomem %s",
			     "MPID_Datatype_mem" );
    }

    *dtp = new_dtp;
            
    /* Note: handle is filled in by MPIU_Handle_obj_alloc() */
    MPIU_Object_set_ref(new_dtp, 1);
    new_dtp->is_permanent = 0;
    new_dtp->is_committed = 1;
    new_dtp->attributes   = 0;
    new_dtp->cache_id     = 0;
    new_dtp->name[0]      = 0;
    new_dtp->is_contig = dtype_info->is_contig;
    new_dtp->max_contig_blocks = dtype_info->max_contig_blocks; 
    new_dtp->size = dtype_info->size;
    new_dtp->extent = dtype_info->extent;
    new_dtp->dataloop_size = dtype_info->dataloop_size;
    new_dtp->dataloop_depth = dtype_info->dataloop_depth; 
    new_dtp->eltype = dtype_info->eltype;
    /* set dataloop pointer */
    new_dtp->dataloop = req->dev.dataloop;
    
    new_dtp->ub = dtype_info->ub;
    new_dtp->lb = dtype_info->lb;
    new_dtp->true_ub = dtype_info->true_ub;
    new_dtp->true_lb = dtype_info->true_lb;
    new_dtp->has_sticky_ub = dtype_info->has_sticky_ub;
    new_dtp->has_sticky_lb = dtype_info->has_sticky_lb;
    /* update pointers in dataloop */
    ptrdiff = (MPI_Aint)((char *) (new_dtp->dataloop) - (char *)
                         (dtype_info->dataloop));
    
    /* FIXME: Temp to avoid SEGV when memory tracing */
    new_dtp->hetero_dloop = 0;

    MPID_Dataloop_update(new_dtp->dataloop, ptrdiff);

    new_dtp->contents = NULL;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_CREATE_DERIVED_DATATYPE);

    return mpi_errno;
}

static int entered_flag = 0;
static int entered_count = 0;

/* Release the current lock on the window and grant the next lock in the
   queue if any */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3I_Release_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3I_Release_lock(MPID_Win *win_ptr)
{
    MPIDI_Win_lock_queue *lock_queue, **lock_queue_ptr;
    int requested_lock, mpi_errno = MPI_SUCCESS, temp_entered_count;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);

    if (win_ptr->current_lock_type == MPI_LOCK_SHARED) {
        /* decr ref cnt */
        /* FIXME: MT: Must be done atomically */
        win_ptr->shared_lock_ref_cnt--;
    }

    /* If shared lock ref count is 0 (which is also true if the lock is an
       exclusive lock), release the lock. */
    if (win_ptr->shared_lock_ref_cnt == 0) {

	/* This function needs to be reentrant even in the single-threaded case
           because when going through the lock queue, the do_simple_get 
	   called in the 
	   lock-get-unlock case may itself cause a request to complete, and 
	   this function
           may again get called in the completion action in 
	   ch3u_handle_send_req.c. To
           handle this possibility, we use an entered_flag. If the flag is 
	   not 0, we simply
	   increment the entered_count and return. The loop through the lock 
	   queue is repeated 
	   if the entered_count has changed while we are in the loop.
	 */
	if (entered_flag != 0) {
	    entered_count++;
	    goto fn_exit;
	}
	else {
	    entered_flag = 1;
	    temp_entered_count = entered_count;
	}

	do { 
	    if (temp_entered_count != entered_count) temp_entered_count++;

	    /* FIXME: MT: The setting of the lock type must be done atomically */
	    win_ptr->current_lock_type = MPID_LOCK_NONE;
	    
	    /* If there is a lock queue, try to satisfy as many lock requests as 
	       possible. If the first one is a shared lock, grant it and grant all 
	       other shared locks. If the first one is an exclusive lock, grant 
	       only that one. */
	    
	    /* FIXME: MT: All queue accesses need to be made atomic */
	    lock_queue = (MPIDI_Win_lock_queue *) win_ptr->lock_queue;
	    lock_queue_ptr = (MPIDI_Win_lock_queue **) &(win_ptr->lock_queue);
	    while (lock_queue) {
		/* if it is not a lock-op-unlock type case or if it is a 
		   lock-op-unlock type case but all the data has been received, 
		   try to acquire the lock */
		if ((lock_queue->pt_single_op == NULL) || 
		    (lock_queue->pt_single_op->data_recd == 1)) {
		    
		    requested_lock = lock_queue->lock_type;
		    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, requested_lock) 
			== 1) {
			
			if (lock_queue->pt_single_op != NULL) {
			    /* single op. do it here */
			    MPIDI_PT_single_op * single_op;
			    
			    single_op = lock_queue->pt_single_op;
			    if (single_op->type == MPIDI_CH3_PKT_LOCK_PUT_UNLOCK) {
				mpi_errno = MPIR_Localcopy(single_op->data,
							   single_op->count,
							   single_op->datatype,
							   single_op->addr,
							   single_op->count,
							   single_op->datatype);
			    }   
			    else if (single_op->type == MPIDI_CH3_PKT_LOCK_ACCUM_UNLOCK) {
				if (win_ptr->shm_allocated == TRUE)
				    MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
				mpi_errno = do_simple_accumulate(single_op);
				if (win_ptr->shm_allocated == TRUE)
				    MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
			    }
			    else if (single_op->type == MPIDI_CH3_PKT_LOCK_GET_UNLOCK) {
				mpi_errno = do_simple_get(win_ptr, lock_queue);
			    }
			    
                            if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
			    
			    /* if put or accumulate, send rma done packet and release lock. */
			    if (single_op->type != MPIDI_CH3_PKT_LOCK_GET_UNLOCK) {
                                /* NOTE: Only *queued* single_op operations are completed here.
                                   Lock-op-unlock/single_op RMA ops can also be completed as
                                   they arrive within various packet/request handlers via
                                   MPIDI_CH3_Finish_rma_op_target().  That call cannot be used
                                   here, because it would enter this function recursively. */
                                MPIDI_VC_t *vc;
                                MPIDI_Comm_get_vc(win_ptr->comm_ptr, lock_queue->origin_rank, &vc);
				mpi_errno = 
                                    MPIDI_CH3I_Send_flush_ack_pkt(vc, win_ptr,
								    lock_queue->source_win_handle);
                                if (mpi_errno) { MPIU_ERR_POP(mpi_errno); }
				
				/* release the lock */
				if (win_ptr->current_lock_type == MPI_LOCK_SHARED) {
				    /* decr ref cnt */
				    /* FIXME: MT: Must be done atomically */
				    win_ptr->shared_lock_ref_cnt--;
				}
				
				/* If shared lock ref count is 0 
				   (which is also true if the lock is an
				   exclusive lock), release the lock. */
				if (win_ptr->shared_lock_ref_cnt == 0) {
				    /* FIXME: MT: The setting of the lock type 
				       must be done atomically */
				    win_ptr->current_lock_type = MPID_LOCK_NONE;
				}
				
				/* dequeue entry from lock queue */
				MPIU_Free(single_op->data);
				MPIU_Free(single_op);
				*lock_queue_ptr = lock_queue->next;
				MPIU_Free(lock_queue);
				lock_queue = *lock_queue_ptr;
			    }
			    
			    else {
				/* it's a get. The operation is not complete. It 
				   will be completed in ch3u_handle_send_req.c. 
				   Free the single_op structure. If it's an 
				   exclusive lock, break. Otherwise continue to the
				   next operation. */
				
				MPIU_Free(single_op);
				*lock_queue_ptr = lock_queue->next;
				MPIU_Free(lock_queue);
				lock_queue = *lock_queue_ptr;
				
				if (requested_lock == MPI_LOCK_EXCLUSIVE)
				    break;
			    }
			}
			
			else {
			    /* send lock granted packet. */
                            MPIDI_VC_t *vc;
                            MPIDI_Comm_get_vc(win_ptr->comm_ptr, lock_queue->origin_rank, &vc);
			    mpi_errno = 
                                MPIDI_CH3I_Send_lock_granted_pkt(vc, win_ptr,
								 lock_queue->source_win_handle);
			    
			    /* dequeue entry from lock queue */
			    *lock_queue_ptr = lock_queue->next;
			    MPIU_Free(lock_queue);
			    lock_queue = *lock_queue_ptr;
			    
			    /* if the granted lock is exclusive, 
			       no need to continue */
			    if (requested_lock == MPI_LOCK_EXCLUSIVE)
				break;
			}
		    }
		}
		else {
		    lock_queue_ptr = &(lock_queue->next);
		    lock_queue = lock_queue->next;
		}
	    }
	} while (temp_entered_count != entered_count);
	entered_count = entered_flag = 0;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3I_RELEASE_LOCK);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



#undef FUNCNAME
#define FUNCNAME do_simple_accumulate
#undef FCNAME 
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_simple_accumulate(MPIDI_PT_single_op *single_op)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_User_function *uop;
    MPIDI_STATE_DECL(MPID_STATE_DO_SIMPLE_ACCUMULATE);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_SIMPLE_ACCUMULATE);

    if (single_op->op == MPI_REPLACE)
    {
        /* simply copy the data */
        mpi_errno = MPIR_Localcopy(single_op->data, single_op->count,
                                   single_op->datatype, single_op->addr,
                                   single_op->count, single_op->datatype);
        if (mpi_errno) {
	    MPIU_ERR_POP(mpi_errno);
	}
        goto fn_exit;
    }

    if (HANDLE_GET_KIND(single_op->op) == HANDLE_KIND_BUILTIN)
    {
        /* get the function by indexing into the op table */
        uop = MPIR_OP_HDL_TO_FN(single_op->op);
    }
    else
    {
	/* --BEGIN ERROR HANDLING-- */
        mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opnotpredefined", "**opnotpredefined %d", single_op->op );
        goto fn_fail;
	/* --END ERROR HANDLING-- */
    }
    
    /* only basic datatypes supported for this optimization. */
    (*uop)(single_op->data, single_op->addr,
           &(single_op->count), &(single_op->datatype));

 fn_fail:
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_SIMPLE_ACCUMULATE);
    return mpi_errno;
}



#undef FUNCNAME
#define FUNCNAME do_simple_get
#undef FCNAME 
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_simple_get(MPID_Win *win_ptr, MPIDI_Win_lock_queue *lock_queue)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_resp_t * get_resp_pkt = &upkt.get_resp;
    MPID_Request *req;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno=MPI_SUCCESS;
    MPI_Aint type_size;
    MPIDI_VC_t *vc;
    MPIDI_STATE_DECL(MPID_STATE_DO_SIMPLE_GET);
    
    MPIDI_FUNC_ENTER(MPID_STATE_DO_SIMPLE_GET);

    req = MPID_Request_create();
    if (req == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**nomemreq");
    }
    req->dev.target_win_handle = win_ptr->handle;
    req->dev.source_win_handle = lock_queue->source_win_handle;
    req->dev.flags = lock_queue->pt_single_op->flags;
    
    MPIDI_Request_set_type(req, MPIDI_REQUEST_TYPE_GET_RESP); 
    req->kind = MPID_REQUEST_SEND;
    req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendRespComplete;
    req->dev.OnFinal     = MPIDI_CH3_ReqHandler_GetSendRespComplete;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;
    
    MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
    get_resp_pkt->request_handle = lock_queue->pt_single_op->request_handle;
    
    iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
    iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
    
    iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)lock_queue->pt_single_op->addr;
    MPID_Datatype_get_size_macro(lock_queue->pt_single_op->datatype, type_size);
    iov[1].MPID_IOV_LEN = lock_queue->pt_single_op->count * type_size;
    
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, lock_queue->origin_rank, &vc);
    /* Because this is in a packet handler, it is already within a critical section */	
    /* MPIU_THREAD_CS_ENTER(CH3COMM,vc); */
    mpi_errno = MPIDI_CH3_iSendv(vc, req, iov, 2);
    /* MPIU_THREAD_CS_EXIT(CH3COMM,vc); */
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno != MPI_SUCCESS)
    {
        MPID_Request_release(req);
	MPIU_ERR_SETANDJUMP(mpi_errno,MPI_ERR_OTHER,"**ch3|rmamsg");
    }
    /* --END ERROR HANDLING-- */

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_DO_SIMPLE_GET);

    return mpi_errno;
}
