/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"
#include "nd_sm.h"

MPID_Nem_nd_dev_hnd_t   MPID_Nem_nd_dev_hnd_g;
MPID_Nem_nd_conn_hnd_t  MPID_Nem_nd_lconn_hnd;
MPIU_ExSetHandle_t MPID_Nem_nd_exset_hnd_g = MPIU_EX_INVALID_SET;

/* The ND handler func decls */
static int gen_send_fail_handler(MPID_Nem_nd_msg_result_t *send_result);
static int gen_recv_fail_handler(MPID_Nem_nd_msg_result_t *recv_result);
static int send_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int zcp_mw_send_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int cont_send_success_handler(MPID_Nem_nd_msg_result_t *zcp_result);
static int netmod_msg_send_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int zcp_read_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int zcp_read_fail_handler(MPID_Nem_nd_msg_result_t *send_result);
static int wait_cack_success_handler(MPID_Nem_nd_msg_result_t *recv_result);
static int wait_lack_success_handler(MPID_Nem_nd_msg_result_t *recv_result);
static int recv_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int dummy_msg_handler(MPID_Nem_nd_msg_result_t *result);
static int quiescent_msg_handler(MPID_Nem_nd_msg_result_t *result);
static int free_msg_result_handler(MPID_Nem_nd_msg_result_t *result);

/* The EX handler func decls */
extern "C"{
static int __cdecl listen_success_handler(MPIU_EXOVERLAPPED *recv_ov);
static int __cdecl connecting_success_handler(MPIU_EXOVERLAPPED *send_ov);
static int __cdecl connecting_fail_handler(MPIU_EXOVERLAPPED *send_ov);
static int __cdecl quiescent_handler(MPIU_EXOVERLAPPED *send_ov);
static int __cdecl passive_quiescent_handler(MPIU_EXOVERLAPPED *recv_ov);
static int __cdecl gen_ex_fail_handler(MPIU_EXOVERLAPPED *ov);
static int __cdecl block_op_handler(MPIU_EXOVERLAPPED *ov);
static int __cdecl manual_event_handler(MPIU_EXOVERLAPPED *ov);
static int __cdecl dummy_handler(MPIU_EXOVERLAPPED *ov);
}

