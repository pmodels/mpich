/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "scif_impl.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_recv_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_scif_recv_handler(scifconn_t * const sc)
{
    const int sc_fd = sc->fd;
    shmchan_t *const sc_ch = &sc->crecv;
    MPIDI_VC_t *const sc_vc = sc->vc;
    int mpi_errno = MPI_SUCCESS;
    ssize_t bytes_recvd;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_RECV_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_RECV_HANDLER);

    if (sc_vc->ch.recv_active == NULL) {
        /* receive a new message */
        bytes_recvd =
            MPID_nem_scif_read(sc_fd, sc_ch, MPID_nem_scif_recv_buf,
                               MPID_NEM_SCIF_RECV_MAX_PKT_LEN);
        MPIU_ERR_CHKANDJUMP1(bytes_recvd <= 0, mpi_errno, MPI_ERR_OTHER,
                             "**scif_read", "**scif_scif_read %s", MPIU_Strerror(errno));

        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE,
                         (MPIU_DBG_FDEST,
                          "New recv " MPIDI_MSG_SZ_FMT
                          " (fd=%d, vc=%p, sc=%p)", bytes_recvd, sc_fd, sc_vc, sc));

        mpi_errno = MPID_nem_handle_pkt(sc_vc, MPID_nem_scif_recv_buf, bytes_recvd);
        if (mpi_errno)
            MPIU_ERR_POP_LABEL(mpi_errno, fn_noncomm_fail);
    }
    else {
        /* there is a pending receive, receive it directly into the
         * user buffer */
        MPIDI_CH3I_VC *const sc_vc_ch = &sc_vc->ch;
        MPID_Request *const rreq = sc_vc_ch->recv_active;
        MPID_IOV *iov = &rreq->dev.iov[rreq->dev.iov_offset];
        int (*reqFn) (MPIDI_VC_t *, MPID_Request *, int *);

        MPIU_Assert(rreq->dev.iov_count > 0);
        MPIU_Assert(rreq->dev.iov_offset >= 0);
        MPIU_Assert(rreq->dev.iov_count + rreq->dev.iov_offset <= MPID_IOV_LIMIT);

        bytes_recvd = MPID_nem_scif_readv(sc_fd, sc_ch, iov, rreq->dev.iov_count);
        MPIU_ERR_CHKANDJUMP1(bytes_recvd <= 0, mpi_errno, MPI_ERR_OTHER,
                             "**scif_readv", "**scif_scif_readv %s", MPIU_Strerror(errno));

        MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "Cont recv %ld", bytes_recvd);

        /* update the iov */
        for (iov = &rreq->dev.iov[rreq->dev.iov_offset];
             iov < &rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count]; ++iov) {
            if (bytes_recvd < iov->MPID_IOV_LEN) {
                iov->MPID_IOV_BUF = (char *) iov->MPID_IOV_BUF + bytes_recvd;
                iov->MPID_IOV_LEN -= bytes_recvd;
                rreq->dev.iov_count =
                    &rreq->dev.iov[rreq->dev.iov_offset + rreq->dev.iov_count] - iov;
                rreq->dev.iov_offset = iov - rreq->dev.iov;
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "bytes_recvd = %ld", (long int) bytes_recvd);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov len = %ld", (long int) iov->MPID_IOV_LEN);
                MPIU_DBG_MSG_D(CH3_CHANNEL, VERBOSE, "iov_offset = %d", rreq->dev.iov_offset);
                goto fn_exit;
            }
            bytes_recvd -= iov->MPID_IOV_LEN;
        }

        /* the whole iov has been received */

        reqFn = rreq->dev.OnDataAvail;
        if (!reqFn) {
            MPIU_Assert(MPIDI_Request_get_type(rreq) != MPIDI_REQUEST_TYPE_GET_RESP);
            MPIDI_CH3U_Request_complete(rreq);
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
            sc_vc_ch->recv_active = NULL;
        }
        else {
            int complete = 0;

            mpi_errno = reqFn(sc_vc, rreq, &complete);
            if (mpi_errno)
                MPIU_ERR_POP_LABEL(mpi_errno, fn_noncomm_fail);

            if (complete) {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...complete");
                sc_vc_ch->recv_active = NULL;
            }
            else {
                MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, "...not complete");
            }
        }
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_RECV_HANDLER);
    return mpi_errno;
  fn_fail:     /* comm related failures jump here */
    {
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", sc_vc->pg_rank);
    }
  fn_noncomm_fail:     /* NON-comm related failures jump here */
    goto fn_exit;

}

#undef FUNCNAME
#define FUNCNAME state_commrdy_handler
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int state_commrdy_handler(int is_readable, int is_writeable, scifconn_t * const sc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *sc_vc = sc->vc;
    MPID_nem_scif_vc_area *sc_vc_scif = VC_SCIF(sc_vc);
    MPIDI_STATE_DECL(MPID_STATE_STATE_COMMRDY_HANDLER);

    MPIDI_FUNC_ENTER(MPID_STATE_STATE_COMMRDY_HANDLER);

    if (is_readable) {
        mpi_errno = MPID_nem_scif_recv_handler(sc);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    if (is_writeable && !MPIDI_CH3I_Sendq_empty(sc_vc_scif->send_queue)) {
        mpi_errno = MPID_nem_scif_send_queued(sc_vc, &sc_vc_scif->send_queue);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        /* check to see if this VC is waiting for outstanding sends to
         * finish in order to terminate */
        if (MPIDI_CH3I_Sendq_empty(sc_vc_scif->send_queue) && sc_vc_scif->terminate == 1) {
            /* The sendq is empty, so we can immediately terminate
             * the connection. */
            mpi_errno = MPID_nem_scif_vc_terminated(sc_vc);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }
    }
  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_STATE_COMMRDY_HANDLER);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPID_nem_scif_connpoll(int in_blocking_poll)
{
    int mpi_errno = MPI_SUCCESS, i;
    static int npolls = 0;

    /* poll the real scif channels */
    for (i = 0; i < MPID_nem_scif_nranks; ++i) {
        int is_readable;
        int is_writeable;

        if (i == MPID_nem_scif_myrank)
            continue;
        scifconn_t *it_sc = &MPID_nem_scif_conns[i];
        if (it_sc->fd == -1)
            continue;   /* no connection */
        if (MPID_nem_scif_chk_dma_unreg(&it_sc->csend)) {
            MPID_nem_scif_unregmem(it_sc->fd, &it_sc->csend);
        }
        is_readable = MPID_nem_scif_poll_recv(&it_sc->crecv);
        is_writeable = MPID_nem_scif_poll_send(it_sc->fd, &it_sc->csend);
        MPIU_ERR_CHKANDJUMP1(is_writeable == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_poll_send", "**scif_poll_send %s", MPIU_Strerror(errno));
        mpi_errno = state_commrdy_handler(is_readable, is_writeable, it_sc);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        if (npolls >= NPOLLS) {
            struct pollfd fds;
            fds.fd = it_sc->fd;
            fds.events = POLLIN;
            fds.revents = 0;
            MPIU_ERR_CHKANDJUMP1(poll(&fds, 1, 0) < 0, mpi_errno, MPI_ERR_OTHER,
                                 "**poll", "**poll %s", MPIU_Strerror(errno));
            MPIU_ERR_CHKANDJUMP(fds.revents & POLLERR, mpi_errno, MPI_ERR_OTHER, "**poll");
        }
    }
    if (npolls++ >= NPOLLS)
        npolls = 0;

  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}
