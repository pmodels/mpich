/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2013 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "scif_impl.h"
#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

MPID_nem_netmod_funcs_t MPIDI_nem_scif_funcs = {
    MPID_nem_scif_init,
    MPID_nem_scif_finalize,
#ifdef ENABLE_CHECKPOINTING
    NULL,
    NULL,
    NULL,
#endif
    MPID_nem_scif_connpoll,
    MPID_nem_scif_get_business_card,
    MPID_nem_scif_connect_to_root,
    MPID_nem_scif_vc_init,
    MPID_nem_scif_vc_destroy,
    MPID_nem_scif_vc_terminate,
    NULL        /* anysource iprobe */
};

int MPID_nem_scif_nranks;
scifconn_t *MPID_nem_scif_conns;
char *MPID_nem_scif_recv_buf;
int MPID_nem_scif_myrank;

static int listen_fd;
static int listen_port;

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_post_init
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_scif_post_init(void)
{
    int mpi_errno = MPI_SUCCESS, pmi_errno;
    MPIDI_PG_t *my_pg = MPIDI_Process.my_pg;

    int my_rank = MPIDI_CH3I_my_rank;
    int i;
    MPIDI_VC_t *vc;
    scifconn_t *sc;
    MPIDI_CH3I_VC *vc_ch;
    MPID_nem_scif_vc_area *vc_scif;
    size_t s;
    int ret;
    off_t offset;
    int peer_rank;

    MPIDI_STATE_DECL(MPID_NEM_SCIF_POST_INIT);
    MPIDI_FUNC_ENTER(MPID_NEM_SCIF_POST_INIT);

    for (i = 0; i < MPID_nem_scif_nranks; i++) {

        vc = &my_pg->vct[i];
        vc_ch = &vc->ch;

        if (vc->pg_rank == MPID_nem_scif_myrank || vc_ch->is_local) {
            continue;
        }

        /* restore some value which might be rewrited during MPID_nem_vc_init() */

        vc->sendNoncontig_fn = MPID_nem_scif_SendNoncontig;
        vc_ch->iStartContigMsg = MPID_nem_scif_iStartContigMsg;
        vc_ch->iSendContig = MPID_nem_scif_iSendContig;

    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_SCIF_POST_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_init(MPIDI_PG_t * pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int pmi_errno;
    int ret;
    int i;
    MPIU_CHKPMEM_DECL(2);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_INIT);

    /* first make sure that our private fields in the vc fit into the
     * area provided  */
    MPIU_Assert(sizeof(MPID_nem_scif_vc_area) <= MPID_NEM_VC_NETMOD_AREA_LEN);

    MPID_nem_scif_nranks = pg_p->size;
    MPID_nem_scif_myrank = pg_rank;

    /* set up listener socket */
    {
        listen_fd = scif_open();
        MPIU_ERR_CHKANDJUMP1(listen_fd == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_open", "**scif_open %s", MPIU_Strerror(errno));

        listen_port = scif_bind(listen_fd, 0);
        MPIU_ERR_CHKANDJUMP1(listen_port == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_bind", "**scif_bind %s", MPIU_Strerror(errno));

        ret = scif_listen(listen_fd, MPID_nem_scif_nranks);
        MPIU_ERR_CHKANDJUMP1(ret == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_listen", "**scif_listen %s", MPIU_Strerror(errno));
    }

    /* create business card */
    mpi_errno = MPID_nem_scif_get_business_card(pg_rank, bc_val_p, val_max_sz_p);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    MPIU_CHKPMEM_MALLOC(MPID_nem_scif_conns, scifconn_t *,
                        MPID_nem_scif_nranks * sizeof(scifconn_t), mpi_errno, "connection table");
    memset(MPID_nem_scif_conns, 0, MPID_nem_scif_nranks * sizeof(scifconn_t));
    for (i = 0; i < MPID_nem_scif_nranks; ++i)
        MPID_nem_scif_conns[i].fd = -1;

    MPIU_CHKPMEM_MALLOC(MPID_nem_scif_recv_buf, char *,
                        MPID_NEM_SCIF_RECV_MAX_PKT_LEN, mpi_errno, "SCIF temporary buffer");
    MPIU_CHKPMEM_COMMIT();
    mpi_errno = MPID_nem_register_initcomp_cb(MPID_nem_scif_post_init);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);
    pmi_errno = PMI_Barrier();
    MPIU_ERR_CHKANDJUMP1(pmi_errno != PMI_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier",
                         "**pmi_barrier %d", pmi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_INIT);
    return mpi_errno;
  fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_get_business_card
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS;
    int str_errno = MPIU_STR_SUCCESS;
    int ret;
    char hostname[512];
    uint16_t self;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_GET_BUSINESS_CARD);

    hostname[sizeof(hostname) - 1] = 0;
    gethostname(hostname, sizeof(hostname) - 1);
    str_errno =
        MPIU_Str_add_string_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_HOST_DESCRIPTION_KEY, hostname);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    str_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_PORT_KEY, listen_port);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

    ret = scif_get_nodeIDs(NULL, 0, &self);
    MPIU_ERR_CHKANDJUMP2(ret == -1, mpi_errno, MPI_ERR_OTHER,
                         "**scif_get_nodeIDs", "**scif_get_nodeIDs %s %d",
                         MPIU_Strerror(errno), errno);
    str_errno = MPIU_Str_add_int_arg(bc_val_p, val_max_sz_p, MPIDI_CH3I_NODE_KEY, self);
    if (str_errno) {
        MPIU_ERR_CHKANDJUMP(str_errno == MPIU_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIU_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_GET_BUSINESS_CARD);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc)
{
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME scif_addr_from_bc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int scif_addr_from_bc(const char *business_card, uint16_t * addr, uint16_t * port)
{
    int mpi_errno = MPI_SUCCESS;
    int ret;
    int tmp;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_GET_ADDR_PORT_FROM_BC);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_GET_ADDR_PORT_FROM_BC);

    ret = MPIU_Str_get_int_arg(business_card, MPIDI_CH3I_PORT_KEY, &tmp);
    *port = (uint16_t) tmp;
    /* MPIU_STR_FAIL is not a valid MPI error code so we store the
     * result in ret instead of mpi_errno. */
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingport");

    ret = MPIU_Str_get_int_arg(business_card, MPIDI_CH3I_NODE_KEY, &tmp);
    *addr = (uint16_t) tmp;
    MPIU_ERR_CHKANDJUMP(ret != MPIU_STR_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**argstr_missingnode");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_GET_ADDR_PORT_FROM_BC);
    return mpi_errno;
  fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}

static int get_addr(MPIDI_VC_t * vc, struct scif_portID *addr)
{
    int mpi_errno = MPI_SUCCESS;
    char *bc;
    int pmi_errno;
    int val_max_sz;
    MPIU_CHKLMEM_DECL(1);

    /* Allocate space for the business card */
    pmi_errno = PMI_KVS_Get_value_length_max(&val_max_sz);
    MPIU_ERR_CHKANDJUMP1(pmi_errno, mpi_errno, MPI_ERR_OTHER, "**fail", "**fail %d", pmi_errno);
    MPIU_CHKLMEM_MALLOC(bc, char *, val_max_sz, mpi_errno, "bc");

    mpi_errno = vc->pg->getConnInfo(vc->pg_rank, bc, val_max_sz, vc->pg);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

    mpi_errno = scif_addr_from_bc(bc, &addr->node, &addr->port);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIU_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_vc_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch;
    MPID_nem_scif_vc_area *vc_scif;
    int ret;
    size_t s;
    scifconn_t *sc;
    off_t offset;
    int peer_rank;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_VC_INIT);

    /* do the connection */
    if (vc->pg_rank < MPID_nem_scif_myrank) {
        vc_ch = &vc->ch;
        vc_scif = VC_SCIF(vc);
        vc->sendNoncontig_fn = MPID_nem_scif_SendNoncontig;
        vc_ch->iStartContigMsg = MPID_nem_scif_iStartContigMsg;
        vc_ch->iSendContig = MPID_nem_scif_iSendContig;

        vc_ch->pkt_handler = NULL;      // pkt_handlers;
        vc_ch->num_pkt_handlers = 0;    // MPIDI_NEM_SCIF_PKT_NUM_TYPES;
        vc_ch->next = NULL;
        vc_ch->prev = NULL;

        ASSIGN_SC_TO_VC(vc_scif, NULL);
        vc_scif->send_queue.head = vc_scif->send_queue.tail = NULL;
        vc_scif->sc = sc = &MPID_nem_scif_conns[vc->pg_rank];
        vc_scif->terminate = 0;
        sc->vc = vc;

        sc->fd = scif_open();
        MPIU_ERR_CHKANDJUMP1(sc->fd == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_open", "**scif_open %s", MPIU_Strerror(errno));
        mpi_errno = get_addr(vc, &sc->addr);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        ret = scif_connect(sc->fd, &sc->addr);
        MPIU_ERR_CHKANDJUMP1(ret == -1, mpi_errno, MPI_ERR_OTHER,
                             "**scif_connect", "**scif_connect %s", MPIU_Strerror(errno));
        s = scif_send(sc->fd, &MPID_nem_scif_myrank, sizeof(MPID_nem_scif_myrank), SCIF_SEND_BLOCK);
        MPIU_ERR_CHKANDJUMP1(s != sizeof(MPID_nem_scif_myrank), mpi_errno, MPI_ERR_OTHER,
                             "**scif_send", "**scif_send %s", MPIU_Strerror(errno));
    }
    else {
        struct scif_portID portID;
        int fd;
        // Can accept a connection from any peer, not necessary from vc->pg_rank.
        // So we need to know the actual peer and adjust vc.
        ret = scif_accept(listen_fd, &portID, &fd, SCIF_ACCEPT_SYNC);
        MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER,
                             "**scif_accept", "**scif_accept %s", MPIU_Strerror(errno));
        s = scif_recv(fd, &peer_rank, sizeof(peer_rank), SCIF_RECV_BLOCK);
        MPIU_ERR_CHKANDJUMP1(s != sizeof(peer_rank), mpi_errno, MPI_ERR_OTHER, "**scif_recv",
                             "**scif_recv %s", MPIU_Strerror(errno));
        // check and adjust vc
        if (peer_rank != vc->pg_rank) {
            // get another vc
            MPIDI_PG_Get_vc(MPIDI_Process.my_pg, peer_rank, &vc);
            // check another new corresponds to actual peer_rank
            MPIU_ERR_CHKANDJUMP1(peer_rank != vc->pg_rank, mpi_errno, MPI_ERR_OTHER,
                                 "**MPIDI_PG_Get_vc", "**MPIDI_PG_Get_vc %s",
                                 "wrong vc after accept");
        }
        vc_ch = &vc->ch;
        vc_scif = VC_SCIF(vc);

        vc->sendNoncontig_fn = MPID_nem_scif_SendNoncontig;
        vc_ch->iStartContigMsg = MPID_nem_scif_iStartContigMsg;
        vc_ch->iSendContig = MPID_nem_scif_iSendContig;
        vc_ch->pkt_handler = NULL;      // pkt_handlers;
        vc_ch->num_pkt_handlers = 0;    // MPIDI_NEM_SCIF_PKT_NUM_TYPES;
        vc_ch->next = NULL;
        vc_ch->prev = NULL;
        ASSIGN_SC_TO_VC(vc_scif, NULL);
        vc_scif->send_queue.head = vc_scif->send_queue.tail = NULL;
        vc_scif->sc = sc = &MPID_nem_scif_conns[vc->pg_rank];
        vc_scif->terminate = 0;
        sc->vc = vc;
        sc->addr = portID;
        sc->fd = fd;
    }
    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);
    ret = MPID_nem_scif_init_shmsend(&sc->csend, sc->fd, vc->pg_rank);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER,
                         "**scif_init_shmsend", "**scif_init_shmsend %s", MPIU_Strerror(errno));

    /* Exchange offsets */
    s = scif_send(sc->fd, &sc->csend.offset, sizeof(off_t), SCIF_SEND_BLOCK);
    MPIU_ERR_CHKANDJUMP1(s != sizeof(off_t), mpi_errno, MPI_ERR_OTHER,
                         "**scif_send", "**scif_send %s", MPIU_Strerror(errno));
    s = scif_recv(sc->fd, &offset, sizeof(off_t), SCIF_RECV_BLOCK);
    MPIU_ERR_CHKANDJUMP1(s != sizeof(off_t), mpi_errno, MPI_ERR_OTHER,
                         "**scif_recv", "**scif_recv %s", MPIU_Strerror(errno));

    ret = MPID_nem_scif_init_shmrecv(&sc->crecv, sc->fd, offset, vc->pg_rank);
    MPIU_ERR_CHKANDJUMP1(ret, mpi_errno, MPI_ERR_OTHER,
                         "**scif_init_shmrecv", "**scif_init_shmrecv %s", MPIU_Strerror(errno));

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_VC_INIT);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_vc_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_vc_destroy(MPIDI_VC_t * vc)
{
    /* currently do nothing */
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_vc_terminate
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_scif_vc_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    int req_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_SCIF_VC_TERMINATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_SCIF_VC_TERMINATE);

    if (vc->state != MPIDI_VC_STATE_CLOSED) {
        /* VC is terminated as a result of a fault.  Complete
         * outstanding sends with an error and terminate connection
         * immediately. */
        MPIU_ERR_SET1(req_errno, MPI_ERR_OTHER, "**comm_fail", "**comm_fail %d", vc->pg_rank);
        mpi_errno = MPID_nem_scif_error_out_send_queue(vc, req_errno);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
        mpi_errno = MPID_nem_scif_vc_terminated(vc);
        if (mpi_errno)
            MPIU_ERR_POP(mpi_errno);
    }
    else {
        MPID_nem_scif_vc_area *vc_scif = VC_SCIF(vc);

        vc_scif->terminate = 1;
        /* VC is terminated as a result of the close protocol.  Wait
         * for sends to complete, then terminate. */

        if (MPIDI_CH3I_Sendq_empty(vc_scif->send_queue)) {
            /* The sendq is empty, so we can immediately terminate the
             * connection. */
            mpi_errno = MPID_nem_scif_vc_terminated(vc);
            if (mpi_errno)
                MPIU_ERR_POP(mpi_errno);
        }
        /* else: just return.  We'll call vc_terminated() from the
         * commrdy_handler once the sendq is empty. */
    }

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_SCIF_VC_TERMINATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_scif_vc_terminated
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_scif_vc_terminated(MPIDI_VC_t * vc)
{
    /* This is called when the VC is to be terminated once all queued
     * sends have been sent. */
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_NEM_SCIF_VC_TERMINATED);

    MPIDI_FUNC_ENTER(MPID_NEM_SCIF_VC_TERMINATED);

    mpi_errno = MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED);
    if (mpi_errno)
        MPIU_ERR_POP(mpi_errno);

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_NEM_SCIF_VC_TERMINATED);
    return mpi_errno;
  fn_fail:
    MPIU_DBG_MSG_FMT(NEM_SOCK_DET, VERBOSE, (MPIU_DBG_FDEST, "failure. mpi_errno = %d", mpi_errno));
    goto fn_exit;
}