static inline int MPID_Nem_nd_handle_posted_sendq_head_req(MPIDI_VC_t *vc, int *req_complete);
static inline int MPID_Nem_nd_handle_posted_sendq_tail_req(MPIDI_VC_t *vc, int *req_complete);
static int process_pending_req(MPID_Nem_nd_conn_hnd_t conn_hnd);
int MPID_Nem_nd_update_fc_info(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg);
int MPID_Nem_nd_sm_block(MPID_Nem_nd_block_op_hnd_t op_hnd);
int MPID_Nem_nd_pack_iov(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_IOV *iovp,
                         int offset_start,
                         int *offset_endp,
                         MPID_Nem_nd_msg_t *pmsg,
                         MPID_Nem_nd_pack_t *pack_typep,
                         SIZE_T *nbp);
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_sm_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_sm_init(void )
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_INIT);

    /* Create Ex set */
    MPID_Nem_nd_exset_hnd_g = MPIU_ExCreateSet();
    MPIU_ERR_CHKANDJUMP((MPID_Nem_nd_exset_hnd_g == MPIU_EX_INVALID_SET),
        mpi_errno, MPI_ERR_OTHER, "**ex_create_set");

    /* Initialize ND device */
    mpi_errno = MPID_Nem_nd_dev_hnd_init(&MPID_Nem_nd_dev_hnd_g, MPID_Nem_nd_exset_hnd_g);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Initialize a blocking op that waits until all pending ops on the conn complete */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_block_op_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_block_op_init(MPID_Nem_nd_block_op_hnd_t *phnd, int npending_ops, MPID_Nem_nd_conn_hnd_t conn_hnd, int is_manual_event)
{
    int mpi_errno = MPI_SUCCESS;
	HRESULT hr;
    OVERLAPPED *pov;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_BLOCK_OP_INIT);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_BLOCK_OP_INIT);

    MPIU_Assert(phnd != NULL);
    MPIU_Assert(npending_ops > 0);

    /* FIXME: For now we only allow 1 blocking op on the conn */
    MPIU_Assert(conn_hnd->npending_ops == 0);
	MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    MPIU_CHKPMEM_MALLOC(*phnd, MPID_Nem_nd_block_op_hnd_t, sizeof(struct MPID_Nem_nd_block_op_hnd_), mpi_errno, "Block op hnd");

    (*phnd)->npending_ops = npending_ops;
	(*phnd)->conn_hnd = conn_hnd;

    if(is_manual_event){
        MPIU_ExInitOverlapped(&((*phnd)->ex_ov), manual_event_handler, manual_event_handler);
    }
    else{
        MPIU_Assert(0);
        MPIU_ExInitOverlapped(&((*phnd)->ex_ov), block_op_handler, block_op_handler);
    }

    pov = MPIU_EX_GET_OVERLAPPED_PTR(&((*phnd)->ex_ov));

    /* Executive initializes event to NULL - So create events after initializing the 
     * handlers
     */
    pov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    MPIU_ERR_CHKANDJUMP((pov->hEvent == NULL), mpi_errno, MPI_ERR_OTHER, "**intern");

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Get notifications from cq(%p)/conn(%p)/block_op(%p) on ov(%p)",
        MPID_Nem_nd_dev_hnd_g->p_cq, conn_hnd, (*phnd), pov));

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_BLOCK_OP_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_block_op_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_block_op_finalize(MPID_Nem_nd_block_op_hnd_t *phnd)
{
    int mpi_errno = MPI_SUCCESS;
    OVERLAPPED *pov;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);

    MPIU_Assert(phnd != NULL);
    if(*phnd){
        pov = MPIU_EX_GET_OVERLAPPED_PTR(&((*phnd)->ex_ov));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Trying to finalize conn(%p)/block_op(%p) on ov(%p)",
           (*phnd)->conn_hnd, (*phnd), pov));
        if(pov->hEvent){
            CloseHandle(pov->hEvent);
        }
        MPIU_Free(*phnd);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_block_op_reinit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_block_op_reinit(MPID_Nem_nd_block_op_hnd_t op_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    OVERLAPPED *pov;
    BOOL ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_BLOCK_OP_REINIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_BLOCK_OP_REINIT);

	MPIU_Assert(op_hnd != MPID_NEM_ND_BLOCK_OP_HND_INVALID);
	MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(op_hnd->conn_hnd));

	/* Re-initialize the ex ov */
    ret = MPIU_ExReInitOverlapped(&(op_hnd->ex_ov), NULL, NULL);
    MPIU_ERR_CHKANDJUMP((ret == FALSE), mpi_errno, MPI_ERR_OTHER, "**intern");

    pov = MPIU_EX_GET_OVERLAPPED_PTR(&(op_hnd->ex_ov));
    MPIU_Assert(pov->hEvent != NULL);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Re-initializing conn(%p)/block_op(%p) on ov(%p)",
           op_hnd->conn_hnd, op_hnd, pov));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_BLOCK_OP_REINIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_msg_bufs_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_msg_bufs_init(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS, i;
    HRESULT hr;
    ND_SGE sge;
    MPID_Nem_nd_block_op_hnd_t rsbuf_op_hnd, ssbuf_op_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_MSG_BUFS_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_MSG_BUFS_INIT);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(conn_hnd));
    MPIU_Assert(MPID_NEM_ND_DEV_HND_IS_INIT(MPID_Nem_nd_dev_hnd_g));
    /* Init connection flow control fields */
    conn_hnd->recv_credits = 0;
    conn_hnd->send_credits = MPID_NEM_ND_CONN_FC_RW + 1;
    /* conn_hnd->send_credits = MPID_NEM_ND_CONN_FC_BUFS_MAX; */

    MSGBUF_FREEQ_INIT(conn_hnd);

    /* Register the sendq & recvq with adapter - We block while registering memory */
    mpi_errno = MPID_Nem_nd_block_op_init(&rsbuf_op_hnd, 1, conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Registring rs memory conn(%p)/block_op(%p) on ov(%p)",
           conn_hnd, rsbuf_op_hnd, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(rsbuf_op_hnd)));

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->rsbuf, sizeof(conn_hnd->rsbuf), MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(rsbuf_op_hnd), &(conn_hnd->rsbuf_hmr));
    if(SUCCEEDED(hr)){
		/* Manual event */
		conn_hnd->npending_ops++;
		mpi_errno = MPID_Nem_nd_sm_block(rsbuf_op_hnd);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    mpi_errno = MPID_Nem_nd_block_op_init(&ssbuf_op_hnd, 1, conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Registring ss memory conn(%p)/block_op(%p) on ov(%p)",
           conn_hnd, ssbuf_op_hnd, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(ssbuf_op_hnd)));

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->ssbuf, sizeof(conn_hnd->ssbuf), MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(ssbuf_op_hnd), &(conn_hnd->ssbuf_hmr));
    if(SUCCEEDED(hr)){
		/* Manual event */
		conn_hnd->npending_ops++;
		mpi_errno = MPID_Nem_nd_sm_block(ssbuf_op_hnd);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    /* Set the msg handlers & conn refs for send buffers */
    for(i=0;i<MPID_NEM_ND_CONN_SENDQ_SZ;i++){
        conn_hnd->ssbuf[i].conn_hnd = conn_hnd;
        SET_MSGBUF_HANDLER(&((conn_hnd->ssbuf[i]).msg), send_success_handler, gen_send_fail_handler);
    }

    /* Prepost receives - These receives are reposted within the progress engine */
    sge.Length = sizeof(MPID_Nem_nd_msg_t);
    sge.hMr = conn_hnd->rsbuf_hmr;
    conn_hnd->p_ep->StartRequestBatch();
    for(i=0;i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
        conn_hnd->rsbuf[i].conn_hnd = conn_hnd;
        SET_MSGBUF_HANDLER(&(conn_hnd->rsbuf[i].msg), recv_success_handler, gen_recv_fail_handler);
        sge.pAddr = &(conn_hnd->rsbuf[i].msg);
        hr = conn_hnd->p_ep->Receive(GET_PNDRESULT_FROM_MSGBUF(&(conn_hnd->rsbuf[i].msg)), &sge, 1);
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }
    conn_hnd->p_ep->SubmitRequestBatch();

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn_hnd (%p)", conn_hnd));
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "========== RECV SBUFS ===========");
    for(i=0; i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn_hnd->rsbuf[%d].msg = (%p)", i, &(conn_hnd->rsbuf[i].msg)));
    }
    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "========== SEND SBUFS ===========");
    for(i=0; i<MPID_NEM_ND_CONN_SENDQ_SZ;i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn_hnd->ssbuf[%d].msg = (%p)", i, &(conn_hnd->ssbuf[i].msg)));
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_MSG_BUFS_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_accept
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_accept(MPID_Nem_nd_conn_hnd_t lconn_hnd, MPID_Nem_nd_conn_hnd_t new_conn_hnd, int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    ND_SGE sge;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_ACCEPT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_ACCEPT);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(lconn_hnd));
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(new_conn_hnd));

    if(is_blocking){
        MPID_Nem_nd_block_op_hnd_t op_hnd;

        mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd, 1, lconn_hnd, 1);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Blocking on accept lconn(%p)/block_op(%p) on ov(%p)",
           lconn_hnd, op_hnd, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd)));

        hr = new_conn_hnd->p_conn->Accept(new_conn_hnd->p_ep, NULL, 0, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd));
        if(SUCCEEDED(hr)){
			/* Manual event */
			lconn_hnd->npending_ops++;
			mpi_errno = MPID_Nem_nd_sm_block(op_hnd);
			if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);

        /* Get a new connector */
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->CreateConnector(&(lconn_hnd->p_conn));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting next req for conn on lconn(%p)/block_op(%p) on ov(%p)",
           lconn_hnd, op_hnd, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd)));

        /* Post next req for connection */
        hr = MPID_Nem_nd_dev_hnd_g->p_listen->GetConnectionRequest(lconn_hnd->p_conn, MPIU_EX_GET_OVERLAPPED_PTR(&(lconn_hnd->recv_ov)));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }
    else{ /* is not blocking */
        MPIU_Assert(0);
        SET_EX_RD_HANDLER(lconn_hnd, listen_success_handler, quiescent_handler);
        hr = MPID_Nem_nd_lconn_hnd->p_conn->Accept(new_conn_hnd->p_ep, NULL, 0, MPIU_EX_GET_OVERLAPPED_PTR(&(MPID_Nem_nd_lconn_hnd->recv_ov)));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POST_ACCEPT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_listen_for_conn
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_listen_for_conn(int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS, ret, use_default_interface=0;
    SIZE_T len;
    HRESULT hr;
    char *buf;
    int i;
    unsigned short port;
    char tmp_buf[MAX_HOST_DESCRIPTION_LEN];
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_LISTEN_FOR_CONN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_LISTEN_FOR_CONN);

    /* Create listen conn */
    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_LISTEN_CONN, NULL, NULL, &MPID_Nem_nd_lconn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Listen for connections */
    hr = MPID_Nem_nd_dev_hnd_g->p_ad->Listen(0, MPID_NEM_ND_PROT_FAMILY, 0, &port, &(MPID_Nem_nd_dev_hnd_g->p_listen));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    ret = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_PORT_KEY, port);
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");

    SET_EX_RD_HANDLER(MPID_Nem_nd_lconn_hnd, listen_success_handler, quiescent_handler);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting first req for conn on lconn(%p)on ov(%p)",
           MPID_Nem_nd_lconn_hnd, MPIU_EX_GET_OVERLAPPED_PTR(&(MPID_Nem_nd_lconn_hnd->recv_ov))));

    /* FIXME: How many conn requests should we pre-post ? */
    hr = MPID_Nem_nd_dev_hnd_g->p_listen->GetConnectionRequest(MPID_Nem_nd_lconn_hnd->p_conn, MPIU_EX_GET_OVERLAPPED_PTR(&(MPID_Nem_nd_lconn_hnd->recv_ov)));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted connection request");
    /* Create Business Card */
    use_default_interface = 1;
    /* MPICH_INTERFACE_HOSTNAME & MPICH_INTERFACE_HOSTNAME_R%d env vars
     * override the default interface selected
     * FIXME: Do we need to change the override env var names ?
     */
    ret = getenv_s((size_t *)&len, NULL, 0, "MPICH_INTERFACE_HOSTNAME");
    if(ret == ERANGE){
        MPIU_Assert(sizeof(tmp_buf) >= len);
        ret = getenv_s((size_t *)&len, tmp_buf, sizeof(tmp_buf), "MPICH_INTERFACE_HOSTNAME");
        if(ret == 0){
            use_default_interface = 0;
            ret = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_INTERFACE_KEY, tmp_buf);
            MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }

    MPIU_Snprintf(tmp_buf, sizeof(tmp_buf), "MPICH_INTERFACE_HOSTNAME_R%d", pg_rank);
    ret = getenv_s((size_t *)&len, NULL, 0, tmp_buf);
    if(ret == ERANGE){
        buf = MPIU_Strdup(tmp_buf);
        MPIU_Assert(sizeof(tmp_buf) >= len);
        ret = getenv_s((size_t *)&len, tmp_buf, sizeof(tmp_buf), buf);
        MPIU_Free(buf);
        if(ret == 0){
            use_default_interface = 0;
            ret = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_INTERFACE_KEY, tmp_buf);
            MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }

    if(use_default_interface){
        buf = inet_ntoa(MPID_Nem_nd_dev_hnd_g->s_addr_in.sin_addr);
        MPIU_ERR_CHKANDJUMP2((buf == NULL),
            mpi_errno, MPI_ERR_OTHER, "**inet_ntoa", "**inet_ntoa %s %d",
            MPIU_OSW_Strerror(MPIU_OSW_Get_errno()), MPIU_OSW_Get_errno());

        MPIU_Strncpy(tmp_buf, buf, sizeof(tmp_buf));
        ret = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_INTERFACE_KEY, tmp_buf);
        MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Listening on %s:%d", tmp_buf, port));

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_LISTEN_FOR_CONN);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Wait for discing until the other side sends us some data or disconnects */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_passive_disc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_passive_disc(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_PASSIVE_DISC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_PASSIVE_DISC);

    if(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd)){
        int i=0;
        /* Make the conn an orphan */
        conn_hnd->vc = NULL;
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
        SET_EX_RD_HANDLER(conn_hnd, dummy_handler, dummy_handler);

        /* Set the recv sbuf handlers to quiescent msg handlers - the conn is disconnected
         * after we receive a CACK/CNAK, i.e., some data, on this conn
         */
        for(i=0;i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
            SET_MSGBUF_HANDLER(&((conn_hnd->rsbuf[i]).msg), quiescent_msg_handler, quiescent_msg_handler);
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_PASSIVE_DISC);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Start disconnecting the nd conn corresponding to vc
 * Free resources once we disconnect
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_disc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_disc(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_DISC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_DISC);

    if(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd)){
        int i=0;
        /* Make the conn an orphan */
        conn_hnd->vc = NULL;
        conn_hnd->tmp_vc = NULL;

        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
        SET_EX_WR_HANDLER(conn_hnd, quiescent_handler, quiescent_handler);

        /* FIXME: DEREGISTER ALL RECV BUFS HERE ...*/
        /* Set the recv sbuf handlers to dummy handlers */
        for(i=0;i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
            SET_MSGBUF_HANDLER(&((conn_hnd->rsbuf[i]).msg), dummy_msg_handler, dummy_msg_handler);
        }

        MPIU_Assert(conn_hnd->npending_ops == 0);
        MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "Posting disconnect on conn(%p)", conn_hnd);
    
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting disc on conn(%p) on ov(%p)",
           conn_hnd, MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov))));

        /* Post disconnect on the ND Conn corresponding to VC */
        hr = conn_hnd->p_conn->Disconnect(MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_disc", "**nd_disc %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_DISC);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* ND post send/recv utils */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_send_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_send_msg(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg, SIZE_T msg_len, int is_blocking)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    ND_SGE sge;
    MPID_Nem_nd_block_op_hnd_t op_hnd;
    MPID_Nem_nd_msg_result_t *pmsg_result;
    ND_RESULT *pnd_result;
    BOOL was_fc_pkt;
    MPIU_CHKPMEM_DECL(1);

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_SEND_MSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_SEND_MSG);
    
    was_fc_pkt = (MPID_NEM_ND_IS_FC_PKT(pmsg->hdr.type)) ? TRUE : FALSE;

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Post send msg [conn=%p, on/msg = %p, sz=%d]",conn_hnd, pmsg, msg_len));

    /* Update FC info */
    mpi_errno = MPID_Nem_nd_update_fc_info(conn_hnd, pmsg);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if(is_blocking){
        /* FIXME: Allow blocking sends */
        MPIU_Assert(0);
		mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd, 1, conn_hnd, 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_CHKPMEM_MALLOC(pmsg_result, MPID_Nem_nd_msg_result_t *, sizeof(MPID_Nem_nd_msg_result_t ), mpi_errno, "block send op result");
        INIT_MSGRESULT(pmsg_result, free_msg_result_handler, free_msg_result_handler);
        pnd_result = &(pmsg_result->result);
    }
    else{
        pnd_result = GET_PNDRESULT_FROM_MSGBUF(pmsg);
    }

    /* The msg packet is already formed we just need to send the data */
    sge.Length = msg_len;
    sge.pAddr = pmsg;
    sge.hMr = conn_hnd->ssbuf_hmr;

    hr = conn_hnd->p_ep->Send(pnd_result, &sge, 1, ND_OP_FLAG_READ_FENCE);
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

	/* Increment the number of pending ops on conn */
	/* conn_hnd->npending_ops++; */

    if(is_blocking){
		/* Block till all current pending ops complete */
		mpi_errno = MPID_Nem_nd_sm_block(op_hnd);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

		/* No pending ops */
		MPIU_Assert(conn_hnd->npending_ops == 0);

        MPIU_CHKPMEM_COMMIT();
    }

    if(was_fc_pkt){
        if(conn_hnd->send_in_progress){
            if(!conn_hnd->zcp_in_progress){
                MPID_NEM_ND_CONN_DECR_CACHE_SCREDITS(conn_hnd);
            }
            /* ZCP packets are not flow controlled */
        }
        else{
            MPID_NEM_ND_CONN_DECR_SCREDITS(conn_hnd);
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POST_SEND_MSG);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_update_fc_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_update_fc_info(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_UPDATE_FC_INFO);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_UPDATE_FC_INFO);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Send credits = %d, Recv credits = %d", conn_hnd->send_credits, conn_hnd->recv_credits));    
    if(pmsg != NULL){
        /* Note: When updating msg with fc info never modify fc info for
         * the connection 
         */
        if(conn_hnd->fc_pkt_pending){
            /* Update the msg with fc info */
            pmsg->hdr.credits = MPID_NEM_ND_CONN_FC_RW;
            conn_hnd->fc_pkt_pending = 0;
        }
        /* If no fc pkt pending don't touch the msg */
    }
    else{
        /* Update the conn with fc info */
        if(MPID_NEM_ND_CONN_FC_RW_AVAIL(conn_hnd)){
            conn_hnd->fc_pkt_pending = 1;
            conn_hnd->recv_credits = 0;
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_UPDATE_FC_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_recv_msg(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    ND_SGE sge;
    BOOL was_fc_pkt;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_RECV_MSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_RECV_MSG);

    was_fc_pkt = (MPID_NEM_ND_IS_FC_PKT(pmsg->hdr.type)) ? TRUE : FALSE;

    /* The msg packet is already formed we just need to send the data */
    sge.Length = sizeof(MPID_Nem_nd_msg_t );
    sge.pAddr = pmsg;
    sge.hMr = conn_hnd->rsbuf_hmr;
    hr = conn_hnd->p_ep->Receive(GET_PNDRESULT_FROM_MSGBUF(pmsg), &sge, 1);
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

    /* The recv credits have to be updated before updating
     * the fc info
     */
    if(was_fc_pkt){
        /* We increment recv credits each time we repost a recv 
         * for a flow controlled pkt
         */
        MPID_NEM_ND_CONN_INCR_RCREDITS(conn_hnd);
    }

    if(conn_hnd->fc_pkt_pending){
		/* Use fc virtual conn to send flow control pkt 
		 * Since we received FC_RW+1 pkts we are guaranteed
		 * that the fc virtual conn is available
		 */
        MPID_Nem_nd_msg_t *pfc_msg;

        MSGBUF_FREEQ_DEQUEUE(conn_hnd, pfc_msg);
        MPIU_Assert(pfc_msg != NULL);
        SET_MSGBUF_HANDLER(pfc_msg, netmod_msg_send_success_handler, gen_send_fail_handler);

        pfc_msg->hdr.type = MPID_NEM_ND_CRED_PKT;

        /* FIXME: fc info in pkt is updated for every msg sent.
         * Do we have to explicitly update fc info here ?
         */
        mpi_errno = MPID_Nem_nd_update_fc_info(conn_hnd, pfc_msg);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Sending CRED PKT...");
        mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pfc_msg, sizeof(MPID_Nem_nd_msg_hdr_t ), 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        mpi_errno = MPID_Nem_nd_update_fc_info(conn_hnd, NULL);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POST_RECV_MSG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME bind_mw_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int bind_mw_success_handler(MPID_Nem_nd_msg_result_t *zcp_send_result)
{
    int mpi_errno = MPI_SUCCESS;
    int ret_errno;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_pack_t pack_type = MPID_NEM_ND_INVALID_PACK;
    MPID_Nem_nd_msg_t *pmsg;
    MPID_Request *zcp_req = NULL;
    int i;
    SIZE_T nb, msg_len, rem_len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_ZCP_SEND_MSGRESULT(zcp_send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    zcp_req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "bind succ for IOV[%d] = %p; iov_offset = %d/rem=%d, req=%p",
        conn_hnd->zcp_send_offset, zcp_req->dev.iov[conn_hnd->zcp_send_offset].MPID_IOV_BUF,
        zcp_req->dev.iov_offset, zcp_req->dev.iov_count, zcp_req));
    MPIU_Assert(zcp_req->dev.iov_offset + zcp_req->dev.iov_count <= MPID_IOV_LIMIT);
    MPIU_Assert(zcp_req->dev.iov_count > 0);

    MPIU_Assert(!MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd));

    /* Post the ND message 
     * iov[zcp_req->dev.iov_offset, conn_hnd->zcp_send_offset]
     * First pack any non-ZCP IOVs, then copy zcp send mw & send
     */
    MSGBUF_FREEQ_DEQUEUE(conn_hnd, pmsg);
    MPIU_Assert(pmsg != NULL);
    
    SET_MSGBUF_HANDLER(pmsg, zcp_mw_send_success_handler, gen_send_fail_handler);
    pmsg->hdr.type = MPID_NEM_ND_RD_AVAIL_PKT;
    pmsg->hdr.credits = 0;
    msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
    rem_len = sizeof(pmsg->buf);

    nb = 0;
    if(zcp_req->dev.iov_offset < conn_hnd->zcp_send_offset){
        int off_end;
        off_end = conn_hnd->zcp_send_offset;
            
        /* Piggy-back IOVs */
        mpi_errno = MPID_Nem_nd_pack_iov(conn_hnd,
                        zcp_req->dev.iov,
                        zcp_req->dev.iov_offset,
                        &off_end,
                        pmsg,
                        &(pack_type),
                        &nb);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_Assert(pack_type == MPID_NEM_ND_SR_PACK);

        msg_len += nb;
        rem_len -= nb;
    }

    /* Now copy the MSG MW to the packet */
    MPIU_Assert(rem_len >= sizeof(MPID_Nem_nd_msg_mw_t ));
    ret_errno = memcpy_s((void *)&(pmsg->buf[nb]), rem_len, &(conn_hnd->zcp_msg_send_mw), sizeof(MPID_Nem_nd_msg_mw_t ));
    MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,    
        "**nd_write", "**nd_write %s %d", strerror(ret_errno), ret_errno);

    msg_len += sizeof(MPID_Nem_nd_msg_mw_t );
    rem_len -= sizeof(MPID_Nem_nd_msg_mw_t );

    /* Block on progress engine if we exceed the number of RDs allowed on the conn/device */
    while(MPID_Nem_nd_dev_hnd_g->npending_rds >= 2){
        mpi_errno = MPID_Nem_nd_sm_poll(0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    MPID_Nem_nd_dev_hnd_g->npending_rds++; conn_hnd->npending_rds++;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "dev prds = %d; conn prds = %d",
        MPID_Nem_nd_dev_hnd_g->npending_rds, conn_hnd->npending_rds));


    for(i=zcp_req->dev.iov_offset; i<conn_hnd->zcp_send_offset; i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending SR packed IOV[off=%d/tot_iovs=%d]=[%p/%u]",
        i, zcp_req->dev.iov_count,
        zcp_req->dev.iov[i].MPID_IOV_BUF,
        zcp_req->dev.iov[i].MPID_IOV_LEN));

    }
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending mem descriptor (buf=%p) : base = %p, length=%I64d, token=%d, mw=%p",
                zcp_req->dev.iov[conn_hnd->zcp_send_offset].MPID_IOV_BUF,
                _byteswap_uint64(conn_hnd->zcp_msg_send_mw.mw_data.Base),
                _byteswap_uint64(conn_hnd->zcp_msg_send_mw.mw_data.Length),
                conn_hnd->zcp_msg_send_mw.mw_data.Token,
                conn_hnd->zcp_send_mw));

    mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, msg_len, 0);
	if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if(zcp_req->dev.iov[conn_hnd->zcp_send_offset].MPID_IOV_LEN == 0){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "zcp_req(%p) off[%d -> %d], cnt[%d -> %d]",
            zcp_req, zcp_req->dev.iov_offset, conn_hnd->zcp_send_offset + 1,
            zcp_req->dev.iov_count, zcp_req->dev.iov_count - (conn_hnd->zcp_send_offset - zcp_req->dev.iov_offset + 1)));
        /* Rem IOVs */
        zcp_req->dev.iov_count -= (conn_hnd->zcp_send_offset - zcp_req->dev.iov_offset + 1);

        zcp_req->dev.iov_offset = conn_hnd->zcp_send_offset + 1;
    }
    else{
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "zcp_req(%p) off[%d -> %d], cnt[%d -> %d]",
            zcp_req, zcp_req->dev.iov_offset, conn_hnd->zcp_send_offset,
            zcp_req->dev.iov_count, zcp_req->dev.iov_count - (conn_hnd->zcp_send_offset - zcp_req->dev.iov_offset)));
        /* Rem IOVs */
        zcp_req->dev.iov_count -= (conn_hnd->zcp_send_offset - zcp_req->dev.iov_offset);

        zcp_req->dev.iov_offset = conn_hnd->zcp_send_offset;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME reg_zcp_reg_sreq_bind_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int reg_zcp_reg_sreq_bind_handler(MPIU_EXOVERLAPPED *send_ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_result_t *pmsg_result;
    MPID_Request *zcp_req = NULL;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_REG_SREQ_BIND_HANDLER);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_REG_SREQ_BIND_HANDLER);

    conn_hnd = GET_CONNHND_FROM_EX_SEND_OV(send_ov);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    zcp_req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "About to bind IOV[%d] = %p; iov_offset = %d/tot=%d, req=%p",
        conn_hnd->zcp_send_offset, zcp_req->dev.iov[conn_hnd->zcp_send_offset].MPID_IOV_BUF,
        zcp_req->dev.iov_offset, zcp_req->dev.iov_count, zcp_req));
    MPIU_Assert(zcp_req->dev.iov_offset + zcp_req->dev.iov_count <= MPID_IOV_LIMIT);
    MPIU_Assert(zcp_req->dev.iov_count > 0);  

    /* Create Memory Window for sending data */
    MPIU_CHKPMEM_MALLOC(pmsg_result, MPID_Nem_nd_msg_result_t *, 
        sizeof(MPID_Nem_nd_msg_result_t ), mpi_errno, "cr_mem_win result");

    INIT_MSGRESULT(pmsg_result, free_msg_result_handler, free_msg_result_handler);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->CreateMemoryWindow(&(pmsg_result->result), &(conn_hnd->zcp_send_mw));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_CHKPMEM_COMMIT();

    /* Initialize the MW descriptor to be sent in the ND message */
    conn_hnd->zcp_msg_send_mw.mw_data.Base = 0;
    conn_hnd->zcp_msg_send_mw.mw_data.Length = 0;
    conn_hnd->zcp_msg_send_mw.mw_data.Token = 0;

    INIT_MSGRESULT(&(conn_hnd->zcp_send_result), bind_mw_success_handler, gen_send_fail_handler);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Binding IOV[%d/tot=%d]=[%p/%u], dev_off=%d, mr=%x, mw=%p, conn=%p",
        conn_hnd->zcp_send_offset, 
        zcp_req->dev.iov_count,
        conn_hnd->zcp_send_sge.pAddr,
        conn_hnd->zcp_send_sge.Length,
        zcp_req->dev.iov_offset,
        conn_hnd->zcp_send_mr_hnd,
        conn_hnd->zcp_send_mw, conn_hnd));

    hr = conn_hnd->p_ep->Bind(&(conn_hnd->zcp_send_result.result), 
            conn_hnd->zcp_send_mr_hnd,
            conn_hnd->zcp_send_mw,
            conn_hnd->zcp_send_sge.pAddr,
            conn_hnd->zcp_send_sge.Length, 
            (ND_OP_FLAG_READ_FENCE | ND_OP_FLAG_ALLOW_READ), 
            &(conn_hnd->zcp_msg_send_mw.mw_data));

    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_REG_SREQ_BIND_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Register mem for sreq */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_zcp_reg_smem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_zcp_reg_smem(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_IOV *iovp, int iov_offset)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_REG_SMEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_REG_SMEM);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Registering IOV[%d]=%p/%u (conn=%p)",
        iov_offset, iovp[iov_offset].MPID_IOV_BUF, iovp[iov_offset].MPID_IOV_LEN, conn_hnd));

    /* Keep track of the zcp send offset */
    conn_hnd->zcp_send_offset = iov_offset;

    conn_hnd->zcp_send_sge.hMr = NULL;
    if(iovp[iov_offset].MPID_IOV_LEN > MPID_NEM_ND_DEV_IO_LIMIT(MPID_Nem_nd_dev_hnd_g)){
        conn_hnd->zcp_send_sge.pAddr = iovp[iov_offset].MPID_IOV_BUF;
        conn_hnd->zcp_send_sge.Length = MPID_NEM_ND_DEV_IO_LIMIT(MPID_Nem_nd_dev_hnd_g);

        iovp[iov_offset].MPID_IOV_BUF += MPID_NEM_ND_DEV_IO_LIMIT(MPID_Nem_nd_dev_hnd_g);
        iovp[iov_offset].MPID_IOV_LEN -= MPID_NEM_ND_DEV_IO_LIMIT(MPID_Nem_nd_dev_hnd_g);
    }
    else{
        conn_hnd->zcp_send_sge.pAddr = iovp[iov_offset].MPID_IOV_BUF;
        conn_hnd->zcp_send_sge.Length = iovp[iov_offset].MPID_IOV_LEN;

        iovp[iov_offset].MPID_IOV_LEN = 0;
    }

    MPIU_Assert(!conn_hnd->zcp_in_progress);
    SET_EX_WR_HANDLER(conn_hnd, reg_zcp_reg_sreq_bind_handler, gen_ex_fail_handler);

    memset(&(conn_hnd->zcp_send_mr_hnd), 0x0, sizeof(conn_hnd->zcp_send_mr_hnd));

    /* Register buffer */
    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(
        conn_hnd->zcp_send_sge.pAddr,
        conn_hnd->zcp_send_sge.Length,
        MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)),
        &(conn_hnd->zcp_send_mr_hnd));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_REG_SMEM);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Both start and end offsets returned are valid offsets */
