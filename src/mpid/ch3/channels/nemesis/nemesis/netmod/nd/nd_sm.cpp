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
static int zcp_send_success_handler(MPID_Nem_nd_msg_result_t *zcp_result);
static int netmod_msg_send_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int zcp_read_fail_handler(MPID_Nem_nd_msg_result_t *send_result);
static int wait_cack_success_handler(MPID_Nem_nd_msg_result_t *recv_result);
static int wait_lack_success_handler(MPID_Nem_nd_msg_result_t *recv_result);
static int recv_success_handler(MPID_Nem_nd_msg_result_t *send_result);
static int dummy_msg_handler(MPID_Nem_nd_msg_result_t *result);
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
}

static inline int MPID_Nem_nd_handle_posted_sendq_head_req(MPIDI_VC_t *vc, int *req_complete);
static int process_pending_req(MPID_Nem_nd_conn_hnd_t conn_hnd);
int MPID_Nem_nd_update_fc_info(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg);

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

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_block_op_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_block_op_init(MPID_Nem_nd_block_op_hnd_t *phnd)
{
    int mpi_errno = MPI_SUCCESS;
    OVERLAPPED *pov;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_BLOCK_OP_INIT);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_BLOCK_OP_INIT);

    MPIU_Assert(phnd != NULL);

    MPIU_CHKPMEM_MALLOC(*phnd, MPID_Nem_nd_block_op_hnd_t, sizeof(struct MPID_Nem_nd_block_op_hnd_), mpi_errno, "Block op hnd");

    MPIU_ExInitOverlapped(&((*phnd)->ex_ov), block_op_handler, block_op_handler);

    pov = MPIU_EX_GET_OVERLAPPED_PTR(&((*phnd)->ex_ov));

    /* Executive initializes event to NULL - So create events after initializing the 
     * handlers
     */
    pov->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    MPIU_ERR_CHKANDJUMP((pov->hEvent == NULL), mpi_errno, MPI_ERR_OTHER, "**intern");

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
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);

    MPIU_Assert(phnd != NULL);
    if(*phnd){
        MPIU_Free(*phnd);
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_BLOCK_OP_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}


