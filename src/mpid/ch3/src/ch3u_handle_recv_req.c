/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#include "mpidrma.h"

static int create_derived_datatype(MPID_Request * rreq, MPID_Datatype ** dtp);

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
    MPI_Win source_win_handle = rreq->dev.source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags = rreq->dev.flags;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PUTRECVCOMPLETE);

    /* NOTE: It is possible that this request is already completed before
       entering this handler. This happens when this req handler is called
       within the same req handler on the same request.
       Consider this case: req is queued up in SHM queue with ref count of 2:
       one is for completing the request and another is for dequeueing from
       the queue. The first called req handler on this request completed
       this request and decrement ref counter to 1. Request is still in the
       queue. Within this handler, we call the req handler on the same request
       for the second time (for example when making progress on SHM queue),
       and the second called handler also tries to complete this request,
       which leads to wrong execution.
       Here we check if req is already completed to prevent processing the
       same request twice. */
    if (MPID_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
       because inside finish_op_on_target() we may call this request handler
       on the same request again (in release_lock()). Marking this request as
       completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_PUT,
                                    flags, source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
    MPI_Aint true_lb, true_extent;
    MPID_Win *win_ptr;
    MPI_Win source_win_handle = rreq->dev.source_win_handle;
    MPIDI_CH3_Pkt_flags_t flags = rreq->dev.flags;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_ACCUMRECVCOMPLETE);

    /* NOTE: It is possible that this request is already completed before
       entering this handler. This happens when this req handler is called
       within the same req handler on the same request.
       Consider this case: req is queued up in SHM queue with ref count of 2:
       one is for completing the request and another is for dequeueing from
       the queue. The first called req handler on this request completed
       this request and decrement ref counter to 1. Request is still in the
       queue. Within this handler, we call the req handler on the same request
       for the second time (for example when making progress on SHM queue),
       and the second called handler also tries to complete this request,
       which leads to wrong execution.
       Here we check if req is already completed to prevent processing the
       same request twice. */
    if (MPID_Request_is_complete(rreq)) {
        *complete = FALSE;
        goto fn_exit;
    }

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_ACCUM_RESP);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* accumulate data from tmp_buf into user_buf */
    mpi_errno = do_accumulate_op(rreq->dev.final_user_buf, rreq->dev.real_user_buf,
                                 rreq->dev.user_count, rreq->dev.datatype, rreq->dev.op);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* free the temporary buffer */
    MPIR_Type_get_true_extent_impl(rreq->dev.datatype, &true_lb, &true_extent);
    MPIU_Free((char *) rreq->dev.final_user_buf + true_lb);

    /* mark data transfer as complete and decrement CC */
    MPIDI_CH3U_Request_complete(rreq);

    /* NOTE: finish_op_on_target() must be called after we complete this request,
       because inside finish_op_on_target() we may call this request handler
       on the same request again (in release_lock()). Marking this request as
       completed will prevent us from processing the same request twice. */
    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_ACCUMULATE,
                                    flags, source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
    MPI_Aint true_lb, true_extent;
    size_t len;
    int iovcnt;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_GACCUMRECVCOMPLETE);

    MPID_Win_get_ptr(rreq->dev.target_win_handle, win_ptr);

    MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP);
    get_accum_resp_pkt->request_handle = rreq->dev.resp_request_handle;
    get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_accum_resp_pkt->source_win_handle = rreq->dev.source_win_handle;
    get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    get_accum_resp_pkt->immed_len = 0;

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

    resp_req->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    resp_req->dev.target_win_handle = rreq->dev.target_win_handle;
    resp_req->dev.flags = rreq->dev.flags;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    /* length of target data */
    MPIU_Assign_trunc(len, rreq->dev.user_count * type_size, size_t);

    /* both origin buffer and target buffer are basic datatype,
       fill IMMED data area in response packet header. */
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        /* Try to copy target data into packet header. */
        MPIU_Assign_trunc(get_accum_resp_pkt->immed_len,
                          MPIR_MIN(len, (MPIDI_RMA_IMMED_BYTES / type_size) * type_size),
                          int);

        if (get_accum_resp_pkt->immed_len > 0) {
            void *src = resp_req->dev.user_buf;
            void *dest = (void*) get_accum_resp_pkt->data;
            /* copy data from origin buffer to immed area in packet header */
            mpi_errno = immed_copy(src, dest, (size_t)get_accum_resp_pkt->immed_len);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }

    if (len == (size_t)get_accum_resp_pkt->immed_len) {
        /* All origin data is in packet header, issue the header. */
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iovcnt = 1;
    }
    else {
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)resp_req->dev.user_buf + get_accum_resp_pkt->immed_len);
        iov[1].MPID_IOV_LEN = rreq->dev.user_count * type_size - get_accum_resp_pkt->immed_len;
        iovcnt = 2;
    }

    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iSendv(vc, resp_req, iov, iovcnt);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);

    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    /* Mark get portion as handled */
    rreq->dev.resp_request_handle = MPI_REQUEST_NULL;

    MPIU_Assert(MPIDI_Request_get_type(rreq) == MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
    /* accumulate data from tmp_buf into user_buf */
    mpi_errno = do_accumulate_op(rreq->dev.final_user_buf, rreq->dev.real_user_buf,
                                 rreq->dev.user_count, rreq->dev.datatype, rreq->dev.op);
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    if (mpi_errno) {
        MPIU_ERR_POP(mpi_errno);
    }

    /* free the temporary buffer */
    MPIR_Type_get_true_extent_impl(rreq->dev.datatype, &true_lb, &true_extent);
    MPIU_Free((char *) rreq->dev.final_user_buf + true_lb);
    
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
    if (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (rreq->dev.flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    get_resp_pkt->immed_len = 0;
    
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


static inline int perform_put_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_put_t *put_pkt = &((lock_entry->pkt).put);
    MPIDI_VC_t *vc = NULL;
    int mpi_errno = MPI_SUCCESS;

    if (lock_entry->data == NULL) {
        /* all data fits in packet header */
        mpi_errno = MPIR_Localcopy(put_pkt->data, put_pkt->count, put_pkt->datatype,
                                   put_pkt->addr, put_pkt->count, put_pkt->datatype);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = MPIR_Localcopy(lock_entry->data, put_pkt->count, put_pkt->datatype,
                                   put_pkt->addr, put_pkt->count, put_pkt->datatype);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, put_pkt->origin_rank, &vc);

    /* do final action */
    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_PUT,
                                    put_pkt->flags, put_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

static inline int perform_get_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_resp_t *get_resp_pkt = &upkt.get_resp;
    MPIDI_CH3_Pkt_get_t *get_pkt = &((lock_entry->pkt).get);
    MPID_Request *sreq = NULL;
    MPIDI_VC_t *vc = NULL;
    MPI_Aint type_size;
    size_t len;
    int iovcnt;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;

    sreq = MPID_Request_create();
    if (sreq == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }
    MPIU_Object_set_ref(sreq, 1);

    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_RESP);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GetSendComplete;
    sreq->dev.OnFinal = MPIDI_CH3_ReqHandler_GetSendComplete;

    sreq->dev.target_win_handle = win_ptr->handle;
    sreq->dev.flags = get_pkt->flags;

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    MPIDI_Pkt_init(get_resp_pkt, MPIDI_CH3_PKT_GET_RESP);
    get_resp_pkt->request_handle = get_pkt->request_handle;
    get_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    get_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_resp_pkt->source_win_handle = get_pkt->source_win_handle;
    get_resp_pkt->immed_len = 0;

    /* length of target data */
    MPID_Datatype_get_size_macro(get_pkt->datatype, type_size);
    MPIU_Assign_trunc(len, get_pkt->count * type_size, size_t);

    /* both origin buffer and target buffer are basic datatype,
       fill IMMED data area in response packet header. */
    if (get_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        /* Try to copy target data into packet header. */
        MPIU_Assign_trunc(get_resp_pkt->immed_len,
                          MPIR_MIN(len, (MPIDI_RMA_IMMED_BYTES / type_size) * type_size),
                          int);

        if (get_resp_pkt->immed_len > 0) {
            void *src = get_pkt->addr;
            void *dest = (void*) get_resp_pkt->data;
            /* copy data from origin buffer to immed area in packet header */
            mpi_errno = immed_copy(src, dest, (size_t)get_resp_pkt->immed_len);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }

    if (len == (size_t)get_resp_pkt->immed_len) {
        /* All origin data is in packet header, issue the header. */
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
        iovcnt = 1;
    }
    else {
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_resp_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)get_pkt->addr + get_resp_pkt->immed_len);
        iov[1].MPID_IOV_LEN = get_pkt->count * type_size - get_resp_pkt->immed_len;
        iovcnt = 2;
    }

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, get_pkt->origin_rank, &vc);

    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iovcnt);
    if (mpi_errno != MPI_SUCCESS) {
        MPID_Request_release(sreq);
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int perform_acc_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_accum_t *acc_pkt = &((lock_entry->pkt).accum);
    MPIDI_VC_t *vc = NULL;
    int mpi_errno = MPI_SUCCESS;

    MPIU_Assert(lock_entry->all_data_recved == 1);

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    if (lock_entry->data == NULL) {
        /* All data fits in packet header */
        mpi_errno = do_accumulate_op(acc_pkt->data, acc_pkt->addr,
                                     acc_pkt->count, acc_pkt->datatype, acc_pkt->op);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = do_accumulate_op(lock_entry->data, acc_pkt->addr,
                                     acc_pkt->count, acc_pkt->datatype, acc_pkt->op);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, acc_pkt->origin_rank, &vc);

    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_ACCUMULATE,
                                    acc_pkt->flags, acc_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int perform_get_acc_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_get_accum_resp_t *get_accum_resp_pkt = &upkt.get_accum_resp;
    MPIDI_CH3_Pkt_get_accum_t *get_accum_pkt = &((lock_entry->pkt).get_accum);
    MPID_Request *sreq = NULL;
    MPIDI_VC_t *vc = NULL;
    MPI_Aint type_size;
    size_t len;
    int iovcnt;
    MPID_IOV iov[MPID_IOV_LIMIT];
    int mpi_errno = MPI_SUCCESS;

    sreq = MPID_Request_create();
    if (sreq == NULL) {
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomemreq");
    }
    MPIU_Object_set_ref(sreq, 1);

    MPIDI_Request_set_type(sreq, MPIDI_REQUEST_TYPE_GET_ACCUM_RESP);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = MPIDI_CH3_ReqHandler_GaccumSendComplete;
    sreq->dev.OnFinal = MPIDI_CH3_ReqHandler_GaccumSendComplete;

    sreq->dev.target_win_handle = win_ptr->handle;
    sreq->dev.flags = get_accum_pkt->flags;

    /* Copy data into a temporary buffer */
    MPID_Datatype_get_size_macro(get_accum_pkt->datatype, type_size);
    sreq->dev.user_buf = (void *)MPIU_Malloc(get_accum_pkt->count * type_size);

    if (MPIR_DATATYPE_IS_PREDEFINED(get_accum_pkt->datatype)) {
        MPIU_Memcpy(sreq->dev.user_buf, get_accum_pkt->addr,
                    get_accum_pkt->count * type_size);
    } else {
        MPID_Segment *seg = MPID_Segment_alloc();
        MPI_Aint last = type_size * get_accum_pkt->count;

        MPIU_ERR_CHKANDJUMP1(seg == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment");
        MPID_Segment_init(get_accum_pkt->addr, get_accum_pkt->count,
                          get_accum_pkt->datatype, seg, 0);
        MPID_Segment_pack(seg, 0, &last, sreq->dev.user_buf);
        MPID_Segment_free(seg);
    }

    /* here we increment the Active Target counter to guarantee the GET-like
       operation are completed when counter reaches zero. */
    win_ptr->at_completion_counter++;

    MPIDI_Pkt_init(get_accum_resp_pkt, MPIDI_CH3_PKT_GET_ACCUM_RESP);
    get_accum_resp_pkt->request_handle = get_accum_pkt->request_handle;
    get_accum_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        get_accum_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    get_accum_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    get_accum_resp_pkt->source_win_handle = get_accum_pkt->source_win_handle;
    get_accum_resp_pkt->immed_len = 0;

    /* length of target data */
    MPIU_Assign_trunc(len, get_accum_pkt->count * type_size, size_t);

    /* both origin buffer and target buffer are basic datatype,
       fill IMMED data area in response packet header. */
    if (get_accum_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_IMMED_RESP) {
        /* Try to copy target data into packet header. */
        MPIU_Assign_trunc(get_accum_resp_pkt->immed_len,
                          MPIR_MIN(len, (MPIDI_RMA_IMMED_BYTES / type_size) * type_size),
                          int);

        if (get_accum_resp_pkt->immed_len > 0) {
            void *src = sreq->dev.user_buf;
            void *dest = (void*) get_accum_resp_pkt->data;
            /* copy data from origin buffer to immed area in packet header */
            mpi_errno = immed_copy(src, dest, (size_t)get_accum_resp_pkt->immed_len);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }

    if (len == (size_t)get_accum_resp_pkt->immed_len) {
        /* All origin data is in packet header, issue the header. */
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iovcnt = 1;
    }
    else {
        iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) get_accum_resp_pkt;
        iov[0].MPID_IOV_LEN = sizeof(*get_accum_resp_pkt);
        iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST) ((char *)sreq->dev.user_buf + get_accum_resp_pkt->immed_len);
        iov[1].MPID_IOV_LEN = get_accum_pkt->count * type_size - get_accum_resp_pkt->immed_len;
        iovcnt = 2;
    }

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, get_accum_pkt->origin_rank, &vc);

    mpi_errno = MPIDI_CH3_iSendv(vc, sreq, iov, iovcnt);
    if (mpi_errno != MPI_SUCCESS) {
        MPID_Request_release(sreq);
	MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");
    }

    /* Perform ACCUMULATE OP */
    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    if (lock_entry->data == NULL) {
        /* All data fits in packet header */
        mpi_errno = do_accumulate_op(get_accum_pkt->data, get_accum_pkt->addr,
                                     get_accum_pkt->count, get_accum_pkt->datatype, get_accum_pkt->op);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else {
        mpi_errno = do_accumulate_op(lock_entry->data, get_accum_pkt->addr,
                                     get_accum_pkt->count, get_accum_pkt->datatype, get_accum_pkt->op);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int perform_fop_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_fop_resp_t *fop_resp_pkt = &upkt.fop_resp;
    MPIDI_CH3_Pkt_fop_t *fop_pkt = &((lock_entry->pkt).fop);
    MPID_Request *resp_req = NULL;
    MPIDI_VC_t *vc = NULL;
    int mpi_errno = MPI_SUCCESS;

    /* FIXME: this function is same with PktHandler_FOP(), should
       do code refactoring on both of them. */

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, fop_pkt->origin_rank, &vc);

    MPIDI_Pkt_init(fop_resp_pkt, MPIDI_CH3_PKT_FOP_RESP);
    fop_resp_pkt->request_handle = fop_pkt->request_handle;
    fop_resp_pkt->source_win_handle = fop_pkt->source_win_handle;
    fop_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    fop_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (fop_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        fop_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;
    fop_resp_pkt->immed_len = fop_pkt->immed_len;

    /* copy data to resp pkt header */
    void *src = fop_pkt->addr, *dest = fop_resp_pkt->data;
    mpi_errno = immed_copy(src, dest, (size_t)fop_resp_pkt->immed_len);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Apply the op */
    if (fop_pkt->op != MPI_NO_OP) {
        MPI_User_function *uop = MPIR_OP_HDL_TO_FN(fop_pkt->op);
        int one = 1;
        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);
        (*uop)(fop_pkt->data, fop_pkt->addr, &one, &(fop_pkt->datatype));
        if (win_ptr->shm_allocated == TRUE)
            MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);
    }

    /* send back the original data */
    MPIU_THREAD_CS_ENTER(CH3COMM,vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, fop_resp_pkt, sizeof(*fop_resp_pkt), &resp_req);
    MPIU_THREAD_CS_EXIT(CH3COMM,vc);
    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (resp_req != NULL) {
        if (!MPID_Request_is_complete(resp_req)) {
            /* sending process is not completed, set proper OnDataAvail
               (it is initialized to NULL by lower layer) */
            resp_req->dev.target_win_handle = fop_pkt->target_win_handle;
            resp_req->dev.flags = fop_pkt->flags;
            resp_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_FOPSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(resp_req);
            goto fn_exit;
        }
        else {
            MPID_Request_release(resp_req);
        }
    }

    /* do final action */
    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_FOP,
                                    fop_pkt->flags, fop_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int perform_cas_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    MPIDI_CH3_Pkt_t upkt;
    MPIDI_CH3_Pkt_cas_resp_t *cas_resp_pkt = &upkt.cas_resp;
    MPIDI_CH3_Pkt_cas_t *cas_pkt = &((lock_entry->pkt).cas);
    MPID_Request *send_req = NULL;
    MPIDI_VC_t *vc = NULL;
    MPI_Aint len;
    int mpi_errno = MPI_SUCCESS;

    /* get vc object */
    MPIDI_Comm_get_vc(win_ptr->comm_ptr, cas_pkt->origin_rank, &vc);

    MPIDI_Pkt_init(cas_resp_pkt, MPIDI_CH3_PKT_CAS_RESP);
    cas_resp_pkt->request_handle = cas_pkt->request_handle;
    cas_resp_pkt->source_win_handle = cas_pkt->source_win_handle;
    cas_resp_pkt->target_rank = win_ptr->comm_ptr->rank;
    cas_resp_pkt->flags = MPIDI_CH3_PKT_FLAG_NONE;
    if (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_LOCK)
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED;
    if ((cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_FLUSH) ||
        (cas_pkt->flags & MPIDI_CH3_PKT_FLAG_RMA_UNLOCK))
        cas_resp_pkt->flags |= MPIDI_CH3_PKT_FLAG_RMA_FLUSH_ACK;

    /* Copy old value into the response packet */
    MPID_Datatype_get_size_macro(cas_pkt->datatype, len);
    MPIU_Assert(len <= sizeof(MPIDI_CH3_CAS_Immed_u));

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_LOCK(win_ptr);

    MPIU_Memcpy((void *) &cas_resp_pkt->data, cas_pkt->addr, len);

    /* Compare and replace if equal */
    if (MPIR_Compare_equal(&cas_pkt->compare_data, cas_pkt->addr, cas_pkt->datatype)) {
        MPIU_Memcpy(cas_pkt->addr, &cas_pkt->origin_data, len);
    }

    if (win_ptr->shm_allocated == TRUE)
        MPIDI_CH3I_SHM_MUTEX_UNLOCK(win_ptr);

    /* Send the response packet */
    MPIU_THREAD_CS_ENTER(CH3COMM, vc);
    mpi_errno = MPIDI_CH3_iStartMsg(vc, cas_resp_pkt, sizeof(*cas_resp_pkt), &send_req);
    MPIU_THREAD_CS_EXIT(CH3COMM, vc);

    MPIU_ERR_CHKANDJUMP(mpi_errno != MPI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**ch3|rmamsg");

    if (send_req != NULL) {
        if (!MPID_Request_is_complete(send_req)) {
            /* sending process is not completed, set proper OnDataAvail
               (it is initialized to NULL by lower layer) */
            send_req->dev.target_win_handle = cas_pkt->target_win_handle;
            send_req->dev.flags = cas_pkt->flags;
            send_req->dev.OnDataAvail = MPIDI_CH3_ReqHandler_CASSendComplete;

            /* here we increment the Active Target counter to guarantee the GET-like
               operation are completed when counter reaches zero. */
            win_ptr->at_completion_counter++;

            MPID_Request_release(send_req);
            goto fn_exit;
        }
        else
            MPID_Request_release(send_req);
    }

    /* do final action */
    mpi_errno = finish_op_on_target(win_ptr, vc, MPIDI_CH3_PKT_CAS,
                                    cas_pkt->flags, cas_pkt->source_win_handle);
    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


static inline int perform_op_in_lock_queue(MPID_Win *win_ptr, MPIDI_RMA_Lock_entry_t *lock_entry)
{
    int mpi_errno = MPI_SUCCESS;

    if (lock_entry->pkt.type == MPIDI_CH3_PKT_LOCK) {

        /* single LOCK request */

        MPIDI_CH3_Pkt_lock_t *lock_pkt = &(lock_entry->pkt.lock);
        if (lock_pkt->origin_rank == win_ptr->comm_ptr->rank) {
            mpi_errno = handle_lock_ack(win_ptr, lock_pkt->origin_rank,
                                              MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else {
            MPIDI_VC_t *vc = NULL;
            MPIDI_Comm_get_vc_set_active(win_ptr->comm_ptr,
                                         lock_pkt->origin_rank, &vc);
            mpi_errno = MPIDI_CH3I_Send_lock_ack_pkt(vc, win_ptr,
                                                     MPIDI_CH3_PKT_FLAG_RMA_LOCK_GRANTED,
                                              lock_pkt->source_win_handle);
            if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }
    else {
        /* LOCK+OP packet */
        switch(lock_entry->pkt.type) {
        case (MPIDI_CH3_PKT_PUT):
            mpi_errno = perform_put_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_GET):
            mpi_errno = perform_get_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_ACCUMULATE):
            mpi_errno = perform_acc_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_GET_ACCUM):
            mpi_errno = perform_get_acc_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_FOP):
            mpi_errno = perform_fop_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        case (MPIDI_CH3_PKT_CAS):
            mpi_errno = perform_cas_in_lock_queue(win_ptr, lock_entry);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            break;
        default:
            MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER,
                                 "**invalidpkt", "**invalidpkt %d", lock_entry->pkt.type);
        }
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
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
    MPIDI_RMA_Lock_entry_t *lock_entry, *lock_entry_next;
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
           because when going through the lock queue, pkt_handler() in
           perform_op_in_lock_queue() may again call release_lock(). To handle
           this possibility, we use an entered_flag.
           If the flag is not 0, we simply increment the entered_count and return.
           The loop through the lock queue is repeated if the entered_count has
           changed while we are in the loop.
	 */
	if (entered_flag != 0) {
	    entered_count++; /* Count how many times we re-enter */
	    goto fn_exit;
	}

        entered_flag = 1;  /* Mark that we are now entering release_lock() */
        temp_entered_count = entered_count;

	do {
	    if (temp_entered_count != entered_count) temp_entered_count++;

	    /* FIXME: MT: The setting of the lock type must be done atomically */
	    win_ptr->current_lock_type = MPID_LOCK_NONE;

	    /* If there is a lock queue, try to satisfy as many lock requests as 
	       possible. If the first one is a shared lock, grant it and grant all 
	       other shared locks. If the first one is an exclusive lock, grant 
	       only that one. */

	    /* FIXME: MT: All queue accesses need to be made atomic */
            lock_entry = (MPIDI_RMA_Lock_entry_t *) win_ptr->lock_queue;
            while (lock_entry) {
                lock_entry_next = lock_entry->next;

                if (lock_entry->all_data_recved) {
                MPIDI_CH3_PKT_RMA_GET_LOCK_TYPE(lock_entry->pkt, requested_lock, mpi_errno);
                if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, requested_lock) == 1) {
                    /* dequeue entry from lock queue */
                    MPL_LL_DELETE(win_ptr->lock_queue, win_ptr->lock_queue_tail, lock_entry);

                    /* perform this OP */
                    mpi_errno = perform_op_in_lock_queue(win_ptr, lock_entry);
                    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                    /* free this entry */
                    mpi_errno = MPIDI_CH3I_Win_lock_entry_free(win_ptr, lock_entry);
                    if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                    /* if the granted lock is exclusive,
                       no need to continue */
                    if (requested_lock == MPI_LOCK_EXCLUSIVE)
                        break;
                }
                }
                lock_entry = lock_entry_next;
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
#define FUNCNAME MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_ReqHandler_PiggybackLockOpRecvComplete( MPIDI_VC_t *vc,
                                                      MPID_Request *rreq,
                                                      int *complete )
{
    int requested_lock;
    MPI_Win target_win_handle;
    MPID_Win *win_ptr = NULL;
    MPIDI_RMA_Lock_entry_t *lock_queue_entry = rreq->dev.lock_queue_entry;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);

    /* This handler is triggered when we received all data of a lock queue
       entry */

    /* Note that if we decided to drop op data, here we just need to complete this
       request; otherwise we try to get the lock again in this handler. */
    if (rreq->dev.lock_queue_entry != NULL) {

    /* Mark all data received in lock queue entry */
    lock_queue_entry->all_data_recved = 1;

    /* try to acquire the lock here */
    MPIDI_CH3_PKT_RMA_GET_LOCK_TYPE(lock_queue_entry->pkt, requested_lock, mpi_errno);
    MPIDI_CH3_PKT_RMA_GET_TARGET_WIN_HANDLE(lock_queue_entry->pkt, target_win_handle, mpi_errno);
    MPID_Win_get_ptr(target_win_handle, win_ptr);

    if (MPIDI_CH3I_Try_acquire_win_lock(win_ptr, requested_lock) == 1) {
        /* dequeue entry from lock queue */
        MPL_LL_DELETE(win_ptr->lock_queue, win_ptr->lock_queue_tail, lock_queue_entry);

        /* perform this OP */
        mpi_errno = perform_op_in_lock_queue(win_ptr, lock_queue_entry);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* free this entry */
        mpi_errno = MPIDI_CH3I_Win_lock_entry_free(win_ptr, lock_queue_entry);
        if (mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    /* If try acquiring lock failed, just leave the lock queue entry in the queue with
       all_data_recved marked as 1, release_lock() function will traverse the queue
       and find entry with all_data_recved being 1 to grant the lock. */
    }

    /* mark receive data transfer as complete and decrement CC in receive
       request */
    MPIDI_CH3U_Request_complete(rreq);
    *complete = TRUE;

 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_REQHANDLER_PIGGYBACKLOCKOPRECVCOMPLETE);
    return mpi_errno;
}