static inline MPID_Nem_nd_pack_t nd_pack_iov_get_params(MPID_IOV *iovp, int start, int *end){
    u_long rem_len = MPID_NEM_ND_CONN_UDATA_SZ;
    int i;
    for(i = start; (i < *end) && (rem_len >= iovp[i].MPID_IOV_LEN); i++){
        rem_len -= iovp[i].MPID_IOV_LEN;
    }
    if(i == *end){
        /* All IOVs can be packed */
        *end = i - 1;
        return MPID_NEM_ND_SR_PACK;
    }
    else if(iovp[i].MPID_IOV_LEN <= MPID_NEM_ND_CONN_UDATA_SZ){
        *end = i - 1;
        return MPID_NEM_ND_SR_PACK;
    }
    else{
        /* One more IOV can be packed using ZCP packing */
        *end = i;
        return MPID_NEM_ND_ZCP_PACK;
    }
}

/* Input:
 * offset_start => the start req offset for packing
 * offset_endp => the max req offset for packing, (offset_endp-1) is the max valid offset
 * Output:
 * offset_endp => Used to return the final req offset packed
 * pack_typep => Used to return the packing type
 * nbp => Used to return bytes packed
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_pack_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_pack_iov(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_IOV *iovp,
                         int offset_start,
                         int *offset_endp,
                         MPID_Nem_nd_msg_t *pmsg,
                         MPID_Nem_nd_pack_t *pack_typep,
                         SIZE_T *nbp){
    int mpi_errno = MPI_SUCCESS;
    int ret_errno;
    int off;
    char *p;
    SIZE_T rem_len;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_PACK_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_PACK_IOV);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));
    MPIU_Assert(iovp != NULL);
    MPIU_Assert(offset_endp != NULL);
    MPIU_Assert(offset_start >= 0);
    MPIU_Assert(offset_start <= *offset_endp);
    MPIU_Assert(pack_typep != NULL);
    MPIU_Assert(pmsg != NULL);
    MPIU_Assert(nbp != NULL);

    off = *offset_endp;
    *pack_typep = nd_pack_iov_get_params(iovp, offset_start, &off);
    MPIU_Assert(off < *offset_endp);
    *offset_endp = off;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "PACK_TYPE=%d - [%d, %d]", *pack_typep, offset_start, *offset_endp));

    p = pmsg->buf;
    rem_len = MPID_NEM_ND_CONN_UDATA_SZ;

    if(*pack_typep == MPID_NEM_ND_SR_PACK){
        /* Note that both start and end offsets returned by nd_pack_iov_get_params()
         * are valid/packable offsets
         */
        for(off = offset_start; off <= *offset_endp; off++){
            MPIU_Assert(rem_len >= iovp[off].MPID_IOV_LEN);
            ret_errno = memcpy_s((void *)p, rem_len,
                            iovp[off].MPID_IOV_BUF, iovp[off].MPID_IOV_LEN);
	    	MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,
		    	"**nd_write", "**nd_write %s %d", strerror(ret_errno), ret_errno);

            p += iovp[off].MPID_IOV_LEN;
            rem_len -= iovp[off].MPID_IOV_LEN;
        }
        *nbp = MPID_NEM_ND_CONN_UDATA_SZ - rem_len;
    }
    else if(*pack_typep == MPID_NEM_ND_ZCP_PACK){
        /* We are not going to use this msg right now */
        MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

        MPID_Nem_nd_dev_hnd_g->zcp_pending = 1;

        mpi_errno = MPID_Nem_nd_zcp_reg_smem(conn_hnd, iovp, *offset_endp);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        *nbp = 0;
    }
    else{
        /* Unrecognized packing type */
        MPIU_Assert(0);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_PACK_IOV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_sendbv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_sendbv(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *sreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_SENDBV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_SENDBV);

    MPIU_Assert(!conn_hnd->send_in_progress);

    conn_hnd->send_in_progress = 1;
    conn_hnd->cache_credits = conn_hnd->send_credits;
    conn_hnd->send_credits = 0;

    mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POST_SENDBV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_sendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_sendv(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *sreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_msg_t *pmsg;
    SIZE_T msg_len = 0, nb;
    MPID_Nem_nd_pack_t pack_type;
    int offset_end, i;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_SENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_SENDV);

    MPIU_Assert((conn_hnd->send_credits > 0) ? (!conn_hnd->send_in_progress) : 1);

    MSGBUF_FREEQ_DEQUEUE(conn_hnd, pmsg);
    MPIU_Assert(pmsg != NULL);
    SET_MSGBUF_HANDLER(pmsg, send_success_handler, gen_send_fail_handler);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting send on [conn = %p] for ", conn_hnd));

    for(i=0; i<sreqp->dev.iov_count; i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "IOV[%d] = {%p, %u}",
            sreqp->dev.iov_offset + i,
            sreqp->dev.iov[sreqp->dev.iov_offset + i].MPID_IOV_BUF,
            sreqp->dev.iov[sreqp->dev.iov_offset + i].MPID_IOV_LEN));
    }

    offset_end = sreqp->dev.iov_offset + sreqp->dev.iov_count;
    mpi_errno = MPID_Nem_nd_pack_iov(conn_hnd, sreqp->dev.iov,
                    sreqp->dev.iov_offset, &offset_end, pmsg, &pack_type, &nb);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                    
    if(pack_type == MPID_NEM_ND_SR_PACK){
	    pmsg->hdr.type = MPID_NEM_ND_DATA_PKT;
        pmsg->hdr.credits = 0;
        msg_len = sizeof(MPID_Nem_nd_msg_hdr_t ) + nb;

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "SR PACKING: Sending msg packet of size %d (msg type=%d)", msg_len, pmsg->hdr.type));
        if(sreqp->dev.iov_offset == 0){
	        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending udata packet of type  = %d", ((MPIDI_CH3_Pkt_t *)(&(pmsg->buf)))->type));
        }
        else{
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Contd to send data on req=%p [iov=%d/tot=%d]",
                sreqp, sreqp->dev.iov_offset, sreqp->dev.iov_count));
        }

	    mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, msg_len, 0);
	    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* Packing always consumes whole IOVs */
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "SR pack : sreq(%p) off %d -> %d", sreqp, sreqp->dev.iov_offset, sreqp->dev.iov_offset + 1));

        sreqp->dev.iov_count -= (offset_end - sreqp->dev.iov_offset + 1);
        sreqp->dev.iov_offset = offset_end + 1;
        if(sreqp->dev.iov_count > 0){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Could not pack all IOVs (rem = %d iovs)- SEND_IN_PROGRESS...", sreqp->dev.iov_count);
            if(!conn_hnd->send_in_progress){
                /* Could not pack all IOVs - Block all subsequent sends */
                conn_hnd->send_in_progress = 1;
                /* Queue data till the zcpy is over - keep track of the send credits */
                conn_hnd->cache_credits = conn_hnd->send_credits;
                conn_hnd->send_credits = 0;
            }
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Finished posting sends for all IOVs...");
        }
    }
    else if(pack_type == MPID_NEM_ND_ZCP_PACK){
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ZCP PACKING - SEND_IN_PROGRESS...");
        conn_hnd->zcp_in_progress = 1;
        if(!conn_hnd->send_in_progress){
            /* The progress engine ZCP handlers will send the data */
            conn_hnd->send_in_progress = 1;
            /* Queue data till the zcpy is over - keep track of the send credits */
            conn_hnd->cache_credits = conn_hnd->send_credits;
            conn_hnd->send_credits = 0;
        }
    }
    else{
        /* Unrecognized pack type */
        MPIU_Assert(0);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_POST_SENDV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* ND progress utils */
/* FIXME: Integrate this func with Executive */
#define FUNCNAME MPID_Nem_nd_process_completions
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_process_completions(INDCompletionQueue *pcq, BOOL *pstatus)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_PROCESS_COMPLETIONS);

    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_PROCESS_COMPLETIONS); */

    MPIU_Assert(pcq != NULL);
    MPIU_Assert(pstatus != NULL);

    for (;;){
        int nresults = 0;
        ND_RESULT *nd_results[1];
        HRESULT hr;
        MPID_Nem_nd_msg_result_t *pmsg_result;
        MPID_Nem_nd_msg_handler_t handler_fn;

        nresults = pcq->GetResults(nd_results, 1);

        if(nresults == 0){
			/* No pending op in cq */
			*pstatus = FALSE;
			break;
        }
        /* An Event completed */
        *pstatus = TRUE;

        hr = nd_results[0]->Status;
        pmsg_result = GET_MSGRESULT_FROM_NDRESULT(nd_results[0]);
        MPIU_Assert(pmsg_result != NULL);
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Got something on %p", GET_MSGBUF_FROM_MSGRESULT(pmsg_result)));
        if(hr == ND_SUCCESS){
            handler_fn = pmsg_result->succ_fn;
        }
        else{
            handler_fn = pmsg_result->fail_fn;
        }
        MPIU_Assert(handler_fn != NULL);
        return handler_fn(pmsg_result);
    }
 fn_exit:
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_PROCESS_COMPLETIONS); */
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* The handlers are named as [ND_STATE]_[SUCCESS/FAIL]_handler
 * Since Executive is not compatible with ND we use Ex to process
 * completions for connect/accept/close and ND handlers for send/recv/read
 */
