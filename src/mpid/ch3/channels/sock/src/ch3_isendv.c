/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static void update_request(MPIR_Request * sreq, MPL_IOV * iov, int iov_count,
                           int iov_offset, size_t nb)
{
    int i;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_UPDATE_REQUEST);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_UPDATE_REQUEST);

    for (i = 0; i < iov_count; i++) {
        sreq->dev.iov[i] = iov[i];
    }
    if (iov_offset == 0) {
        MPIR_Assert(iov[0].MPL_IOV_LEN == sizeof(MPIDI_CH3_Pkt_t));
        sreq->dev.pending_pkt = *(MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF;
        sreq->dev.iov[0].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) & sreq->dev.pending_pkt;
    }
    sreq->dev.iov[iov_offset].MPL_IOV_BUF =
        (MPL_IOV_BUF_CAST) ((char *) sreq->dev.iov[iov_offset].MPL_IOV_BUF + nb);
    sreq->dev.iov[iov_offset].MPL_IOV_LEN -= nb;
    sreq->dev.iov_count = iov_count;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_UPDATE_REQUEST);
}

int MPIDI_CH3_iSendv(MPIDI_VC_t * vc, MPIR_Request * sreq, MPL_IOV * iov, int n_iov)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vcch = &vc->ch;
    int (*reqFn) (MPIDI_VC_t *, MPIR_Request *, int *);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_ISENDV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_ISENDV);

    if (sreq->dev.ext_hdr_sz > 0) {
        int i;
        for (i = n_iov - 1; i >= 1; i--) {
            iov[i + 1].MPL_IOV_BUF = iov[i].MPL_IOV_BUF;
            iov[i + 1].MPL_IOV_LEN = iov[i].MPL_IOV_LEN;
        }
        iov[1].MPL_IOV_BUF = (MPL_IOV_BUF_CAST) sreq->dev.ext_hdr_ptr;
        iov[1].MPL_IOV_LEN = sreq->dev.ext_hdr_sz;
        n_iov++;
    }

    MPIR_Assert(n_iov <= MPL_IOV_LIMIT);
    MPIR_Assert(iov[0].MPL_IOV_LEN <= sizeof(MPIDI_CH3_Pkt_t));

    /* The sock channel uses a fixed length header, the size of which is the
     * maximum of all possible packet headers */
    iov[0].MPL_IOV_LEN = sizeof(MPIDI_CH3_Pkt_t);
    MPL_DBG_STMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                 MPIDI_DBG_Print_packet((MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF));

    if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTED) { /* MT */
        /* Connection already formed.  If send queue is empty attempt to send
         * data, queuing any unsent data. */
        if (MPIDI_CH3I_SendQ_empty(vcch)) {     /* MT */
            size_t nb;
            int rc;

            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "send queue empty, attempting to write");

            MPL_DBG_PKT(vcch->conn, (MPIDI_CH3_Pkt_t *) iov[0].MPL_IOV_BUF, "isendv");
            /* MT - need some signalling to lock down our right to use the
             * channel, thus insuring that the progress engine does
             * also try to write */

            /* FIXME: the current code only agressively writes the first IOV.
             * Eventually it should be changed to agressively write
             * as much as possible.  Ideally, the code would be shared between
             * the send routines and the progress engine. */
            rc = MPIDI_CH3I_Sock_writev(vcch->sock, iov, n_iov, &nb);
            if (rc == MPI_SUCCESS) {
                int offset = 0;

                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                              "wrote %ld bytes", (unsigned long) nb);

                while (offset < n_iov) {
                    if (iov[offset].MPL_IOV_LEN <= nb) {
                        nb -= iov[offset].MPL_IOV_LEN;
                        offset++;
                    } else {
                        MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                    "partial write, request enqueued at head");
                        update_request(sreq, iov, n_iov, offset, nb);
                        MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
                        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                        (MPL_DBG_FDEST, "posting writev, vc=0x%p, sreq=0x%08x", vc,
                                         sreq->handle));
                        vcch->conn->send_active = sreq;
                        mpi_errno = MPIDI_CH3I_Sock_post_writev(vcch->conn->sock,
                                                                sreq->dev.iov + offset,
                                                                sreq->dev.iov_count - offset, NULL);
                        /* --BEGIN ERROR HANDLING-- */
                        if (mpi_errno != MPI_SUCCESS) {
                            mpi_errno =
                                MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, __func__, __LINE__,
                                                     MPI_ERR_OTHER, "**ch3|sock|postwrite",
                                                     "ch3|sock|postwrite %p %p %p", sreq,
                                                     vcch->conn, vc);
                        }
                        /* --END ERROR HANDLING-- */

                        break;
                    }

                }
                if (offset == n_iov) {
                    MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                "write complete, calling OnDataAvail fcn");
                    reqFn = sreq->dev.OnDataAvail;
                    if (!reqFn) {
                        MPIR_Assert(MPIDI_Request_get_type(sreq) != MPIDI_REQUEST_TYPE_GET_RESP);
                        mpi_errno = MPID_Request_complete(sreq);
                        if (mpi_errno != MPI_SUCCESS) {
                            MPIR_ERR_POP(mpi_errno);
                        }
                    } else {
                        int complete;
                        mpi_errno = reqFn(vc, sreq, &complete);
                        if (mpi_errno)
                            MPIR_ERR_POP(mpi_errno);
                        if (!complete) {
                            MPIDI_CH3I_SendQ_enqueue_head(vcch, sreq);
                            MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_CHANNEL, VERBOSE,
                                            (MPL_DBG_FDEST, "posting writev, vc=0x%p, sreq=0x%08x",
                                             vc, sreq->handle));
                            vcch->conn->send_active = sreq;
                            mpi_errno = MPIDI_CH3I_Sock_post_writev(vcch->conn->sock, sreq->dev.iov,
                                                                    sreq->dev.iov_count, NULL);
                            /* --BEGIN ERROR HANDLING-- */
                            if (mpi_errno != MPI_SUCCESS) {
                                mpi_errno =
                                    MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, __func__,
                                                         __LINE__, MPI_ERR_OTHER,
                                                         "**ch3|sock|postwrite",
                                                         "ch3|sock|postwrite %p %p %p", sreq,
                                                         vcch->conn, vc);
                            }
                            /* --END ERROR HANDLING-- */
                        }
                    }
                }
            }
            /* --BEGIN ERROR HANDLING-- */
            else if (MPIR_ERR_GET_CLASS(rc) == MPIDI_CH3I_SOCK_ERR_NOMEM) {
                MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, TYPICAL,
                            "MPIDI_CH3I_Sock_writev failed, out of memory");
                sreq->status.MPI_ERROR = MPIR_ERR_MEMALLOCFAILED;
            } else {
                MPL_DBG_MSG_D(MPIDI_CH3_DBG_CHANNEL, TYPICAL,
                              "MPIDI_CH3I_Sock_writev failed, rc=%d", rc);
                /* Connection just failed.  Mark the request complete and
                 * return an error. */
                MPL_DBG_VCCHSTATECHANGE(vc, VC_STATE_FAILED);
                /* FIXME: Shouldn't the vc->state also change? */

                vcch->state = MPIDI_CH3I_VC_STATE_FAILED;
                sreq->status.MPI_ERROR = MPIR_Err_create_code(rc,
                                                              MPIR_ERR_RECOVERABLE, __func__,
                                                              __LINE__, MPI_ERR_INTERN,
                                                              "**ch3|sock|writefailed",
                                                              "**ch3|sock|writefailed %d", rc);
                /* MT - CH3U_Request_complete performs write barrier */
                MPID_Request_complete(sreq);
                /* Return error to calling routine */
                mpi_errno = sreq->status.MPI_ERROR;
            }
            /* --END ERROR HANDLING-- */
        } else {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, "send queue not empty, enqueuing");
            update_request(sreq, iov, n_iov, 0, 0);
            MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
        }
    } else if (vcch->state == MPIDI_CH3I_VC_STATE_CONNECTING) {
        /* queuing the data so it can be sent later. */
        MPL_DBG_VCUSE(vc, "connecting.  Enqueuing request");
        update_request(sreq, iov, n_iov, 0, 0);
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    } else if (vcch->state == MPIDI_CH3I_VC_STATE_UNCONNECTED) {
        /* Form a new connection, queuing the data so it can be sent later. */
        MPL_DBG_VCUSE(vc, "unconnected.  Enqueuing request");
        update_request(sreq, iov, n_iov, 0, 0);
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
        mpi_errno = MPIDI_CH3I_VC_post_connect(vc);
        if (mpi_errno) {
            MPIR_ERR_POP(mpi_errno);
        }
    } else if (vcch->state != MPIDI_CH3I_VC_STATE_FAILED) {
        /* Unable to send data at the moment, so queue it for later */
        MPL_DBG_VCUSE(vc, "still connecting.  enqueuing request");
        update_request(sreq, iov, n_iov, 0, 0);
        MPIDI_CH3I_SendQ_enqueue(vcch, sreq);
    }
    /* --BEGIN ERROR HANDLING-- */
    else {
        MPL_DBG_VCUSE(vc, "connection failed");
        /* Connection failed.  Mark the request complete and return an error. */
        /* TODO: Create an appropriate error message */
        sreq->status.MPI_ERROR = MPI_ERR_INTERN;
        /* MT - CH3U_Request_complete performs write barrier */
        MPID_Request_complete(sreq);
    }
    /* --END ERROR HANDLING-- */

  fn_fail:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_ISENDV);
    return mpi_errno;
}