/* 
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_block_op_reinit
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_block_op_reinit(MPID_Nem_nd_conn_hnd_t conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    OVERLAPPED *pov;
    BOOL ret;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_BLOCK_OP_REINIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_BLOCK_OP_REINIT);

    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(conn_hnd));

    pov = MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->block_ov));
    MPIU_Assert(pov->hEvent != NULL);

    ret = ResetEvent(pov->hEvent);
    MPIU_ERR_CHKANDJUMP((pov->hEvent == NULL), mpi_errno, MPI_ERR_OTHER, "**intern");

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_BLOCK_OP_REINIT);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
*/

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
    mpi_errno = MPID_Nem_nd_block_op_init(&rsbuf_op_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->rsbuf, sizeof(conn_hnd->rsbuf), MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(rsbuf_op_hnd), &(conn_hnd->rsbuf_hmr));
    if(hr == ND_PENDING){
        SIZE_T nb;
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(rsbuf_op_hnd), &nb, TRUE);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    mpi_errno = MPID_Nem_nd_block_op_init(&ssbuf_op_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->ssbuf, sizeof(conn_hnd->ssbuf), MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(ssbuf_op_hnd), &(conn_hnd->ssbuf_hmr));
    if(hr == ND_PENDING){
        SIZE_T nb;
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(ssbuf_op_hnd), &nb, TRUE);
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

    /* FIXME: REMOVE ME !! -start */
    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn_hnd (%p)", conn_hnd));
    for(i=0; i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "conn_hnd->rsbuf[%d].msg = (%p)", i, &(conn_hnd->rsbuf[i].msg)));
    }
    /* FIXME: REMOVE ME !! -end */

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

        mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        
        hr = new_conn_hnd->p_conn->Accept(new_conn_hnd->p_ep, NULL, 0, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd));
        if(hr == ND_PENDING){
            SIZE_T nb;
            hr = new_conn_hnd->p_conn->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd), &nb, TRUE);
        }
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);

        /* Get a new connector */
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->CreateConnector(&(lconn_hnd->p_conn));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);

        /* Post next req for connection */
        hr = MPID_Nem_nd_dev_hnd_g->p_listen->GetConnectionRequest(lconn_hnd->p_conn, MPIU_EX_GET_OVERLAPPED_PTR(&(lconn_hnd->recv_ov)));
        MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_accept", "**nd_accept %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }
    else{ /* is not blocking */
        MPIU_Assert(0);
        SET_EX_RD_HANDLER(lconn_hnd, listen_success_handler, quiescent_handler);
        hr = MPID_Nem_nd_lconn_hnd->p_conn->Accept(new_conn_hnd->p_ep, NULL, 0, MPIU_EX_GET_OVERLAPPED_PTR(&(MPID_Nem_nd_lconn_hnd->recv_ov)));
        MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
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
    size_t len;
    HRESULT hr;
    char *buf;
    int i;
    unsigned short port;
    char tmp_buf[MAX_HOST_DESCRIPTION_LEN];
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_LISTEN_FOR_CONN);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_LISTEN_FOR_CONN);

    /* Create listen conn */
    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_LISTEN_CONN, NULL, &MPID_Nem_nd_lconn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Listen for connections */
    hr = MPID_Nem_nd_dev_hnd_g->p_ad->Listen(0, MPID_NEM_ND_PROT_FAMILY, 0, &port, &(MPID_Nem_nd_dev_hnd_g->p_listen));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    ret = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_PORT_KEY, port);
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");

    SET_EX_RD_HANDLER(MPID_Nem_nd_lconn_hnd, listen_success_handler, quiescent_handler);

    /* FIXME: How many conn requests should we pre-post ? */
    hr = MPID_Nem_nd_dev_hnd_g->p_listen->GetConnectionRequest(MPID_Nem_nd_lconn_hnd->p_conn, MPIU_EX_GET_OVERLAPPED_PTR(&(MPID_Nem_nd_lconn_hnd->recv_ov)));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_listen", "**nd_listen %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted connection request");
    /* Create Business Card */
    use_default_interface = 1;
    /* MPICH_INTERFACE_HOSTNAME & MPICH_INTERFACE_HOSTNAME_R%d env vars
     * override the default interface selected
     * FIXME: Do we need to change the override env var names ?
     */
    ret = getenv_s(&len, NULL, 0, "MPICH_INTERFACE_HOSTNAME");
    if(ret == ERANGE){
        MPIU_Assert(sizeof(tmp_buf) >= len);
        ret = getenv_s(&len, tmp_buf, sizeof(tmp_buf), "MPICH_INTERFACE_HOSTNAME");
        if(ret == 0){
            use_default_interface = 0;
            ret = MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_ND_INTERFACE_KEY, tmp_buf);
            MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**buscard");
        }
    }

    MPIU_Snprintf(tmp_buf, sizeof(tmp_buf), "MPICH_INTERFACE_HOSTNAME_R%d", pg_rank);
    ret = getenv_s(&len, NULL, 0, tmp_buf);
    if(ret == ERANGE){
        buf = MPIU_Strdup(tmp_buf);
        MPIU_Assert(sizeof(tmp_buf) >= len);
        ret = getenv_s(&len, tmp_buf, sizeof(tmp_buf), buf);
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
        SET_EX_RD_HANDLER(conn_hnd, passive_quiescent_handler, passive_quiescent_handler);

        /* Set the recv sbuf handlers to dummy handlers */
        for(i=0;i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
            SET_MSGBUF_HANDLER(&((conn_hnd->rsbuf[i]).msg), dummy_msg_handler, dummy_msg_handler);
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
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
        SET_EX_WR_HANDLER(conn_hnd, quiescent_handler, quiescent_handler);

        /* Set the recv sbuf handlers to dummy handlers */
        for(i=0;i<MPID_NEM_ND_CONN_RECVQ_SZ;i++){
            SET_MSGBUF_HANDLER(&((conn_hnd->rsbuf[i]).msg), dummy_msg_handler, dummy_msg_handler);
        }

        /* Post disconnect on the ND Conn corresponding to VC */
        hr = conn_hnd->p_conn->Disconnect(MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)));
        MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
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
int MPID_Nem_nd_post_send_msg(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_Nem_nd_msg_t *pmsg, int msg_len, int is_blocking)
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

    /* Update FC info */
    mpi_errno = MPID_Nem_nd_update_fc_info(conn_hnd, pmsg);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    if(is_blocking){
        mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        hr = MPID_Nem_nd_dev_hnd_g->p_cq->Notify(ND_CQ_NOTIFY_ANY, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd));
        MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
            _com_error(hr).ErrorMessage(), hr);

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

    hr = conn_hnd->p_ep->Send(pnd_result, &sge, 1, 0x0);
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);
    if(is_blocking){
        int nresults;
        SIZE_T nb=0;
        ND_RESULT *presult;
        hr = MPID_Nem_nd_dev_hnd_g->p_cq->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd), &nb, TRUE);
        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Sent %d bytes", nb);
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
            _com_error(hr).ErrorMessage(), hr);

        /*
        nresults = MPID_Nem_nd_dev_hnd_g->p_cq->GetResults(&presult, 1);
        MPIU_ERR_CHKANDJUMP2(FAILED(presult->Status),
            mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
            _com_error(presult->Status).ErrorMessage(), presult->Status);
            */
        MPIU_CHKPMEM_COMMIT();
    }
    
    if(was_fc_pkt){
        MPID_NEM_ND_CONN_DECR_SCREDITS(conn_hnd);
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
        /*
        mpi_errno = MPID_Nem_nd_update_fc_info(conn_hnd, pfc_msg);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        */

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
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Request *zcp_req = NULL;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_ZCP_SEND_MSGRESULT(zcp_send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    zcp_req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);

    /* MW created, Registered buf, Bound MW, now post send */
    mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, zcp_req->dev.iov, zcp_req->dev.iov_count);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_BIND_MW_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME reg_zcp_mem_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int reg_zcp_mem_success_handler(MPIU_EXOVERLAPPED *send_ov)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_msg_result_t *pmsg_result;
    MPID_Request *zcp_req = NULL;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_MEM_SUCCESS_HANDLER);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_MEM_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_EX_SEND_OV(send_ov);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    MPIU_CHKPMEM_MALLOC(pmsg_result, MPID_Nem_nd_msg_result_t *, sizeof(MPID_Nem_nd_msg_result_t ), mpi_errno, "cr_mem_win result");
    INIT_MSGRESULT(pmsg_result, free_msg_result_handler, free_msg_result_handler);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->CreateMemoryWindow(&(pmsg_result->result), &(conn_hnd->zcp_send_mw));
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPIU_CHKPMEM_COMMIT();

    zcp_req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);

    conn_hnd->zcp_msg_send_mw.mw_data.Base = 0;
    conn_hnd->zcp_msg_send_mw.mw_data.Length = 0;
    conn_hnd->zcp_msg_send_mw.mw_data.Token = 0;
    /* MW created, mem registered, now bind the buffer */
    /* FIXME: Do we need a read fence ? */
    INIT_MSGRESULT(&(conn_hnd->zcp_send_result), bind_mw_success_handler, gen_send_fail_handler);
    hr = conn_hnd->p_ep->Bind(&(conn_hnd->zcp_send_result.result), conn_hnd->zcp_send_mr_hnd,
            conn_hnd->zcp_send_mw, zcp_req->dev.iov[1].MPID_IOV_BUF,
            zcp_req->dev.iov[1].MPID_IOV_LEN, ND_OP_FLAG_ALLOW_READ, &(conn_hnd->zcp_msg_send_mw.mw_data));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_REG_ZCP_MEM_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