/* Generic send/recv fail handlers */
#undef FUNCNAME
#define FUNCNAME gen_send_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int gen_send_fail_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_GEN_SEND_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_GEN_SEND_FAIL_HANDLER);

    hr = GET_STATUS_FROM_MSGRESULT(send_result);
    MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**nd_write",
        "**nd_write %s %d", _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_GEN_SEND_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;}

#undef FUNCNAME
#define FUNCNAME gen_recv_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int gen_recv_fail_handler(MPID_Nem_nd_msg_result_t *recv_result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_GEN_RECV_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_GEN_RECV_FAIL_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(conn_hnd));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Recv failed on %p(state=%d)", conn_hnd, conn_hnd->state));
    hr = GET_STATUS_FROM_MSGRESULT(recv_result);
    MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**nd_read",
        "**nd_read %s %d", _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_GEN_RECV_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_read_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_read_fail_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_FAIL_HANDLER);

    hr = GET_STATUS_FROM_MSGRESULT(send_result);
    MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**nd_read",
        "**nd_read %s %d", _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME dummy_msg_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int dummy_msg_handler(MPID_Nem_nd_msg_result_t *result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_DUMMY_MSG_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_DUMMY_MSG_HANDLER);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_DUMMY_MSG_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME quiescent_msg_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int quiescent_msg_handler(MPID_Nem_nd_msg_result_t *result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_msg_t *pmsg;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_MSG_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_MSG_HANDLER);

    pmsg = GET_MSGBUF_FROM_MSGRESULT(result);
    MPIU_Assert(pmsg != NULL);

    conn_hnd = GET_CONNHND_FROM_MSGBUF(pmsg);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

    /* A pending op completed on this conn - rd/wr - go ahead and 
     * disconnect the conn
     */
    mpi_errno = MPID_Nem_nd_conn_disc(conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_MSG_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME free_msg_result_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int free_msg_result_handler(MPID_Nem_nd_msg_result_t *result)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_FREE_MSG_RESULT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_FREE_MSG_RESULT_HANDLER);

    MPIU_Free(result);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_FREE_MSG_RESULT_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME trim_nd_sge
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void trim_nd_sge(ND_SGE *nd_sge, int *nd_sge_count, int *nd_sge_offset, SIZE_T nb)
{
    ND_SGE *nd_sge_p;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_TRIM_ND_SGE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_TRIM_ND_SGE);
    MPIU_Assert(nd_sge != NULL);
    MPIU_Assert(nd_sge_count != NULL);
    MPIU_Assert(*nd_sge_count > 0);
    MPIU_Assert(nd_sge_offset != NULL);
    MPIU_Assert(*nd_sge_offset < MPID_IOV_LIMIT);
    MPIU_Assert(nb >= 0);

    nd_sge_p = &(nd_sge[*nd_sge_offset]);
    while(nb){
        MPIU_Assert(*nd_sge_count > 0);
        if(nb < nd_sge_p->Length){
            /* We never read partial nd_sges */
            MPIU_Assert(0);
            nd_sge_p->pAddr = (char *)(nd_sge_p->pAddr) + nb;
            nd_sge_p->Length -= nb;
            nb = 0;
        }
        else{
            *nd_sge_count -= 1;
            *nd_sge_offset += 1;
            nb -= nd_sge_p->Length;
            nd_sge_p++;
        }
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_TRIM_ND_SGE);
}

/* The function modifies ND_MW - don't use it if you need to use the MW again (eg: invalidate) */
#undef FUNCNAME
#define FUNCNAME trim_nd_mw
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline void trim_nd_mw(ND_MW_DESCRIPTOR *nd_mw, int *nd_mw_count, int *nd_mw_offset, SIZE_T nb)
{
    ND_MW_DESCRIPTOR *nd_mw_p;
    uint64_t len = 0;
    uint64_t base = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_TRIM_ND_MW);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_TRIM_ND_MW);

    MPIU_Assert(nd_mw != NULL);
    MPIU_Assert(nd_mw_count != NULL);
    MPIU_Assert(*nd_mw_count > 0);
    MPIU_Assert(nd_mw_offset != NULL);
    MPIU_Assert(*nd_mw_offset < MPID_IOV_LIMIT);
    MPIU_Assert(nb >= 0);

    nd_mw_p = &(nd_mw[*nd_mw_offset]);
    while(nb){
        MPIU_Assert(*nd_mw_count > 0);
        len = _byteswap_uint64(nd_mw_p->Length);

        if(nb < len){
            base = _byteswap_uint64(nd_mw_p->Base);
            nd_mw_p->Base = _byteswap_uint64(base + nb);
            len -= nb;
            nd_mw_p->Length = _byteswap_uint64(len);
            nb = 0;
        }
        else{
            *nd_mw_count -= 1;
            *nd_mw_offset += 1;
            nb -= len;
            nd_mw_p->Length = 0;
            nd_mw_p++;
        }
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_TRIM_ND_MW);
}

/* This function trims the iov array, *iov_p, of size *n_iov_p
 * assuming nb bytes are transferred
 * Side-effect : *iov_p, *n_iov_p, buf & len of (*iov_p)
 *  could be modified by this function.
 * Returns the number of bytes copied
 */
#undef FUNCNAME
#define FUNCNAME copy_and_trim_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static SIZE_T copy_and_trim_iov(MPID_IOV *iov, int *n_iov_p, int *offset_p, char *buf, SIZE_T nb)
{
    MPID_IOV *cur_iov;
    int cur_n_iov, cur_offset;
    SIZE_T buflen;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_COPY_AND_TRIM_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_COPY_AND_TRIM_IOV);

    MPIU_Assert(iov != NULL);
    MPIU_Assert(n_iov_p);
    MPIU_Assert((*n_iov_p) > 0);
    MPIU_Assert(offset_p != NULL);
    MPIU_Assert((*offset_p >= 0) && (*offset_p < MPID_IOV_LIMIT));
    MPIU_Assert(buf != NULL);

    cur_n_iov = *n_iov_p;
    cur_offset = *offset_p;
    cur_iov = &(iov[cur_offset]);

    buflen = nb;

    while(nb > 0){
        if(nb < cur_iov->MPID_IOV_LEN){
            memcpy_s(cur_iov->MPID_IOV_BUF, cur_iov->MPID_IOV_LEN, buf, nb);
            buf += nb;
            cur_iov->MPID_IOV_BUF += nb;
            cur_iov->MPID_IOV_LEN -= nb;
            nb = 0;
        }
        else{
            memcpy_s(cur_iov->MPID_IOV_BUF, cur_iov->MPID_IOV_LEN, buf, cur_iov->MPID_IOV_LEN);
            buf += cur_iov->MPID_IOV_LEN;
            nb -= cur_iov->MPID_IOV_LEN;
            cur_iov->MPID_IOV_LEN = 0;
            cur_n_iov--;
            cur_offset++;
            if(cur_n_iov > 0){
                cur_iov++;
            }
            else{
                /* More data available in the buffer than can be copied 
                 * The return value indicates the number of bytes copied
                 */
                break;
            }
        }
    }

    *n_iov_p = cur_n_iov;
    *offset_p = cur_offset;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_COPY_AND_TRIM_IOV);
    /* Bytes processed/trimmed */
    return (buflen - nb);
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_handle_recv_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_Nem_nd_handle_recv_req(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *rreqp, int *req_complete)
{
    int mpi_errno = MPI_SUCCESS;
    int (*req_fn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_HANDLE_RECV_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_HANDLE_RECV_REQ);

    MPIU_Assert(rreqp != NULL);
    MPIU_Assert(req_complete != NULL);

    *req_complete = 0;

    req_fn = rreqp->dev.OnDataAvail;
    if(req_fn){
        mpi_errno = req_fn(conn_hnd->vc, rreqp, req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        if (*req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... Not complete");
            rreqp->dev.iov_offset = 0;
        }
    }
    else{
        MPIDI_CH3U_Request_complete(rreqp);
        *req_complete = 1;
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_HANDLE_RECV_REQ);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME nd_read_progress_update
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int nd_read_progress_update(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Request *rreq, char *buf, SIZE_T *pnb, int *req_complete)
{
    int mpi_errno = MPI_SUCCESS;
    SIZE_T buflen, nb;
    MPID_IOV *iov;
    int complete;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_READ_PROGRESS_UPDATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_READ_PROGRESS_UPDATE);

    MPIU_Assert(rreq != NULL);
    MPIU_Assert(buf != NULL);
    MPIU_Assert(req_complete != NULL);
    MPIU_Assert((pnb != NULL) && (*pnb > 0));

    *req_complete = 0;
    buflen = *pnb;
    do{
        *req_complete = 0;

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "trim rreq(%p) off %d/tot=%d", 
            rreq, rreq->dev.iov_offset, rreq->dev.iov_count));

        if(rreq->dev.iov_count != 0){
            iov = rreq->dev.iov;
            nb = copy_and_trim_iov(iov, &(rreq->dev.iov_count), &(rreq->dev.iov_offset), buf, buflen);
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Copied %d bytes...[rem iovs = %d]", nb, rreq->dev.iov_count));

            buf += nb;
            buflen -= nb;
        }

        complete = (rreq->dev.iov_count == 0) ? 1 : 0;

        if(complete){
            mpi_errno = MPID_Nem_nd_handle_recv_req(conn_hnd, rreq, req_complete);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }while((buflen > 0) && !(*req_complete));

    /* Number of bytes processed/consumed */
    *pnb -= buflen;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_READ_PROGRESS_UPDATE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_zcp_recv_sge_reg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_zcp_recv_sge_reg(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    int i;
    MPID_Nem_nd_block_op_hnd_t zcp_op_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_REG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_REG);

    MPIU_Assert(conn_hnd->zcp_recv_sge_count > 0);

    mpi_errno = MPID_Nem_nd_block_op_init(&zcp_op_hnd,
                        conn_hnd->zcp_recv_sge_count,
                        conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    for(i=0; i<conn_hnd->zcp_recv_sge_count; i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Registering sge [%d/%d]={%p/%I64d}",
            i, conn_hnd->zcp_recv_sge_count,
            conn_hnd->zcp_recv_sge[i].pAddr,
            conn_hnd->zcp_recv_sge[i].Length));
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->zcp_recv_sge[i].pAddr,
                conn_hnd->zcp_recv_sge[i].Length,
                MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(zcp_op_hnd),
                &(conn_hnd->zcp_recv_sge[i].hMr));

        if(SUCCEEDED(hr)){
	        conn_hnd->npending_ops++;
	        mpi_errno = MPID_Nem_nd_sm_block(zcp_op_hnd);
	        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        /* FIXME: Change the error message - nd_mem_reg */
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_REG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_zcp_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_zcp_recv(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_msg_t *pzcp_msg;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_RECV);

    /* A msg buf is guaranteed for RDMA read */
    MSGBUF_FREEQ_DEQUEUE(conn_hnd, pzcp_msg);
    MPIU_Assert(pzcp_msg != NULL);
    
    SET_MSGBUF_HANDLER(pzcp_msg, zcp_read_success_handler, zcp_read_fail_handler);

    while(MPID_Nem_nd_dev_hnd_g->npending_rds >= 2){
        mpi_errno = MPID_Nem_nd_sm_poll(0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "RDMA READ: Using remote mem descriptor : base = %p, length=%I64d, token=%d",
        _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Base), 
        _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length),
        conn_hnd->zcp_msg_recv_mw.mw_data.Token));

    {
        SIZE_T len=0;
        for(i=0; i<conn_hnd->zcp_recv_sge_count; i++){
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "RDMA READ: Using local sge [%d/%d] : pAddr = %p, length=%I64d, hMr =%x",
                i, conn_hnd->zcp_recv_sge_count,
                conn_hnd->zcp_recv_sge[i].pAddr,
                conn_hnd->zcp_recv_sge[i].Length,
                conn_hnd->zcp_recv_sge[i].hMr));
            len += conn_hnd->zcp_recv_sge[i].Length;
        }
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Performing RDMA READ for " MPIR_UPINT_FMT_DEC_SPEC "bytes", len));
    }

    hr = conn_hnd->p_ep->Read(GET_PNDRESULT_FROM_MSGBUF(pzcp_msg),
            &(conn_hnd->zcp_recv_sge[0]),
            conn_hnd->zcp_recv_sge_count,
            &(conn_hnd->zcp_msg_recv_mw.mw_data),
            0, ND_OP_FLAG_READ_FENCE);
    MPID_Nem_nd_dev_hnd_g->npending_rds++; conn_hnd->npending_rds++;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "dev prds = %d; conn prds = %d",
        MPID_Nem_nd_dev_hnd_g->npending_rds, conn_hnd->npending_rds));

    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_RECV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


