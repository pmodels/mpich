/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Imrecv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Imrecv(void *buf, int count, MPI_Datatype datatype,
                MPID_Request *message, MPID_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Request *rreq;
    MPID_Comm *comm;
    MPIDI_VC_t *vc = NULL;

    /* message==NULL is equivalent to MPI_MESSAGE_NO_PROC being passed at the
     * upper level */
    if (message == NULL)
    {
        MPIDI_Request_create_null_rreq(rreq, mpi_errno, goto fn_fail);
        *rreqp = rreq;
        goto fn_exit;
    }

    MPIU_Assert(message != NULL);
    MPIU_Assert(message->kind == MPID_REQUEST_MPROBE);

    /* promote the request object to be a "real" recv request */
    message->kind = MPID_REQUEST_RECV;

    *rreqp = rreq = message;

    comm = rreq->comm;

    /* the following code was adapted from FDU_or_AEP and MPID_Irecv */
    /* comm was already added at FDU (mprobe) time */
    rreq->dev.user_buf = buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;

    if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
    {
        int recv_pending;

        /* This is an eager message */
        MPIU_DBG_MSG(CH3_OTHER,VERBOSE,"eager message in the request");

        /* If this is an eager synchronous message, then we need to send an
           acknowledgement back to the sender. */
        if (MPIDI_Request_get_sync_send_flag(rreq))
        {
            MPIDI_Comm_get_vc_set_active(comm, rreq->dev.match.parts.rank, &vc);
            mpi_errno = MPIDI_CH3_EagerSyncAck(vc, rreq);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

        /* the request was found in the unexpected queue, so it has a
           recv_pending_count of at least 1 */
        MPIDI_Request_decr_pending(rreq);
        MPIDI_Request_check_pending(rreq, &recv_pending);

        if (MPID_Request_is_complete(rreq)) {
            /* is it ever possible to have (cc==0 && recv_pending>0) ? */
            MPIU_Assert(!recv_pending);

            /* All of the data has arrived, we need to copy the data and
               then free the buffer. */
            if (rreq->dev.recv_data_sz > 0)
            {
                MPIDI_CH3U_Request_unpack_uebuf(rreq);
                MPIU_Free(rreq->dev.tmpbuf);
            }

            mpi_errno = rreq->status.MPI_ERROR;
            goto fn_exit;
        }
        else
        {
            /* there should never be outstanding completion events for an unexpected
             * recv without also having a "pending recv" */
            MPIU_Assert(recv_pending);
            /* The data is still being transfered across the net.  We'll
               leave it to the progress engine to handle once the
               entire message has arrived. */
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
            {
                MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
                MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
            }

        }
    }
    else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_RNDV_MSG)
    {
        MPIDI_Comm_get_vc_set_active(comm, rreq->dev.match.parts.rank, &vc);

        mpi_errno = vc->rndvRecv_fn(vc, rreq);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
        {
            MPID_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
            MPID_Datatype_add_ref(rreq->dev.datatype_ptr);
        }
    }
    else if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_SELF_MSG)
    {
        mpi_errno = MPIDI_CH3_RecvFromSelf(rreq, buf, count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else
    {
        /* --BEGIN ERROR HANDLING-- */
#ifdef HAVE_ERROR_CHECKING
        int msg_type = MPIDI_Request_get_msg_type(rreq);
#endif
        MPID_Request_release(rreq);
        rreq = NULL;
        MPIR_ERR_SETANDJUMP1(mpi_errno,MPI_ERR_INTERN, "**ch3|badmsgtype",
                             "**ch3|badmsgtype %d", msg_type);
        /* --END ERROR HANDLING-- */
    }

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