/*
#undef FUNCNAME
#define FUNCNAME create_mw_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int create_mw_success_handler(MPID_Nem_nd_msg_result_t *pmsg_result)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Request *zcp_req = NULL;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_CREATE_MW_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_CREATE_MW_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_ZCP_MSGRESULT(pmsg_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    / The request at the tail of the posted queue should contain
     * the buffer
     /
    zcp_req = MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_TAIL(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);

    SET_EX_WR_HANDLER(conn_hnd, reg_zcp_mem_success_handler, gen_ex_fail_handler);
    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(zcp_req->dev.iov[1].MPID_IOV_BUF,
        zcp_req->dev.iov[1].MPID_IOV_LEN, MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)),
        &(conn_hnd->zcp_mr_hnd));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_CREATE_MW_SUCCESS_HANDLER);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
*/
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_start_zcp
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_start_zcp(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_START_ZCP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_START_ZCP);

    /* Register buffer */
    /* FIXME: We only register 1 IOV for now */
    MPIU_Assert(n_iov == 1);

    SET_EX_WR_HANDLER(conn_hnd, reg_zcp_mem_success_handler, gen_ex_fail_handler);
    hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(iov[0].MPID_IOV_BUF,
        iov[0].MPID_IOV_LEN, MPIU_EX_GET_OVERLAPPED_PTR(&(conn_hnd->send_ov)),
        &(conn_hnd->zcp_send_mr_hnd));
    MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_START_ZCP);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_post_sendv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_post_sendv(MPID_Nem_nd_conn_hnd_t conn_hnd, MPID_IOV *iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    errno_t ret_errno;
    char *p;
    MPID_Nem_nd_msg_t *pmsg;
    int i, rem_len = 0, msg_len = 0, tot_len = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_POST_SENDV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_POST_SENDV);

    if(!conn_hnd->zcp_in_progress){
        int start_zcp=0;
	    tot_len = 0;
	    for(i=0; i<n_iov; i++){
		    tot_len += iov[i].MPID_IOV_LEN;
		    if(tot_len > MPID_NEM_ND_CONN_UDATA_SZ) {
                start_zcp = 1;
                break;
		    }
	    }
        if(!start_zcp){
	        /* Get a msgbuf - pack the iovs into it and send it */
		    MPIU_Assert(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd));
		    MSGBUF_FREEQ_DEQUEUE(conn_hnd, pmsg);
		    MPIU_Assert(pmsg != NULL);
            SET_MSGBUF_HANDLER(pmsg, send_success_handler, gen_send_fail_handler);

		    pmsg->hdr.type = MPID_NEM_ND_DATA_PKT;
            pmsg->hdr.credits = 0;
		    p = pmsg->buf;
		    rem_len = MPID_NEM_ND_CONN_UDATA_SZ;
		    msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );

		    for(i=0; i<n_iov; i++){
			    int iov_len = iov[i].MPID_IOV_LEN;
                /* rem_len is never less than iov_len */
                MPIU_Assert(rem_len >= iov_len);
				/* Copy the whole iov to the msg buffer */
				ret_errno = memcpy_s((void *)p, rem_len, iov[i].MPID_IOV_BUF, iov_len);
				MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,
					"**nd_write", "**nd_write %s %d", strerror(ret_errno), ret_errno);
				p += iov_len;
				rem_len -= iov_len;
				msg_len += iov_len;
		    }
		    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending msg packet of size %d (msg type=%d)", msg_len, pmsg->hdr.type));
		    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending udata packet of type  = %d", ((MPIDI_CH3_Pkt_t *)(&(pmsg->buf)))->type));

		    mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, msg_len, 0);
		    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        else{ /* start_zcp */
            conn_hnd->zcp_in_progress = 1;
            /* Don't send data till the zcpy is over */
            conn_hnd->zcp_credits = conn_hnd->send_credits;
            conn_hnd->send_credits = 0;

            /* FIXME: Only handling 1 IOV now */
            mpi_errno = MPID_Nem_nd_start_zcp(conn_hnd, &(iov[1]), 1);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
	}
    else{ /* zcopy in progress */
        MPID_Nem_nd_msg_mw_t msg_mw;
        /* zcopy init should be complete by now - send hdr and MPID_Nem_nd_msg_mw_t
         * related to the data.
         */
        MPIU_Assert(!MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd));
        MSGBUF_FREEQ_DEQUEUE(conn_hnd, pmsg);
        SET_MSGBUF_HANDLER(pmsg, zcp_mw_send_success_handler, gen_send_fail_handler);
        MPIU_Assert(pmsg != NULL);
        /* FIXME: Support more than 2 iovs */
        MPIU_Assert(n_iov == 2);

        pmsg->hdr.type = MPID_NEM_ND_RD_AVAIL_PKT;
        pmsg->hdr.credits = 0;
        msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
        p = pmsg->buf;
        rem_len = MPID_NEM_ND_CONN_UDATA_SZ;
        /* Try to copy the first IOV to the msg packet */
        if(iov[0].MPID_IOV_LEN <= rem_len){
            ret_errno = memcpy_s((void *)p, rem_len, iov[0].MPID_IOV_BUF, iov[0].MPID_IOV_LEN);
            MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,    
                "**nd_write", "**nd_write %s %d", strerror(ret_errno), ret_errno);
            p += iov[0].MPID_IOV_LEN;
            rem_len -= iov[0].MPID_IOV_LEN;
            msg_len += iov[0].MPID_IOV_LEN;
        }
        /* We are guaranteed to have enough space for the MW descriptors */
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Sending mem descriptor (buf=%p) : base = %p, length=%I64d, token=%d\n",
            iov[1].MPID_IOV_BUF,
            _byteswap_uint64(conn_hnd->zcp_msg_send_mw.mw_data.Base), _byteswap_uint64(conn_hnd->zcp_msg_send_mw.mw_data.Length),
            conn_hnd->zcp_msg_send_mw.mw_data.Token));

        ret_errno = memcpy_s((void *)p, sizeof(MPID_Nem_nd_msg_mw_t ), &(conn_hnd->zcp_msg_send_mw), sizeof(MPID_Nem_nd_msg_mw_t ));
        MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,    
            "**nd_write", "**nd_write %s %d", strerror(ret_errno), ret_errno);
        /* FIXME: Add mw for other iovs */
        p += sizeof(MPID_Nem_nd_msg_mw_t );
        msg_len += sizeof(MPID_Nem_nd_msg_mw_t );
		    
        mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, msg_len, 0);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
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

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_PROCESS_COMPLETIONS);

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
            /* An error */
            *pstatus = FALSE;
            break;
        }
        /* An Event completed */
        *pstatus = TRUE;

        hr = nd_results[0]->Status;
        pmsg_result = GET_MSGRESULT_FROM_NDRESULT(nd_results[0]);
        MPIU_Assert(pmsg_result != NULL);
        /* FIXME: REMOVE ME !! -start */
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Got something on %p", GET_MSGBUF_FROM_MSGRESULT(pmsg_result)));
        /* FIXME: REMOVE ME !! -end */
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
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_PROCESS_COMPLETIONS);
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
#define FUNCNAME zcp_read_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_read_success_handler(MPID_Nem_nd_msg_result_t *send_result)
{
    int mpi_errno = MPI_SUCCESS, ret_errno=0;
    MPID_Nem_nd_conn_hnd_t conn_hnd;
    MPID_Nem_nd_block_op_hnd_t dereg_op_hnd;
    MPID_Nem_nd_msg_t *pmsg;
    MPID_Request *zcp_req;
    int (*req_fn)(MPIDI_VC_t *, MPID_Request *, int *);
    int req_complete=0;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_READ_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    zcp_req = MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc);
    MPIU_Assert(zcp_req != NULL);

    /* Call req handler and send Rd complete pkt */
    req_fn = zcp_req->dev.OnDataAvail;
    if(req_fn){
        mpi_errno = req_fn(conn_hnd->vc, zcp_req, &req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        MPIU_Assert(req_complete);
    }
    else{
        MPIDI_CH3U_Request_complete(zcp_req);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Req - RDMA Rd - complete...");
        MPID_NEM_ND_VCCH_SET_ACTIVE_RECV_REQ(conn_hnd->vc, NULL);
    }

    /* Unregister user memory */
    mpi_errno = MPID_Nem_nd_block_op_init(&dereg_op_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->DeregisterMemory(conn_hnd->zcp_recv_sge.hMr,
            MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(dereg_op_hnd));
    if(hr == ND_PENDING){
        SIZE_T nb;
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(dereg_op_hnd), &nb, TRUE);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
        _com_error(hr).ErrorMessage(), hr);

    /* Use the msg & send rd complete pkt */
    pmsg->hdr.type = MPID_NEM_ND_RD_ACK_PKT;

    SET_MSGBUF_HANDLER(pmsg, netmod_msg_send_success_handler, gen_send_fail_handler);
    mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, pmsg, sizeof(MPID_Nem_nd_msg_hdr_t ), 0);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
    MPID_Nem_nd_block_op_hnd_t dereg_op_hnd;
    MPID_Nem_nd_msg_t *pmsg;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_INVALIDATE_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_MW_INVALIDATE_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    /* Allow sends on the conn */
    conn_hnd->zcp_in_progress = 0;
    conn_hnd->send_credits = conn_hnd->zcp_credits;

    /* Repost the recv buf */
    pmsg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(pmsg != NULL);

    SET_MSGBUF_HANDLER(pmsg, recv_success_handler, gen_recv_fail_handler);
    mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    /* Deregister memory */
    mpi_errno = MPID_Nem_nd_block_op_init(&dereg_op_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = MPID_Nem_nd_dev_hnd_g->p_ad->DeregisterMemory(conn_hnd->zcp_send_mr_hnd,
            MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(dereg_op_hnd));
    if(hr == ND_PENDING){
        SIZE_T nb;
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(dereg_op_hnd), &nb, TRUE);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_write", "**nd_write %s %d",
        _com_error(hr).ErrorMessage(), hr);

    /* Call the send success handler for zcp transfer */
    mpi_errno = zcp_send_success_handler(&(conn_hnd->zcp_send_result));
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
#define FUNCNAME zcp_send_success_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int zcp_send_success_handler(MPID_Nem_nd_msg_result_t *zcp_send_result)
{
    int mpi_errno = MPI_SUCCESS;
    int req_complete;
    MPID_Nem_nd_conn_hnd_t conn_hnd;

    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_ZCP_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_ZCP_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_ZCP_SEND_MSGRESULT(zcp_send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    if(!MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_EMPTY(conn_hnd->vc)){
        mpi_errno = MPID_Nem_nd_handle_posted_sendq_head_req(conn_hnd->vc, &req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    /* If we have queued sends and credits to send data - go ahead with sending */
    if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send credits available. Processing queued req...");
        mpi_errno = process_pending_req(conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_ZCP_SEND_SUCCESS_HANDLER);
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
        mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, &(req->dev.iov[req->dev.iov_offset]), req->dev.iov_count);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_DEQUEUE(vc, &req);
        *req_complete = 1;
    }
    else{
        *req_complete = 0;
        mpi_errno = req_handler(vc, req, req_complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
        if (*req_complete){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
            MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_DEQUEUE(vc, &req);
        }
    }
    req->dev.iov_offset = 0;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_HANDLE_POSTED_SENDQ_HEAD_REQ);
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
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_SEND_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_SEND_SUCCESS_HANDLER);

    conn_hnd = GET_CONNHND_FROM_MSGRESULT(send_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    pmsg = GET_MSGBUF_FROM_MSGRESULT(send_result);
    MPIU_Assert(pmsg != NULL);

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send succeeded...");

    if(conn_hnd->vc != NULL){
        /* Increment number of available send credits only when a credit packet is recvd */
        /* Complete the request associated with this send if no pending events */
        mpi_errno = MPID_Nem_nd_handle_posted_sendq_head_req(conn_hnd->vc, &req_complete);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /* Enqueue this send buffer to freeq */
        MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

        /* If we have queued sends and credits to send data - go ahead with sending */
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send credits available. Processing queued req...");
            mpi_errno = process_pending_req(conn_hnd);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
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

    MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Send succeeded...");

    if(conn_hnd->vc != NULL){
        /* Increment number of available send credits only when a credit packet is recvd */
        /* There is no request associated with this send - Netmod msg */
        /* Enqueue this send buffer to freeq */
        MSGBUF_FREEQ_ENQUEUE(conn_hnd, pmsg);

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
        MPIDI_CH3I_VC *vc_ch;

        vc = conn_hnd->vc;
        vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;

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

    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_ACCEPT_CONN, lconn_hnd->p_conn, &new_conn_hnd);
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
    }
    else{
        /* FIXME: TODO */
        MPIU_Assert(0);
    }

    vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    if(MPID_NEM_ND_CONN_HND_IS_VALID(MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc))){
        if(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) != MPID_NEM_ND_VC_STATE_CONNECTED){
            /* VC is connecting - head-to-head scenario */
            MPID_Nem_nd_conn_hnd_t old_conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
            int old_conn_won_hh=0;
            mpi_errno = MPID_Nem_nd_resolve_head_to_head(pg_info->pg_rank, pg, pg_id, &old_conn_won_hh);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "H-H: Old conn (%p:%d) & new conn (%p:%d)", old_conn_hnd, old_conn_hnd->state, new_conn_hnd, new_conn_hnd->state));
            if(old_conn_won_hh){
                /* Won head to head with new conn */
                /* Send a NAK and close the new conn */
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Old conn (%p) won head to head", MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc)));
                terminate_conn = 1;
            }
            else{
                /* Lost head to head with new conn */
                /* Make old conn orphan - The other side with send
                 * us a LNAK and we can close the old conn
                 */
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "New conn (%p) won head to head", new_conn_hnd));

                MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, new_conn_hnd);
                new_conn_hnd->vc = vc;
                terminate_conn = 0;
            }
        }
        else{
            /* VC is already connected */
            terminate_conn = 1;
        }
    }
    else{
        /* No conn associated with this vc */
        /* Associate vc with this connection */
        MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, new_conn_hnd);
        /* Associate this conn with the vc */
        new_conn_hnd->vc = vc;
        terminate_conn = 0;
    }

    if(terminate_conn){
		/* New conn lost head to head OR VC is already connected
         * Do a blocking send for LNAK & disc
         */
		MPID_Nem_nd_msg_t *pmsg;
		int msg_len=0;

		/* Post a LACK - do a blocking send */
		MSGBUF_FREEQ_DEQUEUE(new_conn_hnd, pmsg);
        MPIU_Assert(pmsg != NULL);
		pmsg->hdr.type = MPID_NEM_ND_CONN_NAK_PKT;
		msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
		mpi_errno = MPID_Nem_nd_post_send_msg(new_conn_hnd, pmsg, msg_len, 1);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		MSGBUF_FREEQ_ENQUEUE(new_conn_hnd, pmsg);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Passive disc on (%p)", new_conn_hnd));
        /* Wait for a disconnect from the other side and free resources */
        mpi_errno = MPID_Nem_nd_conn_passive_disc(new_conn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

        /*
		mpi_errno = MPID_Nem_nd_conn_disc(new_conn_hnd);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        */
    }
    else{
        /* Connection successful - send LACK & wait for CACK */
        MPID_Nem_nd_msg_t *pmsg;
        int msg_len=0;

		MPID_NEM_ND_CONN_STATE_SET(new_conn_hnd, MPID_NEM_ND_CONN_WAIT_CACK);
		/* Grab the head of the recv ssbufs and set its handlers */
		pmsg = GET_RECV_SBUF_HEAD(new_conn_hnd);
		MPIU_Assert(pmsg != NULL);
		SET_MSGBUF_HANDLER(pmsg, wait_cack_success_handler, gen_recv_fail_handler);

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Setting the wait_cack recv handler for msg_buf = %p", pmsg));

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Post LACK wait for CACK on (%p)", new_conn_hnd));

		/* Post a LACK - do a blocking send */
		MSGBUF_FREEQ_DEQUEUE(new_conn_hnd, pmsg);
		pmsg->hdr.type = MPID_NEM_ND_CONN_ACK_PKT;
		msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
		mpi_errno = MPID_Nem_nd_post_send_msg(new_conn_hnd, pmsg, msg_len, 1);
		if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
		MSGBUF_FREEQ_ENQUEUE(new_conn_hnd, pmsg);
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
    MPIDI_CH3I_VC *vc_ch;
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
            int msg_len=0;
            /* VC is now connected - send an ACK - CACK - to listen side */
            vc = conn_hnd->vc;
            vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
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
            /* We block till the ACK is sent */
            mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, psend_msg, msg_len, 1);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            MSGBUF_FREEQ_ENQUEUE(conn_hnd, psend_msg);

            /* Repost receive on the used msg buf */
            SET_MSGBUF_HANDLER(precv_msg, recv_success_handler, gen_recv_fail_handler);
            mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, precv_msg);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            /* FIXME: See if there is any queued data to send */
            mpi_errno = process_pending_req(conn_hnd);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
        else{
            /* Received LNAK - Close connection - silently */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LNAK - Closing connection");

            MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);
            mpi_errno = MPID_Nem_nd_conn_disc(conn_hnd);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
        }
    }
    else{
        /* Send NAK - We lost in head to head connection to the listen side */
        MPIU_Assert((precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT) ||
                    (precv_msg->hdr.type == MPID_NEM_ND_CONN_NAK_PKT));
        /* vc is already disconnected from conn */
        MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_QUIESCENT);

        if(precv_msg->hdr.type == MPID_NEM_ND_CONN_ACK_PKT){
            /* Send a CNAK and disc */
            MPID_Nem_nd_msg_t   *psend_msg;
            int msg_len=0;

            /* Blocking send for CNAK */
            MPIU_Assert(!MSGBUF_FREEQ_IS_EMPTY(conn_hnd));
            MSGBUF_FREEQ_DEQUEUE(conn_hnd, psend_msg);
            MPIU_Assert(psend_msg != NULL);

            psend_msg->hdr.type = MPID_NEM_ND_CONN_NAK_PKT;
            msg_len = sizeof(MPID_Nem_nd_msg_hdr_t );
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LACK - Lost HH - Sending CNAK & Closing connection");

            /* We block till the ACK is sent */
            mpi_errno = MPID_Nem_nd_post_send_msg(conn_hnd, psend_msg, msg_len, 1);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            MSGBUF_FREEQ_ENQUEUE(conn_hnd, psend_msg);
        }
        else{
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received LNAK - Lost HH - Closing connection");
        }
        /* Close connection - silently */
        mpi_errno = MPID_Nem_nd_conn_disc(conn_hnd);
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
    MPID_Nem_nd_block_op_hnd_t zcp_op_hnd;
    MPID_Request *rreq = NULL;
    int nb, udata_len=0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_RECV_SUCCESS_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_RECV_SUCCESS_HANDLER);

    pmsg = GET_MSGBUF_FROM_MSGRESULT(recv_result);
    MPIU_Assert(pmsg != NULL);
    conn_hnd = GET_CONNHND_FROM_MSGRESULT(recv_result);
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd));

    nb = GET_NB_FROM_MSGRESULT(recv_result);
    MPIU_ERR_CHKANDJUMP(nb == 0, mpi_errno, MPI_ERR_OTHER, "**nd_write");

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Recvd %d bytes (msg type=%d)",nb, pmsg->hdr.type));

    MPIU_Assert(nb >= sizeof(MPID_Nem_nd_msg_hdr_t ));
    if(!conn_hnd->zcp_in_progress){
        conn_hnd->send_credits += pmsg->hdr.credits;
    }
    else{
        conn_hnd->zcp_credits += pmsg->hdr.credits;
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
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Received DATA PKT (len = %d, credits = %d)",udata_len, pmsg->hdr.credits));

                /* The msg just contains the type and udata */
                mpi_errno = MPID_nem_handle_pkt(conn_hnd->vc, pmsg->buf, udata_len);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

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
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Received RD Avail pkt (len=%d)", udata_len);
                udata_len -= sizeof(MPID_Nem_nd_msg_mw_t);
                mpi_errno = MPID_nem_handle_pkt(conn_hnd->vc, pmsg->buf, udata_len);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                
                rreq = MPID_NEM_ND_VCCH_GET_ACTIVE_RECV_REQ(conn_hnd->vc);

                MPIU_Assert(rreq != NULL);

                ret_errno = memcpy_s((void *)&(conn_hnd->zcp_msg_recv_mw), sizeof(MPID_Nem_nd_msg_mw_t ), (void *)&(pmsg->buf[udata_len]), sizeof(MPID_Nem_nd_msg_mw_t));
				MPIU_ERR_CHKANDJUMP2((ret_errno != 0), mpi_errno, MPI_ERR_OTHER,
					"**nd_read", "**nd_read %s %d", strerror(ret_errno), ret_errno);

                /* Repost recv buffer */
                if(conn_hnd->vc != NULL){
                    mpi_errno = MPID_Nem_nd_post_recv_msg(conn_hnd, pmsg);
                    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
                }

                /* A msg buf is guaranteed for RDMA read */
                MSGBUF_FREEQ_DEQUEUE(conn_hnd, pzcp_msg);
                MPIU_Assert(pzcp_msg != NULL);
                
                SET_MSGBUF_HANDLER(pzcp_msg, zcp_read_success_handler, zcp_read_fail_handler);

                /* FIXME: We just support 1 IOV for now */
                conn_hnd->zcp_recv_sge.Length = rreq->dev.iov[rreq->dev.iov_offset].MPID_IOV_LEN;
                conn_hnd->zcp_recv_sge.pAddr = rreq->dev.iov[rreq->dev.iov_offset].MPID_IOV_BUF;
                /* Registering the local IOV */
                mpi_errno = MPID_Nem_nd_block_op_init(&zcp_op_hnd);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

                hr = MPID_Nem_nd_dev_hnd_g->p_ad->RegisterMemory(conn_hnd->zcp_recv_sge.pAddr, conn_hnd->zcp_recv_sge.Length, MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(zcp_op_hnd), &(conn_hnd->zcp_recv_sge.hMr));
                if(hr == ND_PENDING){
                    SIZE_T nb;
                    hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(zcp_op_hnd), &nb, TRUE);
                }
                MPIU_ERR_CHKANDJUMP2(FAILED(hr),
                    mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
                    _com_error(hr).ErrorMessage(), hr);

                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Performing RDMA read for %d bytes", conn_hnd->zcp_recv_sge.Length));
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Using remote mem descriptor : base = %p, length=%I64d, token=%d",
                    _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Base), _byteswap_uint64(conn_hnd->zcp_msg_recv_mw.mw_data.Length),
                    conn_hnd->zcp_msg_recv_mw.mw_data.Token));

                hr = conn_hnd->p_ep->Read(GET_PNDRESULT_FROM_MSGBUF(pzcp_msg), &(conn_hnd->zcp_recv_sge), 1,
                    &(conn_hnd->zcp_msg_recv_mw.mw_data), 0, 0x0);
                MPIU_ERR_CHKANDJUMP2((hr != ND_PENDING) && FAILED(hr),
                    mpi_errno, MPI_ERR_OTHER, "**nd_read", "**nd_read %s %d",
                    _com_error(hr).ErrorMessage(), hr);

                break;
        case MPID_NEM_ND_RD_ACK_PKT:
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Received RD ACK pkt");
                /* Get the send credits for conn */
                MPIU_Assert(udata_len == 0);
                /* Save the credits in the RD ack pkt */
                /* conn_hnd->zcp_credits = pmsg->hdr.credits; */
                /* Invalidate/unbind the address */
                SET_MSGBUF_HANDLER(pmsg, zcp_mw_invalidate_success_handler, gen_recv_fail_handler);
                hr = conn_hnd->p_ep->Invalidate(GET_PNDRESULT_FROM_MSGBUF(pmsg), conn_hnd->zcp_send_mw, 0x0);
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
    mpi_errno = MPID_Nem_nd_block_op_init(&op_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    hr = conn_hnd->p_conn->CompleteConnect(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd));
    if(hr == ND_PENDING){
        SIZE_T nb;
        hr = MPID_Nem_nd_dev_hnd_g->p_ad->GetOverlappedResult(MPID_NEM_ND_BLOCK_OP_GET_OVERLAPPED_PTR(op_hnd), &nb, TRUE);
    }
    MPIU_ERR_CHKANDJUMP2(FAILED(hr),
        mpi_errno, MPI_ERR_OTHER, "**nd_connect", "**nd_connect %s %d",
        _com_error(hr).ErrorMessage(), hr);

    MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_WAIT_LACK);
    /* Receive is already pre-posted. Set the handlers correctly and wait
     * for LACK from the other process
     */
    pmsg = GET_RECV_SBUF_HEAD(conn_hnd);
    MPIU_Assert(pmsg != NULL);
    SET_MSGBUF_HANDLER(pmsg, wait_lack_success_handler, gen_recv_fail_handler);

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "Setting the wait_lack recv handler for msg_buf = %p", pmsg));

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
    if(MPID_NEM_ND_CONN_HND_IS_VALID(conn_hnd)){
        MPID_Nem_nd_conn_hnd_finalize(MPID_Nem_nd_dev_hnd_g, &conn_hnd);
    }

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

    MPID_Nem_nd_block_op_finalize(&hnd);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_BLOCK_OP_HANDLER);
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
/* FIXME - Remove
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_resolve_remote_addr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_resolve_remote_addr(MPID_Nem_nd_conn_hnd_t conn_hnd,
                                    struct sockaddr *punresolved_sin,
                                    int unresolved_sin_len,
                                    struct sockaddr *presolved_sin,
                                    int resolved_sin_len)
{
    int mpi_errno = MPI_SUCCESS, ret, len;
    SOCKET s;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_RESOLVE_REMOTE_ADDR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_RESOLVE_REMOTE_ADDR);

    s = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    MPIU_ERR_CHKANDJUMP2((s == INVALID_SOCKET), mpi_errno, MPI_ERR_OTHER,
        "**sock_create", "**sock_create %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()), MPIU_OSW_Get_errno());

    ret = WSAIoctl(s, SIO_ROUTING_INTERFACE_QUERY,
        (void *)punresolved_sin, (DWORD )unresolved_sin_len,
        (void *)presolved_sin, (DWORD )resolved_sin_len, (DWORD *)&len, NULL, NULL);
    MPIU_ERR_CHKANDJUMP2((ret == SOCKET_ERROR), mpi_errno, MPI_ERR_OTHER,
        "**ioctl_socket", "**ioctl_socket %s %d",
        MPIU_OSW_Strerror(MPIU_OSW_Get_errno()), MPIU_OSW_Get_errno());

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_RESOLVE_REMOTE_ADDR);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
*/

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

    /* Create a conn - The progress engine will keep track of
     * this connection.
     */
    mpi_errno = MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_g, MPID_NEM_ND_CONNECT_CONN, NULL, &conn_hnd);
    if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

    MPID_NEM_ND_CONN_STATE_SET(conn_hnd, MPID_NEM_ND_CONN_C_CONNECTING);
    /* Set VC's conn to this conn */
    MPID_NEM_ND_VCCH_NETMOD_CONN_HND_SET(vc, conn_hnd);
    /* This connection is related to this vc. If this connection
     * loses in a head to head battle this conn will still point to
     * the vc, however the vc will no longer point to this conn
     * making this conn an ORPHAN CONN
     */
    conn_hnd->vc = vc;

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
int MPID_Nem_nd_sm_poll(void )
{
    int mpi_errno = MPI_SUCCESS;
    BOOL wait_for_event_and_status = FALSE;
    BOOL status = FALSE;
    static int num_skip_polls = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SM_POLL);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SM_POLL);
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
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SM_POLL);
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