/* Function assumes that conn_hnd->zcp_msg_recv_mw is already set */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_zcp_unpack_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_zcp_unpack_iov(MPID_Nem_nd_conn_hnd_t conn_hnd,
                                    MPID_IOV *iovp,
                                    int offset_start,
                                    int *offset_endp)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    SIZE_T invec_len;
    int iov_offset, sge_offset;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_UNPACK_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_UNPACK_IOV);

    invec_len = _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length);
    MPIU_Assert(invec_len > 0);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "ZCP Unpack IOV[%d, %d), vec_len=" MPIR_UPINT_FMT_DEC_SPEC,
        offset_start, *offset_endp, invec_len));
    conn_hnd->zcp_recv_sge_count = 0;

    for(iov_offset=offset_start, sge_offset=0;
            (iov_offset < *offset_endp) && (invec_len > 0) && (sge_offset < MPID_IOV_LIMIT);
            sge_offset++){

        u_long cur_iov_len;

        cur_iov_len = iovp[iov_offset].MPID_IOV_LEN;

        /* Note that invec_len will be < MPID_NEM_ND_DEV_IO_LIMIT */
        if(invec_len < cur_iov_len){
            conn_hnd->zcp_recv_sge[sge_offset].pAddr = iovp[iov_offset].MPID_IOV_BUF;
            conn_hnd->zcp_recv_sge[sge_offset].Length = invec_len;
            conn_hnd->zcp_recv_sge_count++;

            iovp[iov_offset].MPID_IOV_BUF += invec_len;
            iovp[iov_offset].MPID_IOV_LEN -= invec_len;

            invec_len = 0;
            break;
        }
        else{
            conn_hnd->zcp_recv_sge[sge_offset].pAddr = iovp[iov_offset].MPID_IOV_BUF;
            conn_hnd->zcp_recv_sge[sge_offset].Length = cur_iov_len;
            conn_hnd->zcp_recv_sge_count++;

            invec_len -= cur_iov_len;
            iovp[iov_offset].MPID_IOV_LEN = 0;
        }

        if(iovp[iov_offset].MPID_IOV_LEN == 0){
            iov_offset++;
        }
    }

    *offset_endp = iov_offset;

    for(i=0; i<conn_hnd->zcp_recv_sge_count; i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "recv sge[%d/%d]={%p/%I64d}",
            i, conn_hnd->zcp_recv_sge_count,
            conn_hnd->zcp_recv_sge[i].pAddr,
            conn_hnd->zcp_recv_sge[i].Length));
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_UNPACK_IOV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


