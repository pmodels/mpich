/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidi_ch3_impl.h"

static MPIR_Request *create_request(void *hdr, intptr_t hdr_sz, size_t nb)
{
    MPIR_Request *sreq;

    MPIR_FUNC_ENTER;

    sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
    /* --BEGIN ERROR HANDLING-- */
    if (sreq == NULL)
        return NULL;
    /* --END ERROR HANDLING-- */
    MPIR_Object_set_ref(sreq, 2);
    sreq->kind = MPIR_REQUEST_KIND__SEND;
    MPIR_Assert(hdr_sz == sizeof(MPIDI_CH3_Pkt_t));
    sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) hdr;
    sreq->dev.iov[0].iov_base = (void *) ((char *) &sreq->dev.pending_pkt + nb);
    sreq->dev.iov[0].iov_len = hdr_sz - nb;
    sreq->dev.iov_count = 1;
    sreq->dev.OnDataAvail = 0;

    MPIR_FUNC_EXIT;
    return sreq;
}

/*
 * MPIDI_CH3_iStartMsg() attempts to send the message immediately.  If the
 * entire message is successfully sent, then NULL is
 * returned.  Otherwise a request is allocated, the header is copied into the
 * request, and a pointer to the request is returned.
 * An error condition also results in a request be allocated and the error
 * being returned in the status field of the request.
 */
int MPIDI_CH3_iStartMsg(MPIDI_VC_t * vc, void *hdr, intptr_t hdr_sz, MPIR_Request ** sreq_ptr)
{
    MPIR_Request *sreq = NULL;
    MPIDI_CH3I_VC *vcch = &vc->ch;
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    MPIR_Assert(hdr_sz <= sizeof(MPIDI_CH3_Pkt_t));

    /* The SOCK channel uses a fixed length header, the size of which is the
     * maximum of all possible packet headers */
    hdr_sz = sizeof(MPIDI_CH3_Pkt_t);
    MPL_DBG_STMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE, MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) hdr));

    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) { /* MT */
        /* Connection already formed.  If send queue is empty attempt to send
         * data, queuing any unsent data. */
        if (MPIDI_CH3I_SendQ_empty(vcch)) {     /* MT */
            size_t nb;
            int rc;

            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "send queue empty, attempting to write");

            MPL_DBG_PKT(vcch->conn, hdr, "istartmsg");
            /* MT: need some signalling to lock down our right to use the
             * channel, thus insuring that the progress engine does
             * not also try to write */
            rc = MPIDI_CH3I_Sock_write(vcch->sock, hdr, hdr_sz, &nb);
            if (rc == MPI_SUCCESS) {
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                              "wrote %ld bytes", (unsigned long) nb);

                if (nb == hdr_sz) {
                    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                  "entire write complete, %" PRIdPTR " bytes", nb);
                    /* done.  get us out of here as quickly as possible. */
                } else {
                    MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                  "partial write of %" PRIdPTR " bytes, request enqueued at head",
                                  nb);
                    sreq = create_request(hdr, hdr_sz, nb);
                    if (!sreq) {
                        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
                    }

                    MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
                    MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                    (MPL_DBG_FDEST, "posting write, vc=0x%p, sreq=0x%08x", vc,
                                     sreq->handle));
                    vcch->conn->send_active = sreq;
                    mpi_errno =
                        MPIDI_CH3I_Sock_post_write(vcch->conn->sock, sreq->dev.iov[0].iov_base,
                                                   sreq->dev.iov[0].iov_len,
                                                   sreq->dev.iov[0].iov_len, NULL);
                    /* --BEGIN ERROR HANDLING-- */
                    if (mpi_errno != MPI_SUCCESS) {
                        mpi_errno =
                            MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, __func__, __LINE__,
                                                 MPI_ERR_OTHER, "**ch3|sock|postwrite",
                                                 "ch3|sock|postwrite %p %p %p", sreq, vcch->conn,
                                                 vc);
                        goto fn_fail;
                    }
                    /* --END ERROR HANDLING-- */
                }
            }
            /* --BEGIN ERROR HANDLING-- */
            else {
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, TYPICAL,
                              "ERROR - MPIDI_CH3I_Sock_write failed, rc=%d", rc);
                sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
                if (!sreq) {
                    MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
                }
                sreq->kind = MPIR_REQUEST_KIND__SEND;
                MPIR_cc_set(&(sreq->cc), 0);
                sreq->status.MPI_ERROR = MPIR_Err_create_code(rc,
                                                              MPIR_ERR_RECOVERABLE, __func__,
                                                              __LINE__, MPI_ERR_INTERN,
                                                              "**ch3|sock|writefailed",
                                                              "**ch3|sock|writefailed %d", rc);
                /* Make sure that the caller sees this error */
                mpi_errno = sreq->status.MPI_ERROR;
            }
            /* --END ERROR HANDLING-- */
        } else {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "send in progress, request enqueued");
            sreq = create_request(hdr, hdr_sz, 0);
            if (!sreq) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
            }
            MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
        }
    } else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING) { /* MT */
        MPL_DBG_VCUSE(vc, "connecteding. enqueuing request");

        /* queue the data so it can be sent after the connection is formed */
        sreq = create_request(hdr, hdr_sz, 0);
        if (!sreq) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    } else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED) {        /* MT */
        MPL_DBG_VCUSE(vc, "unconnected.  posting connect and enqueuing request");

        /* queue the data so it can be sent after the connection is formed */
        sreq = create_request(hdr, hdr_sz, 0);
        if (!sreq) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);

        /* Form a new connection */
        MPIDI_CH3I_VC_post_connect(vc);
    } else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED) {
        /* Unable to send data at the moment, so queue it for later */
        MPL_DBG_VCUSE(vc, "forming connection, request enqueued");
        sreq = create_request(hdr, hdr_sz, 0);
        if (!sreq) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
        /* Connection failed, so allocate a request and return an error. */
        MPL_DBG_VCUSE(vc, "ERROR - connection failed");
        sreq = MPIR_Request_create(MPIR_REQUEST_KIND__UNDEFINED);
        if (!sreq) {
            MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        sreq->kind = MPIR_REQUEST_KIND__SEND;
        MPIR_cc_set(&sreq->cc, 0);

        sreq->status.MPI_ERROR = MPIR_Err_create_code(MPI_SUCCESS,
                                                      MPIR_ERR_RECOVERABLE, __func__, __LINE__,
                                                      MPI_ERR_INTERN, "**ch3|sock|connectionfailed",
                                                      0);
        /* Make sure that the caller sees this error */
        mpi_errno = sreq->status.MPI_ERROR;
    }
    /* --END ERROR HANDLING-- */

  fn_fail:
    *sreq_ptr = sreq;
    MPIR_FUNC_EXIT;
    return mpi_errno;
}
