/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "nd_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SEND);

    /* We no longer use the send() function, we use iStartContigMsg()/
     * SendContig()/SendNonContig() instead
     */
    MPIU_Assert(0);
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SEND);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_Nem_nd_istart_contig_msg
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Nem_nd_istart_contig_msg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                    MPID_Request **sreq_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * sreq = NULL;
    int is_send_posted = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_ISTART_CONTIG_MSG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_ISTART_CONTIG_MSG);
    MPIU_Assert((hdr_sz > 0) && (hdr_sz <= sizeof(MPIDI_CH3_Pkt_t)));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "istart_contig_msg (hdr_sz=%d,data_sz=%d)", hdr_sz, data_sz));
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        MPID_Nem_nd_conn_hnd_t conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
        /* Try sending data - if no credits queue the remaining data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connected - trying to send data");
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            /* We have send credits */
            MPID_IOV iov[2];
            int iov_cnt;
            /* Post a send for data & queue request */
            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )hdr;
            MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t ));
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            if(data_sz > 0){
                iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
                iov[1].MPID_IOV_LEN = data_sz;
                iov_cnt = 2;
            }
            else{
                iov_cnt = 1;
            }

            mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, iov, iov_cnt);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            /* Now queue the request in posted queue */
            is_send_posted = 1;
        }
    }
    else{
        /* VC is not connected */
        is_send_posted = 0;
        if(MPID_NEM_ND_CONN_IS_CONNECTING(MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc))){
            /* Already connecting - just queue req in pending queue */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connecting - queueing data");
        }
        else{
            /* Start connecting and queue req in pending queue */
            mpi_errno = MPID_Nem_nd_conn_est(vc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted a connect - queueing data");
        }
    }

    /* Create a request and queue it */
    sreq = MPID_Request_create();
    MPIU_Assert(sreq != NULL);
    MPIU_Object_set_ref(sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;

    sreq->dev.OnDataAvail = NULL;
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;

    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
    sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    if(data_sz > 0){
        sreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
        sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
    }
    else{
        sreq->dev.iov_count = 1;
    }
    if(is_send_posted){
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(vc, sreq);
    }
    else{
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(vc, sreq);
    }

    *sreq_ptr = sreq;

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
int MPID_Nem_nd_send_contig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                                void *data, MPIDI_msg_sz_t data_sz)
{
    int mpi_errno = MPI_SUCCESS;
    int is_send_posted = 0;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SEND_CONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SEND_CONTIG);
    MPIU_Assert((hdr_sz > 0) && (hdr_sz <= sizeof(MPIDI_CH3_Pkt_t)));

    MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST, "isend_contig_msg (hdr_sz=%d,data_sz=%d)", hdr_sz, data_sz));
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        MPID_Nem_nd_conn_hnd_t conn_hnd = MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc);
        /* Try sending data - if no credits queue the remaining data */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connected - trying to send data");
        if(MPID_NEM_ND_CONN_HAS_SCREDITS(conn_hnd)){
            /* We have send credits */
            MPID_IOV iov[2];
            int iov_cnt;
            /* Post a send for data & queue request */
            iov[0].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )hdr;
            MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_PktGeneric_t ));
            iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);
            if(data_sz > 0){
                iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
                iov[1].MPID_IOV_LEN = data_sz;
                iov_cnt = 2;
            }
            else{
                iov_cnt = 1;
            }

            mpi_errno = MPID_Nem_nd_post_sendv(conn_hnd, iov, iov_cnt);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);

            /* Now queue the request in posted queue */
            is_send_posted = 1;
        }
    }
    else{
        /* VC is not connected */
        is_send_posted = 0;
        if(MPID_NEM_ND_CONN_IS_CONNECTING(MPID_NEM_ND_VCCH_NETMOD_CONN_HND_GET(vc))){
            /* Already connecting - just queue req in pending queue */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "vc connecting - queueing data");
        }
        else{
            /* Start connecting and queue req in pending queue */
            mpi_errno = MPID_Nem_nd_conn_est(vc);
            if(mpi_errno != MPI_SUCCESS) MPIU_ERR_POP(mpi_errno);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "Posted a connect - queueing data");
        }
    }

    /* FIXME: Update the req dev iov fields only for unposted sends
     */
    /* Create a request and queue it */
    sreq->ch.vc = vc;
    sreq->dev.iov_offset = 0;
    sreq->dev.pending_pkt = *(MPIDI_CH3_PktGeneric_t *)hdr;
    sreq->dev.iov[0].MPID_IOV_BUF = (char *)&sreq->dev.pending_pkt;
    sreq->dev.iov[0].MPID_IOV_LEN = sizeof(MPIDI_CH3_PktGeneric_t);

    if(data_sz > 0){
        sreq->dev.iov[1].MPID_IOV_BUF = (MPID_IOV_BUF_CAST )data;
        sreq->dev.iov[1].MPID_IOV_LEN = data_sz;
        sreq->dev.iov_count = 2;
    }
    else{
        sreq->dev.iov_count = 1;
    }
    if(is_send_posted){
        MPID_NEM_ND_VCCH_NETMOD_POSTED_SENDQ_ENQUEUE(vc, sreq);
    }
    else{
        MPID_NEM_ND_VCCH_NETMOD_PENDING_SENDQ_ENQUEUE(vc, sreq);
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
int MPID_Nem_nd_send_noncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);
    MPIU_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));
    /* FIXME: We have not implemented send for non contig msgs yet */
    MPIU_Assert(0);
    if(MPID_NEM_ND_VC_IS_CONNECTED(vc)){
        /* Try sending data - if no credits queue the remaining data */
    }
    else{
        /* Start connecting */
    }
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ND_SEND_NONCONTIG);
    return mpi_errno;
 fn_fail:
    MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "failed, mpi_errno = %d", mpi_errno);
    goto fn_exit;
}