/*
 * Output:
 * offset_endp => Returns the next offset that is to be packed, could be invalid
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_unpack_iov
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_Nem_nd_unpack_iov(MPID_Nem_nd_conn_hnd_t conn_hnd,
                                    MPID_IOV *iovp,
                                    int offset_start,
                                    int *offset_endp,
                                    MPID_Nem_nd_pack_t pack_type,
                                    MPID_Nem_nd_msg_mw_t *msg_mwp,
                                    char *buf,
                                    SIZE_T *nbp)
{
    int mpi_errno = MPI_SUCCESS;
    int ret_errno;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_UNPACK_IOV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_UNPACK_IOV);
    
    MPIU_Assert(iovp != NULL);
    MPIU_Assert(offset_start >= 0);
    MPIU_Assert(offset_endp != NULL);
    MPIU_Assert(*offset_endp >= offset_start);
    MPIU_Assert((pack_type == MPID_NEM_ND_SR_PACK) ? ((buf != NULL) && (nbp != NULL) && (*nbp > 0)) : 1);

    /* Unpack and register */
    if(pack_type == MPID_NEM_ND_SR_PACK){
        int iov_count, off;
        SIZE_T nb;

        iov_count = *offset_endp - offset_start;
        off = offset_start;

        MPIU_Assert(off < *offset_endp);
        nb = copy_and_trim_iov(iovp, &iov_count, &off, buf, *nbp);

        /* Number of bytes consumed */
        *nbp = nb;
        /* Return the last valid offset that was processed */
        *offset_endp = off;
    }
    else if(pack_type == MPID_NEM_ND_ZCP_PACK){
        if(msg_mwp != NULL){
            /* Save the MW */
            ret_errno = memcpy_s((void *)&(conn_hnd->zcp_msg_recv_mw), sizeof(MPID_Nem_nd_msg_mw_t ), (void *)msg_mwp, sizeof(MPID_Nem_nd_msg_mw_t));
		    MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,
			    "**nd_read", "**nd_read %s %d", strerror(ret_errno), ret_errno);

            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Received mem descriptor : base = %p, length=%I64d, token=%d\n",
                _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Base), _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length),
                conn_hnd->zcp_msg_recv_mw.mw_data.Token));
        }
        else{
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Re-using mem descriptor : base = %p, length=%I64d, token=%d\n",
                _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Base), _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length),
                conn_hnd->zcp_msg_recv_mw.mw_data.Token));
        }

        /* Unpack IOVs to SGEs */
        mpi_errno = MPID_Nem_nd_zcp_unpack_iov(conn_hnd, iovp, offset_start, offset_endp);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* Register SGEs */
        mpi_errno = MPID_Nem_nd_zcp_recv_sge_reg(conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* Unrecognized packing type */
        MPIU_Assert(0);
    }
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_UNPACK_IOV);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_recv_sge_dereg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int zcp_recv_sge_dereg(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    int i;
    MPID_Nem_nd_block_op_hnd_t zcp_op_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_DEREG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_DEREG);

    mpi_errno = MPID_Nem_nd_block_op_init(&zcp_op_hnd,
                    conn_hnd->zcp_recv_sge_count, conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    for(i=0; i<conn_hnd->zcp_recv_sge_count; i++){
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->DeregisterMemory(conn_hnd->zcp_recv_sge[i].hMr,
                MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(zcp_op_hnd));
        if(SUCCEEDED(hr)){
	        conn_hnd->npending_ops++;
	        mpi_errno = MPID_Nem_nd_sm_block(zcp_op_hnd);
	        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
            _com_error(hr).ErrorMessage(), hr);

        conn_hnd->zcp_recv_sge[i].Length = 0;
        conn_hnd->zcp_recv_sge[i].pAddr = 0;
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_RECV_SGE_DEREG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_dereg_smem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int zcp_dereg_smem(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    int i, iov_offset;
    MPID_Nem_nd_block_op_hnd_t zcp_op_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ZCP_DEREG_SMEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ZCP_DEREG_SMEM);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    /* FIXME: Don't block here - let each reg mem take place inside a handler */
    /* Registering the local IOV */
    mpi_errno = MPID_Nem_nd_block_op_init(&zcp_op_hnd, 1, conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->DeregisterMemory(conn_hnd->zcp_send_mr_hnd,
            MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(zcp_op_hnd));
    if(SUCCEEDED(hr)){
        /* Manual event */
        conn_hnd->npending_ops++;
        mpi_errno = MPID_Nem_nd_sm_block(zcp_op_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    /* FIXME: Change the error message - nd_mem_reg */
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
        _com_error(hr).ErrorMessage(), hr);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ZCP_DEREG_SMEM);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_read_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_read_success_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS, ret_errno=0;
    SIZE_T nb=0, invec_len;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_t *pmsg;
    MPID_Request *zcp_reqp;
    int (*req_fn)(MPIDI_VC_t *, MPID_Request *, int *);
    int req_complete=0;
    int sge_count, sge_offset, mw_count, mw_offset, i;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    nb = GET_NB_FROM_MSGRESULT(send_result);
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Finished RDMA Read [" MPIR_UPINT_FMT_DEC_SPEC "] bytes on conn[%p]", nb, conn_hnd));

    MPID_Nem_nd_dev_hnd_g->npending_rds--; conn_hnd->npending_rds--;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "dev prds = %d; conn prds = %d",
        MPID_Nem_nd_dev_hnd_g->npending_rds, conn_hnd->npending_rds));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "After Rd Rcvd mem descriptor : base = %p, length=%I64d, token=%d\n",
        _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Base), _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length),
        conn_hnd->zcp_msg_recv_mw.mw_data.Token));

    zcp_reqp = conn_hnd->zcp_rreqp;
    MPIU_Assert(zcp_reqp != NULL);

    /* Trim nd_sge list of recv bufs */
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Trimming recv_sge[cnt=%d], nb=" MPIR_UPINT_FMT_DEC_SPEC,
        conn_hnd->zcp_recv_sge_count, nb));

    sge_count = conn_hnd->zcp_recv_sge_count;
    sge_offset = 0;
    trim_nd_sge(conn_hnd->zcp_recv_sge, &sge_count, &sge_offset, nb);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "After trimming recv_sge[cnt=%d/off=%d], nb=" MPIR_UPINT_FMT_DEC_SPEC,
        conn_hnd->zcp_recv_sge_count, sge_offset, nb));

    MPIU_Assert(sge_count == 0);

    /* Trim the nd mw descriptor list of send bufs */
    mw_count = 1;
    mw_offset = 0;
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Trimming recv_mw[len=%I64d], nb=" MPIR_UPINT_FMT_DEC_SPEC,
        _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length), nb));

    trim_nd_mw(&(conn_hnd->zcp_msg_recv_mw.mw_data), &mw_count, &mw_offset, nb);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "After trimming recv_mw[len=%I64d], nb=" MPIR_UPINT_FMT_DEC_SPEC,
        _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length), nb));

    invec_len = _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length);

    /* Deregister old bufs */
    mpi_errno = zcp_recv_sge_dereg(conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    req_complete = 0;
    if(zcp_reqp->dev.iov_count == 0){
        mpi_errno = MPID_Nem_nd_handle_recv_req(conn_hnd, zcp_reqp, &req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        if(req_complete){
            MPIU_Assert(invec_len == 0);
            MPID_NEM_ND_VCCH_SET_ACTIVE_RECV_REQ(conn_hnd->vc, NULL);
        }
    }

    if(invec_len == 0){
        /* We are no longer ZCP reading */
        conn_hnd->zcp_rreqp = NULL;

        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Sending RD ACK ...");
        /* We have now read all the send bufs */
        /* Use the msg & send rd complete pkt */
        pmsg->hdr.type = MPID_NEM_ND_RD_ACK_PKT;

        SET_MSGBUF_HANDLER(pmsg, netmod_msg_send_success_handler, gen_send_fail_handler);
        mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, sizeof(MPID_Nem_nd_msg_hdr_t ), 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        int offset_end;
        
        MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Re-unpacking and reading ...");
        /* re-unpack and read */
        offset_end = zcp_reqp->dev.iov_offset + zcp_reqp->dev.iov_count;
        mpi_errno = MPID_Nem_nd_unpack_iov(conn_hnd,
                        zcp_reqp->dev.iov,
                        zcp_reqp->dev.iov_offset,
                        &offset_end,
                        MPID_NEM_ND_ZCP_PACK,
                        NULL,
                        NULL, NULL);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* Next offset to be packed */
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "zcp_reqp(%p) off %d -> %d", zcp_reqp, zcp_reqp->dev.iov_offset, offset_end));

        zcp_reqp->dev.iov_count -= (offset_end - zcp_reqp->dev.iov_offset);
        zcp_reqp->dev.iov_offset = offset_end;

        mpi_errno = MPID_Nem_nd_zcp_recv(conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_mw_invalidate_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_mw_invalidate_success_handler(MPID_Nem_nd_msg_result_t *recv_result)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Request *sreqp = NULL;
    MPID_Nem_nd_msg_t *pmsg;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_INVALIDATE_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_INVALIDATE_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    sreqp = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(sreqp != NULL);

    /* Repost the recv buf */
    pmsg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(pmsg != NULL);

    /* Repost msg buf */
    SET_MSGBUF_HANDLER(pmsg, recv_success_handler, gen_recv_fail_handler);
    mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = zcp_dereg_smem(conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    conn_hnd->zcp_in_progress = 0;
    conn_hnd->zcp_send_offset = 0;
    MPID_Nem_nd_dev_hnd_g->zcp_pending = 0;

    if(sreqp->dev.iov_count == 0){
        /* Call the cont success handler */
        mpi_errno = cont_send_success_handler(&(conn_hnd->zcp_send_result));
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* Continue sending data on this req */
        mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_INVALIDATE_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME zcp_mw_send_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_mw_send_success_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_t *pmsg;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    SET_MSGBUF_HANDLER(pmsg, send_success_handler, gen_send_fail_handler);
    MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);
    /* Nothing to be done here - We are still waiting for an RD complete
     * packet - RD_ACK
     */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_SEND_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME cont_send_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int cont_send_success_handler(MPID_Nem_nd_msg_result_t *zcp_send_result)
{
    int mpi_errno = MPI_SUCCESS;
    int req_complete;
    MPID_Nem_nd_conn_hnd_t conn_hnd;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_CONT_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_CONT_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_ZCP_SEND_MSGRESULT(zcp_send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));
    MPIU_Assert(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(conn_hnd->vc));

    if(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(conn_hnd->vc)){
        req_complete = 0;
        mpi_errno = MPID_Nem_nd_handle_posted_sendq_tail_req(conn_hnd->vc, &req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        if(req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ZCP/Cont_send req complete...");
            /* Allow sends on the conn */
            conn_hnd->send_in_progress = 0;
            conn_hnd->send_credits = conn_hnd->cache_credits;

            /* If we have queued sends and credits to send data - go ahead with sending */
            if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send credits available. Processing queued req...");
                mpi_errno = process_pending_req(conn_hnd);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
        }
        else{
            MPID_Request *sreqp;

            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ZCP/Cont_send req NOT complete... sending remaining/reloaded IOVs");
            sreqp = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
            MPIU_Assert(sreqp != NULL);

            /* Send reloaded iovs */
            mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_CONT_SEND_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME process_pending_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int process_pending_req(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_PROCESS_PENDING_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_PROCESS_PENDING_REQ);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));
    while(!MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_EMPTY(conn_hnd->vc) &&
        MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
        /* Post sends from pending sendq - dequeue from pending queue,
         * post the send, enqueue the req in posted queue
         */
        MPID_Request *req;
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_DEQUEUE(conn_hnd->vc, &req);
        MPIU_Assert(req != NULL);

        /* FIXME: Can we coalesce multiple pending sends ? */
        if(!MPID_NEM_ND_IS_BLOCKING_REQ(req)){
            mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, req);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        else{
            mpi_errno = MPID_Nem_nd_post_sendbv(conn_hnd, req);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }

        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(conn_hnd->vc, req);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_PROCESS_PENDING_REQ);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Handle the request at the head of vc's sendq */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_handle_posted_sendq_head_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_Nem_nd_handle_posted_sendq_head_req(MPIDI_VC_t *vc, int *req_complete)
{
    int (*req_handler)(MPIDI_VC_t *, MPID_Request *, int *);
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_HEAD_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_HEAD_REQ);

    MPIU_Assert(req_complete != NULL);

    MPIU_Assert(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(vc));
    req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_HEAD(vc);
    MPIU_Assert(req != NULL);

    req_handler = req->dev.OnDataAvail;
    if (!req_handler){
        MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);
        MPIDI_CH3U_Request_complete(req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        *req_complete = 1;
    }
    else{
        *req_complete = 0;
        mpi_errno = req_handler(vc, req, req_complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (*req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... Not complete");
            req->dev.iov_offset = 0;
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_HEAD_REQ);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* Handle the request at the tail of vc's sendq */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_handle_posted_sendq_tail_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int MPID_Nem_nd_handle_posted_sendq_tail_req(MPIDI_VC_t *vc, int *req_complete)
{
    int (*req_handler)(MPIDI_VC_t *, MPID_Request *, int *);
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *req = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_TAIL_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_TAIL_REQ);

    MPIU_Assert(req_complete != NULL);

    MPIU_Assert(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(vc));
    req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(vc);
    MPIU_Assert(req != NULL);

    req_handler = req->dev.OnDataAvail;
    if (!req_handler){
        MPIU_Assert(MPIDI_Request_get_type(req) != MPIDI_REQUEST_TYPE_GET_RESP);
        MPIDI_CH3U_Request_complete(req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_REM_TAIL(vc, &req);
        *req_complete = 1;
    }
    else{
        *req_complete = 0;
        mpi_errno = req_handler(vc, req, req_complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (*req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_REM_TAIL(vc, &req);
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... Not complete");
            req->dev.iov_offset = 0;
        }
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_TAIL_REQ);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME send_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_success_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS;
    int req_complete = 0;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_t   *pmsg;
    MPID_Request *sreqp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    sreqp = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(sreqp != NULL);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send succeeded...");

    /* Reset the handlers & enqueue this send buffer to freeq */
    SET_MSGBUF_HANDLER(pmsg, send_success_handler, gen_send_fail_handler);
    MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);
    MPIU_Assert(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY((conn_hnd->vc)));

    if(MPID_NEM_ND_VC_IS_CONNECTED(conn_hnd->vc)){
        /* Increment number of available send credits only when a credit packet is recvd */
        /* Complete the request associated with this send if no pending events */
        req_complete = 0;

        if(!conn_hnd->send_in_progress){
            mpi_errno = MPID_Nem_nd_handle_posted_sendq_head_req(conn_hnd->vc, &req_complete);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_DEQUEUE(conn_hnd->vc, &sreqp);
            if(req_complete){
                /* If we have queued sends and credits to send data - go ahead with sending */
                if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
                    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send credits available. Processing queued req...");
                    mpi_errno = process_pending_req(conn_hnd);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                }
            }
            else{
                MPIU_Assert(0);
                mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(conn_hnd->vc, sreqp);
            }
        }
        else{
            if(MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_HEAD(conn_hnd->vc) ==
                MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc)){
                /* Only ZCP/Cont_send req in posted Q */
                /* If ZCP is in progress - the ZCP handlers will handle sends */
                if(!conn_hnd->zcp_in_progress){
                    if(sreqp->dev.iov_count == 0){
                        mpi_errno = cont_send_success_handler(&(conn_hnd->zcp_send_result));
                        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                    }
                    else{
                        /* Repost the remaining/reloaded IOV */
                        mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
                        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                    }
                }
            }
            else{
                req_complete = 0;
                mpi_errno = MPID_Nem_nd_handle_posted_sendq_head_req(conn_hnd->vc, &req_complete);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_DEQUEUE(conn_hnd->vc, &sreqp);

                MPIU_Assert(req_complete);
            }
        }
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_SEND_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME netmod_msg_send_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int netmod_msg_send_success_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS;
    int req_complete = 0;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_t   *pmsg;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_NETMOD_MSG_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_NETMOD_MSG_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Netmod Send succeeded...");

    /* Reset the handlers & enqueue this send buffer to freeq */
    SET_MSGBUF_HANDLER(pmsg, send_success_handler, gen_send_fail_handler);
    MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

    if(MPID_NEM_ND_VC_IS_CONNECTED(conn_hnd->vc)){
        /* Increment number of available send credits only when a credit packet is recvd */
        /* There is no request associated with this send - Netmod msg */

        /* If we have queued sends and credits to send data - go ahead with sending */
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send credits available. Processing queued req...");
            mpi_errno = process_pending_req(conn_hnd);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_NETMOD_MSG_SEND_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME wait_cack_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int wait_cack_success_handler(MPID_Nem_nd_msg_result_t *recv_result)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_t *pmsg;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_WAIT_CACK_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_WAIT_CACK_SUCCESS_HANDLER);

    pmsg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(pmsg != NULL);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    /* Received a CACK/CNAK */
    MPIU_Assert((pmsg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT) ||
                    (pmsg->hdr.type == MPID_NEM_ND_CONN_NAK_PKT));
    if(pmsg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT){
        /* Connection successful */
        MPIDI_VC_t *vc;

        /* Set this conn vc to the stored vc info */
        vc = conn_hnd->tmp_vc;

        conn_hnd->vc = vc;
        MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, conn_hnd);
        /* We no longer need tmp conn info in VC */
        MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_INIT(vc);
        MPID_NEM_ND_VCCH_NETMOD_STATE_SET(vc, MPID_NEM_ND_VC_STATE_CONNECTED);
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_ACTIVE);

        /* Repost recv buf */
        SET_MSGBUF_HANDLER(pmsg, recv_success_handler, gen_recv_fail_handler);
        mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        mpi_errno = process_pending_req(conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* Connection failed - Lost in head to head on the remote side */
        conn_hnd->vc = NULL;
        conn_hnd->tmp_vc = NULL;

        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
        mpi_errno = MPID_Nem_nd_conn_disc(conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_WAIT_CACK_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME listen_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl listen_success_handler(MPIU_EXOVERLAPPED *recv_ov)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t lconn_hnd, new_conn_hnd;
    MPID_Nem_nd_pg_info_hdr_t *pg_info;
    char *buf=NULL;
    MPIDI_PG_t *pg;
    char *pg_id;
    MPIDI_VC_t *vc;
    MPIDI_CH3I_VC *vc_ch;
    SIZE_T buf_len=0, irl, orl;
    int terminate_conn = 0;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_LISTEN_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_LISTEN_SUCCESS_HANDLER);

    lconn_hnd = GET_CONNHND_FROM_EX_RECV_OV(recv_ov);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(lconn_hnd));

    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_ACCEPT_CONN, lconn_hnd->p_conn, NULL, &new_conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Get the pg information sent with the connect request */
    /* FIXME: Use the IRL and ORL to determine the Queue length for conn */
    buf_len = 0;
    hr = new_conn_hnd->p_conn->GetConnectionData(&irl, &orl, (void *)buf, &buf_len);

    buf = (char *)MPIU_Malloc(buf_len);

    hr = new_conn_hnd->p_conn->GetConnectionData(&irl, &orl, (void *)buf, &buf_len);
    MPIU_ERR_CHKANDJUMP2(FAILED(hr) || (buf_len == 0),
        mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
        _com_error(hr).ErrorMessage(), hr);

    pg_info = (MPID_Nem_nd_pg_info_hdr_t *)buf;

    MPIU_Assert(pg_info->pg_id_len == 0);
    /* If vc is already connected send a LNACK */
    /* Head to head resolution
     * If vc has a valid conn we have a head to head situation
     * If (my_rank < remote_rank) This connection wins
     * else This connection loses
     * If this connection wins hijack the vc => the other conn is now an orphan
     *    and send a LACK before waiting for a CACK/CNACK
     * If this connection loses send a LNACK and disc
     */
    /* A connection is pending - do a blocking accept */

    mpi_errno = MPID_Nem_nd_post_accept(lconn_hnd, new_conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if(pg_info->pg_id_len == 0){
        pg_id = (char *)MPIDI_Process.my_pg->id;
        mpi_errno = MPID_Nem_nd_decode_pg_info(pg_id, pg_info->pg_rank, &vc, &pg);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "New conn (%p) for rank = %d", new_conn_hnd, pg_info->pg_rank));
    }
    else{
        /* FIXME: TODO */
        MPIU_Assert(0);
    }

    vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    if(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_CONNECTED){
        /* VC is already connected */
        terminate_conn = 1;
    }
    else if(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_CONNECTING){
        /* VC is connecting - head-to-head scenario */
        MPID_Nem_nd_conn_hnd_t old_conn_hnd = MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_GET(vc);
        int old_conn_won_hh = 0;

        mpi_errno = MPID_Nem_nd_resolve_head_to_head(pg_info->pg_rank, pg, pg_id, &old_conn_won_hh);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* The old conn may not be VALID yet - So don't use it */
        MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(old_conn_hnd));
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "H-H: Old conn (%p:%d) & new conn (%p:%d)", old_conn_hnd, old_conn_hnd->state, new_conn_hnd, new_conn_hnd->state));
        
        if(old_conn_won_hh){
            /* Won head to head with new conn
             * Send a NAK and close the new conn 
             */
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Old conn (%p) won head to head", MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc)));
            terminate_conn = 1;
        }
        else{
            /* Lost head to head with new conn
             * Make old conn orphan - The other side with send
             * us a LNAK and we then close the old conn
             */
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "New conn (%p) won head to head", new_conn_hnd));

            /*
            MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, new_conn_hnd);
            new_conn_hnd->vc = vc;
            */
            old_conn_hnd->is_orphan = 1;
            /* Save VC info */
            new_conn_hnd->tmp_vc = vc;
            terminate_conn = 0;
        }
    }
    else{ /* VC is DISCONNECTED */
        /* Save vc info with this connection. We are still not
         * sure if the VC is CONNECTING - Since a CNAK could mean
         * an orphan conn
         * Associate this conn with vc when we receive a CACK
         */
        new_conn_hnd->tmp_vc = vc;
        terminate_conn = 0;
    }

    if(terminate_conn){
		/* New conn lost head to head OR VC is already connected
         * Do a blocking send for LNAK & disc
         */
		MPID_Nem_nd_msg_t *pmsg;
		SIZE_T msg_len=0;

        MPID_NEM_ND_CONN_STATE_SET(new_conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
		/* Post a LNAK */
		MSGBUF_FREEQ_DEQUEUE(new_conn_hnd, pmsg);
        MPIU_Assert(pmsg != NULL);
        SET_MSGBUF_HANDLER(pmsg, netmod_msg_send_success_handler, gen_send_fail_handler);

		pmsg->hdr.type = MPID_NEM_ND_CONN_NAK_PKT;
		msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
		mpi_errno = MPID_Nem_nd_post_send_msg(new_conn_hnd, pmsg, msg_len, 0);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Passive disc on (%p)", new_conn_hnd));

        /* Wait for a disconnect/CNAK/CACK from the other side and free resources */
        mpi_errno = MPID_Nem_nd_conn_passive_disc(new_conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    else{
        /* Connection successful - send LACK & wait for CACK */
        MPID_Nem_nd_msg_t *pmsg;
        SIZE_T msg_len=0;

		MPID_NEM_ND_CONN_STATE_SET(new_conn_hnd, MPID_NEM_ND_CONN_WAIT_CACK);
		/* Grab the head of the recv ssbufs and set its handlers */
		pmsg = GET_RECV_SBUF_HEAD(new_conn_hnd);
		MPIU_Assert(pmsg != NULL);
		SET_MSGBUF_HANDLER(pmsg, wait_cack_success_handler, gen_recv_fail_handler);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Setting the wait_cack recv handler for msg_buf = %p", pmsg));

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Post LACK wait for CACK on (%p)", new_conn_hnd));

		/* Post a LACK - do a non-blocking send */
		MSGBUF_FREEQ_DEQUEUE(new_conn_hnd, pmsg);
        SET_MSGBUF_HANDLER(pmsg, netmod_msg_send_success_handler, gen_send_fail_handler);
        pmsg->hdr.type = MPID_NEM_ND_CONN_ACK_PKT;
		msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
		mpi_errno = MPID_Nem_nd_post_send_msg(new_conn_hnd, pmsg, msg_len, 0);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_LISTEN_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Received LACK/LNAK from the listen side */
#undef FUNCNAME
#define FUNCNAME wait_lack_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int wait_lack_success_handler(MPID_Nem_nd_msg_result_t *recv_result){
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_msg_t *precv_msg;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    int terminate_conn = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_WAIT_LACK_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_WAIT_LACK_SUCCESS_HANDLER);

    precv_msg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(precv_msg != NULL);

    conn_hnd = GET_CONNHND_FROM_MSGBUF(precv_msg);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    /* If this conn receives a NAK from listen side we lost in head to head
     * If this conn receives an ACK from the listen side make sure that the
     * conn is not an ORPHAN. If this conn is an ORPHAN this conn lost in a
     * head to head battle. Else send an ACK - CACK.
     */
    if(!MPID_NEM_ND_CONN_IS_ORPHAN(conn_hnd)){
        /* Check whether we received an ACK or NACK */
        MPIU_Assert((precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT) ||
                    (precv_msg->hdr.type == MPID_NEM_ND_CONN_NAK_PKT));
        if(precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT){
            MPID_Nem_nd_msg_t   *psend_msg;
            MPIDI_VC_t *vc;
            SIZE_T msg_len=0;

            /* VC is now connected - send an ACK - CACK - to listen side */

            terminate_conn = 0;

            /* FIXME: Use a single macro to set vc->conn/conn->vc/state/checks etc */
            vc = conn_hnd->tmp_vc;
            conn_hnd->vc = vc;
            MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, conn_hnd);
            /* We no longer need the tmp conn info in vc */
            MPID_NEM_ND_VCCH_NETMOD_TMP_CONN_HND_INIT(vc);
            MPIU_Assert(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_CONNECTING);
            MPID_NEM_ND_VCCH_NETMOD_STATE_SET(vc, MPID_NEM_ND_VC_STATE_CONNECTED);
            MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_ACTIVE);

            MPIU_Assert(!MSGBUF_FREEQ_IS_EMPTY(conn_hnd));
            MSGBUF_FREEQ_DEQUEUE(conn_hnd, psend_msg);
            MPIU_Assert(psend_msg != NULL);
            
            SET_MSGBUF_HANDLER(psend_msg, netmod_msg_send_success_handler, gen_send_fail_handler);
            /* FIXME: We shouldn't waste an entire MSG BUF for an ack */
            psend_msg->hdr.type = MPID_NEM_ND_CONN_ACK_PKT;
            msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LACK - Sending CACK");
            mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, psend_msg, msg_len, 0);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            /* Repost receive on the used msg buf */
            SET_MSGBUF_HANDLER(precv_msg, recv_success_handler, gen_recv_fail_handler);
            mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, precv_msg);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            /* FIXME: See if there is any queued data to send */
            mpi_errno = process_pending_req(conn_hnd);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        else{
            /* Received LNAK - Close connection */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LNAK - Sending CNAK and closing conn");
            terminate_conn = 1;
        }
    }
    else{
        /* Send NAK - We lost in head to head connection to the listen side */
        MPIU_Assert((precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT) ||
                    (precv_msg->hdr.type == MPID_NEM_ND_CONN_NAK_PKT));
        terminate_conn = 1;
        if(precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LACK - Lost HH - Sending CNAK & Closing connection");
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LNAK - Lost HH - Sending CNAK & Closing connection");
        }
    }
    if(terminate_conn){
        /* We reach here in 2 cases
         * case 1: Received LNAK, Conn is not orphan yet
         * case 2: Conn is orphan
         * In both cases send a CNAK and disconnect
         */
        /* Send a CNAK and disc */
        MPID_Nem_nd_msg_t   *psend_msg;
        SIZE_T msg_len=0;

        /* VC is already/or-will-be connected from conn 
         * by the listen side. The VC may also have terminated by now.
         * - So don't change the state of VC here
         * the vc state is left *as-is*
         */
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);

        MPIU_Assert(!MSGBUF_FREEQ_IS_EMPTY(conn_hnd));
        MSGBUF_FREEQ_DEQUEUE(conn_hnd, psend_msg);
        MPIU_Assert(psend_msg != NULL);
        SET_MSGBUF_HANDLER(psend_msg, quiescent_msg_handler, gen_send_fail_handler);

        psend_msg->hdr.type = MPID_NEM_ND_CONN_NAK_PKT;
        msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );

        mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, psend_msg, msg_len, 0);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_WAIT_LACK_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME recv_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int recv_success_handler(MPID_Nem_nd_msg_result_t *recv_result)
{
    int mpi_errno = MPI_SUCCESS, ret_errno;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t  conn_hnd;
    MPID_Nem_nd_msg_t   *pmsg, *pzcp_msg;
    MPID_Request *rreqp = NULL;
    int i, offset_end;
    char *buf;
    SIZE_T buflen, nb, udata_len=0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_RECV_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_RECV_SUCCESS_HANDLER);

    pmsg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(pmsg != NULL);
    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    nb = GET_NB_FROM_MSGRESULT(recv_result);
    MPIU_ERR_CHKANDJUMP(nb == 0, mpi_errno, MPI_ERR_OTHER, "**nd_write");

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Recvd " MPIR_UPINT_FMT_DEC_SPEC " bytes (msg type=%d) on conn=%p",
        nb, pmsg->hdr.type, conn_hnd));

    MPIU_Assert(nb >= sizeof(MPID_Nem_nd_msg_hdr_t ));
    if(!conn_hnd->send_in_progress){
        conn_hnd->send_credits += pmsg->hdr.credits;
    }
    else{
        conn_hnd->cache_credits += pmsg->hdr.credits;
    }
    udata_len = nb - sizeof(MPID_Nem_nd_msg_hdr_t );
    switch(pmsg->hdr.type){
        case MPID_NEM_ND_CRED_PKT:
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Received CRED PKT (credits = %d)",pmsg->hdr.credits));
                
                /* Repost recv buf */
                mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                mpi_errno = process_pending_req(conn_hnd);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                break;
        case MPID_NEM_ND_DATA_PKT:
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Received DATA PKT (len =" MPIR_UPINT_FMT_DEC_SPEC ", credits = %d)",udata_len, pmsg->hdr.credits));
                buf = pmsg->buf;
                buflen = udata_len;

                do{
                    MPIU_Assert(conn_hnd->zcp_rreqp == NULL);
                    rreqp = MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc);
                    if(rreqp == NULL){
                        /* The msg just contains the type and udata */
                        mpi_errno = MPID_nem_handle_pkt(conn_hnd->vc, buf, buflen);
                        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                        /* MPID_nem_handle_pkt() consumes all data */
                        buflen = 0;
                    }
                    else{
                        /* Continuing to recv on this conn - Just copy data into req IOVs */
                        int complete = 0;
                        SIZE_T nb = 0;

                        nb = buflen;
                        mpi_errno = nd_read_progress_update(conn_hnd, rreqp, buf, &nb, &complete);
                        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                        if(complete){
                            MPID_NEM_ND_VCCH_SET_ACTIVE_RECV_REQ(conn_hnd->vc, NULL);
                        }   
                        buflen -= nb;
                        buf += nb;
                    }

                    MPIU_Assert(buflen == 0);
                }while(buflen > 0);

                /* When handling a packet the conn might be disconnected */
                if(conn_hnd->vc != NULL){
                    /* Repost the recv on the scratch buf */
                    mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                    /* A piggy backed credit can unblock send conn */
                    mpi_errno = process_pending_req(conn_hnd);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                }
                break;
        case MPID_NEM_ND_RD_AVAIL_PKT:
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Received RD Avail pkt (len=" MPIR_UPINT_FMT_DEC_SPEC ")", udata_len);
                udata_len -= sizeof(MPID_Nem_nd_msg_mw_t);

                MPIU_Assert(((MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc) != NULL) && (conn_hnd->zcp_rreqp != NULL))? 
                    (MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc) == conn_hnd->zcp_rreqp) : 1);
                rreqp = (conn_hnd->zcp_rreqp) ? (conn_hnd->zcp_rreqp) : MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc) ;
                conn_hnd->zcp_rreqp = rreqp;

                if(rreqp == NULL){
                    mpi_errno = MPID_nem_handle_pkt(conn_hnd->vc, pmsg->buf, udata_len);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    
                    rreqp = MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc);
                    MPIU_Assert(rreqp != NULL);

                    conn_hnd->zcp_rreqp = rreqp;
                }
                else{
                    SIZE_T len = udata_len;
                    /* Continuing to recv data on a req */
                    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,"Cont to recv data on req=%p", rreqp));
                    while(len > 0){
                        int req_complete = 0;
                        SIZE_T nb_unpack;

                        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "RD AVAIL contains " MPIR_UPINT_FMT_DEC_SPEC " bytes", len);

                        offset_end = rreqp->dev.iov_offset + rreqp->dev.iov_count;
                        nb_unpack = len;
                        mpi_errno = MPID_Nem_nd_unpack_iov(conn_hnd,
                                        rreqp->dev.iov,
                                        rreqp->dev.iov_offset,
                                        &offset_end,
                                        MPID_NEM_ND_SR_PACK,
                                        NULL,
                                        pmsg->buf,
                                        &nb_unpack);
                        len -= nb_unpack;
                        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                        
                        /* Update the req offset */
                        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "rreqp(%p) off %d -> %d", rreqp, rreqp->dev.iov_offset, offset_end));

                        rreqp->dev.iov_count -= (offset_end - rreqp->dev.iov_offset);
                        rreqp->dev.iov_offset = offset_end;

                        if(rreqp->dev.iov_count == 0){
                            mpi_errno = MPID_Nem_nd_handle_recv_req(conn_hnd, rreqp, &req_complete);
                            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                            
                            /* RD AVAIL always contains data to zcpy */
                            MPIU_Assert(!req_complete);
                        }
                    }
                }

                for(i=0; i<rreqp->dev.iov_count; i++){
                    int off = rreqp->dev.iov_offset + i;
                    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, 
                        "Trying to zcp unpack req (%p) - iov[%d/tot=%d] = {%p/%u}",
                        rreqp, off, rreqp->dev.iov_count,
                        rreqp->dev.iov[off].MPID_IOV_BUF,
                        rreqp->dev.iov[off].MPID_IOV_LEN
                    ));
                }

                offset_end = rreqp->dev.iov_offset + rreqp->dev.iov_count;
                mpi_errno = MPID_Nem_nd_unpack_iov(conn_hnd,
                                rreqp->dev.iov,
                                rreqp->dev.iov_offset,
                                &offset_end,
                                MPID_NEM_ND_ZCP_PACK,
                                (MPID_Nem_nd_msg_mw_t *)&(pmsg->buf[udata_len]),
                                NULL, NULL);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "rreqp(%p) off %d -> %d", rreqp, rreqp->dev.iov_offset, offset_end));

                rreqp->dev.iov_count -= (offset_end - rreqp->dev.iov_offset);
                rreqp->dev.iov_offset = offset_end;

                /* Repost recv buffer */
                if(conn_hnd->vc != NULL){
                    mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                }

                mpi_errno = MPID_Nem_nd_zcp_recv(conn_hnd);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                break;
        case MPID_NEM_ND_RD_ACK_PKT:
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received RD ACK pkt");
                MPID_Nem_nd_dev_hnd_g->npending_rds--; conn_hnd->npending_rds--;
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "dev prds = %d; conn prds = %d",
                    MPID_Nem_nd_dev_hnd_g->npending_rds, conn_hnd->npending_rds));

                /* Get the send credits for conn */
                MPIU_Assert(udata_len == 0);
                /* Save the credits in the RD ack pkt */
                /* Invalidate/unbind the address */

                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Trying to invalidate MW [%p]",
                    conn_hnd->zcp_send_mw));

                SET_MSGBUF_HANDLER(pmsg, zcp_mw_invalidate_success_handler, gen_recv_fail_handler);
                hr = conn_hnd->p_ep->Invalidate(GET_PNDRESULT_FROM_MSGBUF(pmsg), 
                        conn_hnd->zcp_send_mw, ND_OP_FLAG_READ_FENCE);
                MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
                    mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
                    _com_error(hr).ErrorMessage(), hr);

                break;
        default:
                MPIU_Assert(0);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_RECV_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME connecting_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl connecting_success_handler(MPIU_EXOVERLAPPED *send_ov)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_block_op_hnd_t op_hnd;
    MPID_Nem_nd_msg_t *pmsg;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_EX_SEND_OV(send_ov);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    /* FIXME: We shouldn't block here */
    /* Block and complete the connect() */
    mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd, 1, conn_hnd, 1);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Complete connect on conn(%p)/block_op(%p) on ov(%p)",
           conn_hnd, op_hnd, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd)));

    hr = conn_hnd->p_conn->CompleteConnect(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd));
    if(SUCCEEDED(hr)){
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_WAIT_LACK);
        /* Receive is already pre-posted. Set the handlers correctly and wait
        * for LACK from the other process
        */
        pmsg = GET_RECV_SBUF_HEAD(conn_hnd);
        MPIU_Assert(pmsg != NULL);
        SET_MSGBUF_HANDLER(pmsg, wait_lack_success_handler, gen_recv_fail_handler);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Setting the wait_lack recv handler for msg_buf = %p", pmsg));

        /* Manual event */
		conn_hnd->npending_ops++;
		mpi_errno = MPID_Nem_nd_sm_block(op_hnd);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_connect", "**nd_connect %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME connecting_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl connecting_fail_handler(MPIU_EXOVERLAPPED *send_ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_FAIL_HANDLER);
    /* Disconnect ND conn from VC */
    MPIU_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**nd_connect",
        "**nd_connect %s %d", MPIU_OSW_Strerror(MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(send_ov))),
        MPIU_EX_STATUS_TO_ERRNO(MPIU_ExGetStatus(send_ov)));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_CONNECTING_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME gen_ex_fail_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl gen_ex_fail_handler(MPIU_EXOVERLAPPED *ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_GEN_EX_FAIL_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_GEN_EX_FAIL_HANDLER);

    MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**intern");
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_GEN_EX_FAIL_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME quiescent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl quiescent_handler(MPIU_EXOVERLAPPED *send_ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_HANDLER);

    conn_hnd = GET_CONNHND_FROM_EX_SEND_OV(send_ov);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));
    MPID_Nem_nd_conn_hnd_finalize(MPID_Nem_nd_dev_hnd_g, &conn_hnd);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_QUIESCENT_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME passive_quiescent_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl passive_quiescent_handler(MPIU_EXOVERLAPPED *recv_ov)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPID_Nem_nd_conn_hnd_t conn_hnd;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_PASSIVE_QUIESCENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_PASSIVE_QUIESCENT_HANDLER);

    conn_hnd = GET_CONNHND_FROM_EX_RECV_OV(recv_ov);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    SET_EX_WR_HANDLER(conn_hnd, quiescent_handler, quiescent_handler);

    /* FIXME: DEREGISTER ALL RECV BUFS HERE ...*/
    MPIU_Assert(conn_hnd->npending_ops == 0);
    MPIU_DBG_MSG_P(CH3_CHANNEL, VERBOSE, "Posting disconnect on conn(%p)\n", conn_hnd);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting disc on conn(%p) on ov(%p)",
           conn_hnd, MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov))));

    hr = conn_hnd->p_conn->Disconnect(MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_disc", "**nd_disc %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_PASSIVE_QUIESCENT_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME block_op_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl block_op_handler(MPIU_EXOVERLAPPED *ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_block_op_hnd_t hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_BLOCK_OP_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_BLOCK_OP_HANDLER);
    hnd = CONTAINING_RECORD(ov, MPID_Nem_nd_block_op_hnd_, ex_ov);

	MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(hnd->conn_hnd));

    if(hnd->npending_ops == 0){
        MPID_Nem_nd_block_op_finalize(&hnd);
    }
    else{
        mpi_errno = MPID_Nem_nd_block_op_reinit(hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_BLOCK_OP_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME manual_event_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl manual_event_handler(MPIU_EXOVERLAPPED *ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_block_op_hnd_t hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_MANUAL_EVENT_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_MANUAL_EVENT_HANDLER);
    hnd = CONTAINING_RECORD(ov, MPID_Nem_nd_block_op_hnd_, ex_ov);

	MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(hnd->conn_hnd));
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Manual event handler on conn(%p)/block_op(%p) on ov(%p)",
        hnd->conn_hnd, hnd, ov));

    MPIU_Assert(hnd->npending_ops > 0);
    /* FIXME: Atleast for now both block op and conn have same number of pending ops */
    MPIU_Assert(hnd->conn_hnd->npending_ops > 0);
    /* Note that we might want to wait only for one blocking op on a conn, conn_hnd->npending_ops, but have two
     * blocking ops on the blocking op, eg: registering memory before a connect etc
     */
	/* Handle manual event completion */
    hnd->npending_ops--;
    if(hnd->conn_hnd->npending_ops > 0){
        hnd->conn_hnd->npending_ops--;
    }

	MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "[%d] manual events pending", hnd->conn_hnd->npending_ops);

    if(hnd->npending_ops == 0){
        MPID_Nem_nd_block_op_finalize(&hnd);
    }
    else{
        mpi_errno = MPID_Nem_nd_block_op_reinit(hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_MANUAL_EVENT_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME dummy_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int __cdecl dummy_handler(MPIU_EXOVERLAPPED *ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_block_op_hnd_t hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_DUMMY_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_DUMMY_HANDLER);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_DUMMY_HANDLER);

    return mpi_errno;
}

