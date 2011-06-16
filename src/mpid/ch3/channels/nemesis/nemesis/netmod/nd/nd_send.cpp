/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_istart_contig_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_istart_contig_msg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreqp_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreqp = NULL;
    int is_send_posted = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ISTART_CONTIG_MSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ISTART_CONTIG_MSG);
    MPIU_Assert((hdr_sz > 0) && (hdr_sz <= sizeof(MPIDI_CH3_Pkt_t)));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "istart_contig_msg (hdr_sz=" MPIDI_MSG_SZ_FMT ",data_sz=" MPIDI_MSG_SZ_FMT ")", hdr_sz, data_sz));

    /* Create a request and queue it */
    sreqp = MPID_Request_create();
    MPIU_Assert(sreqp != NULL);
    MPIU_Object_set_ref(sreqp, 2);
    sreqp->kind = MPID_REQUEST_SEND;

    sreqp->dev.OnDataAvail = NULL;
    sreqp->ch.vc = vc;
    sreqp->dev.iov_offset = 0;

    sreqp->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
    sreqp->dev.iov[0].MPID_IOV_BUF = (char *)&sreqp->dev.pending_pkt;
    sreqp->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    if(data_sz > 0){
        sreqp->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
        sreqp->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreqp->dev.iov_count = 2;
    }
    else{
        sreqp->dev.iov_count = 1;
    }

    is_send_posted = 0;
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        MPID_Nem_nd_conn_hnd_t conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
        /* Try sending data - if no credits queue the remaining data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connected - trying to send data");
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            /* We have send credits */
            /* Post a send for data & queue request */
            if(!MPID_NEM_ND_IS_BLOCKING_REQ(sreqp)){
                mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            else{
                mpi_errno = MPID_Nem_nd_post_sendbv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }

            /* Now queue the request in posted queue */
            is_send_posted = 1;
        }
    }
    else{
        /* VC is not connected */
        if(MPID_NEM_ND_VC_IS_CONNECTING(vc)){
            /* Already connecting - just queue req in pending queue */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connecting - queueing data");
        }
        else{
            MPIU_Assert(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_DISCONNECTED);
            /* Start connecting and queue req in pending queue */
            mpi_errno = MPID_Nem_nd_conn_est(vc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted a connect - queueing data");
        }
    }

    if(is_send_posted){
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(vc, sreqp);
    }
    else{
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(vc, sreqp);
    }

    *sreqp_ptr = sreqp;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_ISTART_CONTIG_MSG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_send_contig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_send_contig(MPIDI_VC_t *vc, MPID_Request *sreqp, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int is_send_posted = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SEND_CONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SEND_CONTIG);
    MPIU_Assert((hdr_sz > 0) && (hdr_sz <= sizeof(MPIDI_CH3_Pkt_t)));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "isend_contig_msg (hdr_sz=" MPIDI_MSG_SZ_FMT ",data_sz=" MPIDI_MSG_SZ_FMT ")", hdr_sz, data_sz));
    /* FIXME: Update the req dev iov fields only for unposted sends
     */
    sreqp->ch.vc = vc;
    sreqp->dev.iov_offset = 0;
    sreqp->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
    sreqp->dev.iov[0].MPID_IOV_BUF = (char *)&sreqp->dev.pending_pkt;
    sreqp->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    if(data_sz > 0){
        sreqp->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
        sreqp->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreqp->dev.iov_count = 2;
    }
    else{
        sreqp->dev.iov_count = 1;
    }

    is_send_posted = 0;
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        MPID_Nem_nd_conn_hnd_t conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
        /* Try sending data - if no credits queue the remaining data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connected - trying to send data");
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){

            if(!MPID_NEM_ND_IS_BLOCKING_REQ(sreqp)){
                mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            else{
                mpi_errno = MPID_Nem_nd_post_sendbv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }

            /* Now queue the request in posted queue */
            is_send_posted = 1;
        }
    }
    else{
        /* VC is not connected */
        if(MPID_NEM_ND_VC_IS_CONNECTING(vc)){
            /* Already connecting - just queue req in pending queue */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connecting - queueing data");
        }
        else{
            /* Start connecting and queue req in pending queue */
            if(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) != MPID_NEM_ND_VC_STATE_DISCONNECTED){
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "vc(%p:%d), conn(%p:%d",
                    vc, MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc), 
                    MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc),
                    (MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc))->state));
            }
            MPIU_Assert(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_DISCONNECTED);
            mpi_errno = MPID_Nem_nd_conn_est(vc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted a connect - queueing data");
        }
    }

    if(is_send_posted){
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(vc, sreqp);
    }
    else{
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(vc, sreqp);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SEND_CONTIG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_send_noncontig
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_send_noncontig(MPIDI_VC_t *vc, MPID_Request *sreqp, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int is_send_posted = 0;
    MPID_IOV *iov;
    int iov_cnt = 0, i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);
    MPIU_Assert((hdr_sz > 0) && (hdr_sz <= sizeof(MPIDI_CH3_Pkt_t)));

    iov = &(sreqp->dev.iov[0]);
    /* Reserve 1st IOV for header */
    iov_cnt = MPID_IOV_LIMIT - 1;

    /* On return iov_cnt refers to the number of IOVs loaded */
    mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreqp, &(iov[1]), &iov_cnt);
    MPIU_ERR_CHKANDJUMP(mpi_errno, mpi_errno, MPI_ERR_OTHER, "**ch3|loadsendiov");

    sreqp->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)header;
    iov[0].MPID_IOV_BUF = (char *)&sreqp->dev.pending_pkt;
    iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    iov_cnt += 1;

    /* FIXME: Update the req dev iov fields only for unposted sends
     */
    sreqp->ch.vc = vc;
    sreqp->dev.iov_offset = 0;
    sreqp->dev.iov_count = iov_cnt;

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "isend_noncontig_msg (hdr_sz=" MPIDI_MSG_SZ_FMT ")", hdr_sz));
    for(i=1; i<iov_cnt; i++){
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "isend_noncontig_msg (iov[%d] = %p, size =%u)", i, iov[i].MPID_IOV_BUF, iov[i].MPID_IOV_LEN));
    }
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        MPID_Nem_nd_conn_hnd_t conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
        /* Try sending data - if no credits queue the remaining data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connected - trying to send data");
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            if(!MPID_NEM_ND_IS_BLOCKING_REQ(sreqp)){
                mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }
            else{
                mpi_errno = MPID_Nem_nd_post_sendbv(conn_hnd, sreqp);
                if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            }

            /* Now queue the request in posted queue */
            is_send_posted = 1;
        }
    }
    else{
        /* VC is not connected */
        is_send_posted = 0;
        if(MPID_NEM_ND_VC_IS_CONNECTING(vc)){
            /* Already connecting - just queue req in pending queue */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connecting - queueing data");
        }
        else{
            /* Start connecting and queue req in pending queue */
            if(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) != MPID_NEM_ND_VC_STATE_DISCONNECTED){
                MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "vc(%p:%d), conn(%p:%d",
                    vc, MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc), 
                    MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc),
                    (MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc))->state));
            }
            MPIU_Assert(MPID_NEM_ND_VCCH_NETMOD_STATE_GET(vc) == MPID_NEM_ND_VC_STATE_DISCONNECTED);
            mpi_errno = MPID_Nem_nd_conn_est(vc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted a connect - queueing data");
        }
    }

    if(is_send_posted){
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(vc, sreqp);
    }
    else{
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(vc, sreqp);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}