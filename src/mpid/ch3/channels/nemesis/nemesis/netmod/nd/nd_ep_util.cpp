/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

/* p_conn is the connector used when initializing conn hnd pointed by pconn_hnd */
#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_conn_hnd_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_hnd_init(MPID_Nem_nd_dev_hnd_t dev_hnd, MPID_Nem_nd_conn_type_t conn_type, INDConnector *p_conn, MPID_Nem_nd_conn_hnd_t *pconn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    HRESULT hr;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_HND_INIT);
    MPIU_CHKPMEM_DECL(1);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_HND_INIT);

    MPIU_Assert(MPID_NEM_ND_DEV_HND_IS_VALID(dev_hnd));

    /* Allocate memory for conn handle */
    MPIU_Assert(pconn_hnd != NULL);
    MPIU_CHKPMEM_MALLOC((*pconn_hnd), MPID_Nem_nd_conn_hnd_t , sizeof(MPID_Nem_nd_conn_hnd_), mpi_errno, "ND Conn handle");

    MPID_NEM_ND_CONN_STATE_SET((*pconn_hnd), MPID_NEM_ND_CONN_QUIESCENT);
    if(p_conn == NULL){
        /* Create a new connector */
        hr = (dev_hnd->p_ad)->CreateConnector(&((*pconn_hnd)->p_conn));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_ep_create", "**nd_ep_create %s %d",
            _com_error(hr).ErrorMessage(), hr);
    }
    else{
        /* Use the connector passed by the caller */
        (*pconn_hnd)->p_conn = p_conn;
    }

    MPID_NEM_ND_CONN_STATE_SET((*pconn_hnd), MPID_NEM_ND_CONN_QUIESCENT);
    (*pconn_hnd)->vc = NULL;
    MPIU_ExInitOverlapped(&((*pconn_hnd)->recv_ov), NULL, NULL);
    MPIU_ExInitOverlapped(&((*pconn_hnd)->send_ov), NULL, NULL);

	(*pconn_hnd)->npending_ops = 0;
    (*pconn_hnd)->zcp_in_progress = 0;

    /* Create an endpoint - listen conns don't need an endpoint */
    if((conn_type == MPID_NEM_ND_CONNECT_CONN) || (conn_type == MPID_NEM_ND_ACCEPT_CONN)){
        hr = (*pconn_hnd)->p_conn->CreateEndpoint(dev_hnd->p_cq, dev_hnd->p_cq,
            MPID_NEM_ND_CONN_RECVQ_SZ, MPID_NEM_ND_CONN_SENDQ_SZ,
            MPID_NEM_ND_CONN_SGE_MAX, MPID_NEM_ND_CONN_SGE_MAX,
            MPID_NEM_ND_CONN_RDMA_RD_MAX, MPID_NEM_ND_CONN_RDMA_RD_MAX,
            NULL, &((*pconn_hnd)->p_ep));
        MPIU_ERR_CHKANDJUMP2(FAILED(hr),
            mpi_errno, MPI_ERR_OTHER, "**nd_ep_create", "**nd_ep_create %s %d",
            _com_error(hr).ErrorMessage(), hr);

        mpi_errno = MPID_Nem_nd_conn_msg_bufs_init(*pconn_hnd);
        if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
    }

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_HND_INIT);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#define FUNCNAME MPID_Nem_nd_conn_hnd_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_conn_hnd_finalize(MPID_Nem_nd_dev_hnd_t dev_hnd, MPID_Nem_nd_conn_hnd_t *p_conn_hnd)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_CONN_HND_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_CONN_HND_FINALIZE);

    MPIU_Assert(MPID_NEM_ND_DEV_HND_IS_VALID(dev_hnd));
    MPIU_Assert(p_conn_hnd != NULL);
    /* Conn handle may not be valid here since a finalize() is
     * allowed even if init() fails
     */
    MPIU_Assert(MPID_NEM_ND_CONN_HND_IS_INIT(*p_conn_hnd));
    if((*p_conn_hnd)->p_ep){
        /* Release endpoint */
        (*p_conn_hnd)->p_ep->Release();
    }
    if((*p_conn_hnd)->p_conn){
        /* Release connector */
        (*p_conn_hnd)->p_conn->Release();
    }

    *p_conn_hnd = MPID_NEM_ND_CONN_HND_INVALID;
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_CONN_HND_FINALIZE);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}