/* The caller is responsible for freeing the pg info buffer allocated by
 * this function
 */
#undef FUNCNAME
#define FUNCNAME get_my_pg_info
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_my_pg_info(void *remote_pg_id, char **pbuf, int *pbuf_len){
    char *my_pg_id = (char *)(MPIDI_Process.my_pg->id);
    int my_pg_rank = MPIDI_Process.my_pg_rank;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_GET_MY_PG_INFO);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_GET_MY_PG_INFO);
    MPIU_Assert(remote_pg_id != NULL);
    MPIU_Assert(pbuf != NULL);
    MPIU_Assert(pbuf_len != NULL);

    if(MPID_NEM_ND_IS_SAME_PGID(my_pg_id, ((char *)remote_pg_id))){
        /* Both processes belong to the same pg. We don't need to
         * send the pg_id with the pg_info
         */
        MPID_Nem_nd_pg_info_hdr_t   *phdr;
        *pbuf_len = sizeof(struct MPID_Nem_nd_pg_info_hdr_);
        MPIU_CHKPMEM_MALLOC((*pbuf), char *, (*pbuf_len), mpi_errno, "PG Info");
        phdr = (MPID_Nem_nd_pg_info_hdr_t *)*pbuf;
        phdr->pg_rank = my_pg_rank;
        phdr->pg_id_len = 0;
    }
    else{
        /* The processes belong to different pgs. We need to send the
         * pg_id with the pg_info
         */
        MPID_Nem_nd_pg_info_hdr_t   *phdr;
        int pg_id_len = strlen((char *)MPIDI_Process.my_pg->id) + 1;
        *pbuf_len = sizeof(struct MPID_Nem_nd_pg_info_hdr_) + pg_id_len;
        MPIU_CHKPMEM_MALLOC((*pbuf), char *, (*pbuf_len), mpi_errno, "PG Info");
        phdr = (MPID_Nem_nd_pg_info_hdr_t *)*pbuf;
        phdr->pg_rank = my_pg_rank;
        phdr->pg_id_len = pg_id_len;
        MPIU_Strncpy((*pbuf)+sizeof(struct MPID_Nem_nd_pg_info_hdr_), (char *) MPIDI_Process.my_pg->id, pg_id_len);
    }
    MPIU_CHKPMEM_COMMIT();

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_GET_MY_PG_INFO);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Start connecting on the nd conn corresponding to vc
 * Prepost recvs before we connect
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_est
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_est(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno, ret;
    HRESULT hr;
    int val_max_sz, pg_info_len, i;
    MPID_Nem_nd_conn_hnd_t  conn_hnd;
    char *bc, ifname[MAX_HOST_DESCRIPTION_LEN], *pg_info;
    struct sockaddr_in sin;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_EST);
    MPIU_CHKLMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_EST);

    MPIU_Assert(!MPID_NEM_ND_CONN_HND_IS_INIT(MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc)));

    /* This should be done first because at least some ops below can block 
     * Setting VC state to CONNECTING prevents dup connect()s
     */
    MPID_NEM_ND_VCCH_NETMOD_STATE_SET(vc, MPID_NEM_ND_VC_STATE_CONNECTING);

    /* Create a conn - The progress engine will keep track of
     * this connection.
     */
    /* Set tmp conn info in the VC at init time - This might be required by the accept() side
     * to mark this conn as an orphan
     */
    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_CONNECT_CONN, NULL, vc, &conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_C_CONNECTING);

    /* Save the vc info in the conn 
     * Set conn info in vc & vc info in conn after we receive LACK
     */
    conn_hnd->tmp_vc = vc;

    /* We don't handle dynamic conns yet - no tmp vcs*/
    MPIU_Assert(vc->pg != NULL);
    /* Find the address of the remote node
     * Get the business card and get the address of node
     */
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(bc, char *, val_max_sz, mpi_errno, "bc");

    mpi_errno = vc->pg->getConnInfo(vc->pg_rank, bc, val_max_sz, vc->pg);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    memset((void *)&sin, 0x0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;

    ret = MPIU_Str_get_string_arg(bc, MPIDI_CH3I_ND_INTERFACE_KEY, ifname, sizeof(ifname));
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");

    ret = MPIU_Str_get_int_arg(bc, MPIDI_CH3I_ND_PORT_KEY, (int *)&(i));
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");

    sin.sin_port = i;

    mpi_errno = MPIU_SOCKW_Inet_addr(ifname, &(sin.sin_addr.s_addr));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    mpi_errno = get_my_pg_info(vc->pg->id, &pg_info, &pg_info_len);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    SET_EX_WR_HANDLER(conn_hnd, connecting_success_handler, connecting_fail_handler);
    /* Post connect
     * We Send the pg info with the connect & wait for
     * an ACK (Step 1 & 2 of 3-step handshake)
     */
    /* FIXME: Should the private data be valid till the connect() is
     * successful ?
     */
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Connecting to %s:%d(pg_rank=%d, pg_id_len=%d)", ifname, sin.sin_port, ((MPID_Nem_nd_pg_info_hdr_t *)pg_info)->pg_rank, ((MPID_Nem_nd_pg_info_hdr_t *)pg_info)->pg_id_len));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Posting connect on conn(%p) on ov(%p)",
           conn_hnd, MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov))));

    hr = conn_hnd->p_conn->Connect(conn_hnd->p_ep,
            (const struct sockaddr *)&sin, sizeof(struct sockaddr_in),
            MPID_NEM_ND_PROT_FAMILY, 0, (void *)pg_info, pg_info_len,
            MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_connect", "**nd_connect %s %d",
        _com_error(hr).ErrorMessage(), hr);

    if(hr == ND_PENDING){
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "ND_Connect() posted - pending");
    }
    /* FIXME: Free this info ! */
    /* MPIU_Free(pg_info); */
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_EST);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#define MPID_NEM_ND_SM_SKIP_POLL    1
#define FUNCNAME MPID_Nem_nd_sm_poll
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_sm_poll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS;
    BOOL wait_for_event_and_status = FALSE;
    BOOL status = FALSE;
    static int num_skip_polls = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_POLL);

    /* MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_POLL); */
    /* ND progress */
    if(num_skip_polls++ < MPID_NEM_ND_SM_SKIP_POLL){
        goto fn_exit;
    }
    else{
        num_skip_polls = 0;
    }

    do{
        /* Reset event completion status */
        status = FALSE;
        /* On return, if (wait_for_event_and_status == FALSE) then
        * there are no more events in ND Cq
        */
        mpi_errno = MPID_Nem_nd_process_completions(MPID_Nem_nd_dev_hnd_g->p_cq, &status);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }while(status == TRUE);

    /* Ex progress */
    do{
        wait_for_event_and_status = FALSE;
        mpi_errno = MPIU_ExProcessCompletions(MPID_Nem_nd_exset_hnd_g, &wait_for_event_and_status);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }while(wait_for_event_and_status == TRUE);

 fn_exit:
    /* MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_POLL); */
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

