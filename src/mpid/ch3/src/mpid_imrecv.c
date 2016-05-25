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
                MPIR_Request *message, MPIR_Request **rreqp)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *rreq;
    MPIR_Comm *comm;
    MPIDI_VC_t *vc = NULL;

    /* message==NULL is equivalent to MPI_MESSAGE_NO_PROC being passed at the
     * upper level */
    if (message == NULL)
    {
        MPIDI_Request_create_null_rreq(rreq, mpi_errno, goto fn_fail);
        *rreqp = rreq;
        goto fn_exit;
    }

    MPIR_Assert(message != NULL);
    MPIR_Assert(message->kind == MPIR_REQUEST_KIND__MPROBE);

    /* promote the request object to be a "real" recv request */
    message->kind = MPIR_REQUEST_KIND__RECV;

    *rreqp = rreq = message;

    comm = rreq->comm;

    /* the following code was adapted from FDU_or_AEP and MPID_Irecv */
    /* comm was already added at FDU (mprobe) time */
    rreq->dev.user_buf = buf;
    rreq->dev.user_count = count;
    rreq->dev.datatype = datatype;

#ifdef ENABLE_COMM_OVERRIDES
    MPIDI_Comm_get_vc(comm, rreq->status.MPI_SOURCE, &vc);
    if (vc->comm_ops && vc->comm_ops->imrecv) {
        MPID_THREAD_CS_ENTER(POBJ, vc->pobj_mutex);
        vc->comm_ops->imrecv(vc, rreq);
        MPID_THREAD_CS_EXIT(POBJ, vc->pobj_mutex);
        goto fn_exit;
    }
#endif

    if (MPIDI_Request_get_msg_type(rreq) == MPIDI_REQUEST_EAGER_MSG)
    {
        int recv_pending;

        /* This is an eager message */
        MPL_DBG_MSG(MPIDI_CH3_DBG_OTHER,VERBOSE,"eager message in the request");

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

        if (MPIR_Request_is_complete(rreq)) {
            /* is it ever possible to have (cc==0 && recv_pending>0) ? */
            MPIR_Assert(!recv_pending);

            /* All of the data has arrived, we need to copy the data and
               then free the buffer. */
            if (rreq->dev.recv_data_sz > 0)
            {
                MPIDI_CH3U_Request_unpack_uebuf(rreq);
                MPL_free(rreq->dev.tmpbuf);
            }

            mpi_errno = rreq->status.MPI_ERROR;
            goto fn_exit;
        }
        else
        {
            /* there should never be outstanding completion events for an unexpected
             * recv without also having a "pending recv" */
            MPIR_Assert(recv_pending);
            /* The data is still being transfered across the net.  We'll
               leave it to the progress engine to handle once the
               entire message has arrived. */
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN)
            {
                MPIR_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
                MPIR_Datatype_add_ref(rreq->dev.datatype_ptr);
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
            MPIR_Datatype_get_ptr(datatype, rreq->dev.datatype_ptr);
            MPIR_Datatype_add_ref(rreq->dev.datatype_ptr);
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
        MPIR_Request_free(rreq);
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