/* Note: Blocking operations are costly since we wait for all
 * pending ops, i.e., sends - since we track only sends .
 * FIXME: Alternate method : Keep track of nd progress completions
 * & use the current value of pending ops in conn to determine the
 * the number of operns to block
 */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_sm_block
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_sm_block(MPID_Nem_nd_block_op_hnd_t op_hnd)
{
	int mpi_errno = MPI_SUCCESS;
	BOOL status;
	int npending_ops = 0;
	MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_BLOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_BLOCK);

	/* We need to check conn_hnd status even if block op becomes invalid */
	conn_hnd = op_hnd->conn_hnd;

    MPIU_Assert(conn_hnd->npending_ops == 1);
    /* MPIU_Assert(op_hnd->npending_ops == 1); */
	/* Currently only blocking on pending ex ops */
    /*
    while(conn_hnd->npending_ops > 0){
		HRESULT hr;
		SIZE_T nb=0;

        hr = MPID_Nem_nd_dev_hnd_g->p_cq->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd), &nb, TRUE);
	    MPIU_ERR_CHKANDJUMP(FAILED(hr), mpi_errno, MPI_ERR_OTHER, "**intern");

        do{
            status = FALSE;

            mpi_errno = MPID_Nem_nd_process_completions(MPID_Nem_nd_dev_hnd_g->p_cq, &status);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

		    if(status == FALSE){
 			    mpi_errno = MPIU_ExProcessCompletions(MPID_Nem_nd_exset_hnd_g, &status);
			    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		    }
        }while(status == TRUE);
	}
    */
    while(conn_hnd->npending_ops > 0){
        status = TRUE;
        mpi_errno = MPIU_ExProcessCompletions(MPID_Nem_nd_exset_hnd_g, &status);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        /* Since we only support blocking ops on EX - atleast one op should complete */
        MPIU_Assert(status == TRUE);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_BLOCK);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#define FUNCNAME MPID_Nem_nd_sm_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_sm_finalize(void )
{
    int mpi_errno = MPI_SUCCESS;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_FINALIZE);

    /* Finalize ND device */
    mpi_errno = MPID_Nem_nd_dev_hnd_finalize(&MPID_Nem_nd_dev_hnd_g);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
