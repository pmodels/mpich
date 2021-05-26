/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "ofi_impl.h"

/* ------------------------------------------------------------------------ */
/* ofi_tag_to_vc                                                            */
/* This routine converts tag information from an incoming preposted receive */
/* into the VC that uses the routine.  There is a possibility of a small    */
/* list of temporary VC's that are used during dynamic task management      */
/* to create the VC's.  This search is linear, but should be a small number */
/* of temporary VC's that will eventually be destroyed by the upper layers  */
/* Otherwise the tag is split into a PG "number", which is a hash of the    */
/* data contained in the process group, and a source.  The source/pg number */
/* is enough to look up the VC.                                             */
/* ------------------------------------------------------------------------ */
static inline MPIDI_VC_t *ofi_wc_to_vc(cq_tagged_entry_t * wc)
{
    int pgid = 0, port = 0;
    MPIDI_VC_t *vc = NULL;
    MPIDI_PG_t *pg = NULL;
    uint64_t match_bits = wc->tag;
    int wc_pgid;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_OFI_TAG_TO_VC);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_OFI_TAG_TO_VC);
    if (gl_data.api_set == API_SET_1) {
        wc_pgid = get_pgid(match_bits);
    } else {
        wc_pgid = wc->data;
    }

    if (NO_PGID == wc_pgid) {
        /* -------------------------------------------------------------------- */
        /* Dynamic path -- This uses a linear search, but number of cm vc's is  */
        /* a small number, and they should be ephemeral.  This lookup should    */
        /* be fast yet not normally on the critical path.                       */
        /* -------------------------------------------------------------------- */
        port = get_port(match_bits);
        vc = gl_data.cm_vcs;
        while (vc && vc->port_name_tag != port) {
            vc = VC_OFI(vc)->next;
        }
        if (NULL == vc) {
            MPIR_Assertp(0);
        }
    } else {
        /* -------------------------------------------------------------------- */
        /* If there are no connection management VC's, this is the normal path  */
        /* Generate the PG number has from each known process group compare to  */
        /* the pg number in the tag.  The number of PG's should be small        */
        /* -------------------------------------------------------------------- */
        pg = gl_data.pg_p;
        while (pg) {
            MPIDI_PG_IdToNum(pg, &pgid);
            if (wc_pgid == pgid) {
                break;
            }
            pg = pg->next;
        }
        if (pg) {
            MPIDI_PG_Get_vc(pg, get_psource(match_bits), &vc);
        } else {
            MPIR_Assert(0);
        }
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_OFI_TAG_TO_VC);
    return vc;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_conn_req_callback                                           */
/* A new process has been created and is connected to the current world     */
/* The address of the new process is exchanged via the business card        */
/* instead of being exchanged up front during the creation of the first     */
/* world.  The new connection routine is usually invoked when two worlds    */
/* are started via dynamic tasking.                                         */
/* This routine:                                                            */
/*     * repost the persistent connection management receive request        */
/*     * malloc/create/initialize the VC                                    */
/*     * grabs the address name from the business card                      */
/*     * uses fi_av_insert to insert the addr into the address vector.      */
/* This is marked as a "connection management" vc, and may be destroyed     */
/* by the upper layers.  We handle the cm vc's slightly differently than    */
/* other VC's because they may not be part of a process group.              */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_conn_req_callback(cq_tagged_entry_t * wc, MPIR_Request * rreq)
{
    int ret, len, mpi_errno = MPI_SUCCESS;
    char bc[MPIDI_OFI_KVSAPPSTRLEN];

    MPIDI_VC_t *vc;
    char *addr = NULL;
    fi_addr_t direct_addr;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CONN_REQ_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CONN_REQ_CALLBACK);

    MPIR_Memcpy(bc, rreq->dev.user_buf, wc->len);
    bc[wc->len] = '\0';
    MPIR_Assert(gl_data.conn_req == rreq);
    FI_RC_RETRY(fi_trecv(gl_data.endpoint,
                         gl_data.conn_req->dev.user_buf,
                         MPIDI_OFI_KVSAPPSTRLEN,
                         gl_data.mr,
                         FI_ADDR_UNSPEC,
                         MPIDI_OFI_CONN_REQ,
                         GET_RCD_IGNORE_MASK(),
                         (void *) &(REQ_OFI(gl_data.conn_req)->ofi_context)), trecv);

    addr = MPL_malloc(gl_data.bound_addrlen, MPL_MEM_ADDRESS);
    MPIR_Assertp(addr);

    vc = MPL_malloc(sizeof(MPIDI_VC_t), MPL_MEM_ADDRESS);
    MPIR_Assertp(vc);

    MPIDI_VC_Init(vc, NULL, 0);
    MPIDI_CH3I_NM_OFI_RC(MPIDI_GetTagFromPort(bc, &vc->port_name_tag));
    ret = MPL_str_get_binary_arg(bc, "OFI", addr, gl_data.bound_addrlen, &len);
    MPIR_ERR_CHKANDJUMP((ret != MPL_SUCCESS && ret != MPL_ERR_STR_NOMEM) ||
                        (size_t) len != gl_data.bound_addrlen,
                        mpi_errno, MPI_ERR_OTHER, "**business_card");

    FI_RC(fi_av_insert(gl_data.av, addr, 1, &direct_addr, 0ULL, NULL), avmap);
    VC_OFI(vc)->direct_addr = direct_addr;
    VC_OFI(vc)->ready = 1;
    VC_OFI(vc)->is_cmvc = 1;
    VC_OFI(vc)->next = gl_data.cm_vcs;
    gl_data.cm_vcs = vc;

    MPIDI_CH3I_Acceptq_enqueue(vc, vc->port_name_tag);
    MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT;
  fn_exit:
    MPL_free(addr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CONN_REQ_CALLBACK);
    return mpi_errno;
  fn_fail:
    MPL_free(vc);
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_handle_packet                                               */
/* The "parent" request tracks the state of the entire rendezvous           */
/* As "child" requests complete, the cc counter is decremented              */
/* Notify CH3 that we have an incoming packet (if cc hits 1).  Otherwise    */
/* decrement the ref counter via request completion                         */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_handle_packet(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                             MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t *vc;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_HANDLE_PACKET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_HANDLE_PACKET);
    if (MPIR_cc_get(rreq->cc) == 1) {
        vc = REQ_OFI(rreq)->vc;
        MPIR_Assert(vc);
        MPIDI_CH3I_NM_OFI_RC(MPID_nem_handle_pkt
                             (vc, REQ_OFI(rreq)->pack_buffer, REQ_OFI(rreq)->pack_buffer_size));
        MPL_free(REQ_OFI(rreq)->pack_buffer);
    }
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(rreq));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_HANDLE_PACKET);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_cts_send_callback                                           */
/* A wrapper around MPID_nem_ofi_handle_packet that decrements              */
/* the parent request's counter, and cleans up the CTS request              */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_cts_send_callback(cq_tagged_entry_t * wc, MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CTS_SEND_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CTS_SEND_CALLBACK);
    MPIDI_CH3I_NM_OFI_RC(MPID_nem_ofi_handle_packet(wc, REQ_OFI(sreq)->parent));
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CTS_SEND_CALLBACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_preposted_callback                                          */
/* This callback handles incoming "SendContig" messages (see ofi_msg.c)     */
/* for the send routines.  This implements the CTS response and the RTS     */
/* handler.  The steps are as follows:                                      */
/*   * Create a parent data request and post a receive into a pack buffer   */
/*   * Create a child request and send the CTS packet                       */
/*   * Re-Post the RTS receive and handler to handle the next message       */
/* ------------------------------------------------------------------------ */
static inline int MPID_nem_ofi_preposted_callback(cq_tagged_entry_t * wc, MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    size_t pkt_len;
    char *pack_buffer = NULL;
    MPIDI_VC_t *vc;
    MPIR_Request *new_rreq, *sreq;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_PREPOSTED_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_PREPOSTED_CALLBACK);

    vc = ofi_wc_to_vc(wc);
    MPIR_Assert(vc);
    VC_READY_CHECK(vc);

    pkt_len = REQ_OFI(rreq)->msg_bytes;
    pack_buffer = (char *) MPL_malloc(pkt_len, MPL_MEM_BUFFER);
    /* If the pack buffer is NULL, let OFI handle the truncation
     * in the progress loop
     */
    if (pack_buffer == NULL)
        pkt_len = 0;
    MPID_nem_ofi_create_req(&new_rreq, 1);
    MPIR_cc_inc(new_rreq->cc_ptr);
    new_rreq->dev.OnDataAvail = NULL;
    new_rreq->dev.next = NULL;
    REQ_OFI(new_rreq)->event_callback = MPID_nem_ofi_handle_packet;
    REQ_OFI(new_rreq)->vc = vc;
    REQ_OFI(new_rreq)->pack_buffer = pack_buffer;
    REQ_OFI(new_rreq)->pack_buffer_size = pkt_len;
    FI_RC_RETRY(fi_trecv(gl_data.endpoint,
                         REQ_OFI(new_rreq)->pack_buffer,
                         REQ_OFI(new_rreq)->pack_buffer_size,
                         gl_data.mr,
                         VC_OFI(vc)->direct_addr,
                         wc->tag | MPIDI_OFI_MSG_CTS | MPIDI_OFI_MSG_DATA, 0,
                         &(REQ_OFI(new_rreq)->ofi_context)), trecv);

    MPID_nem_ofi_create_req(&sreq, 1);
    sreq->dev.OnDataAvail = NULL;
    sreq->dev.next = NULL;
    REQ_OFI(sreq)->event_callback = MPID_nem_ofi_cts_send_callback;
    REQ_OFI(sreq)->parent = new_rreq;
    FI_RC_RETRY(fi_tsend(gl_data.endpoint,
                         NULL,
                         0,
                         gl_data.mr,
                         VC_OFI(vc)->direct_addr,
                         wc->tag | MPIDI_OFI_MSG_CTS, &(REQ_OFI(sreq)->ofi_context)), tsend);
    MPIR_Assert(gl_data.persistent_req == rreq);

    FI_RC_RETRY(fi_trecv(gl_data.endpoint,
                         &REQ_OFI(rreq)->msg_bytes,
                         sizeof REQ_OFI(rreq)->msg_bytes,
                         gl_data.mr,
                         FI_ADDR_UNSPEC,
                         MPIDI_OFI_MSG_RTS,
                         GET_RCD_IGNORE_MASK(), &(REQ_OFI(rreq)->ofi_context)), trecv);
    /* Return a proper error to MPI to indicate out of memory condition */
    MPIR_ERR_CHKANDJUMP1(pack_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                         "**nomem", "**nomem %s", "Pack Buffer alloc");
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_PREPOSTED_CALLBACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_connect_to_root_callback                                    */
/* Complete and clean up the request                                        */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_connect_to_root_callback(cq_tagged_entry_t * wc ATTRIBUTE((unused)),
                                          MPIR_Request * sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CONNECT_TO_ROOT_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CONNECT_TO_ROOT_CALLBACK);

    MPL_free(REQ_OFI(sreq)->pack_buffer);

    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(sreq));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CONNECT_TO_ROOT_CALLBACK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_cm_init                                                     */
/* This is a utility routine that sets up persistent connection management  */
/* requests and a persistent data request to handle rendezvous SendContig   */
/* messages.                                                                */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_cm_init(MPIDI_PG_t * pg_p, int pg_rank ATTRIBUTE((unused)))
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_Request *persistent_req, *conn_req;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CM_INIT);

    /* ------------------------------------- */
    /* Set up CH3 and netmod data structures */
    /* ------------------------------------- */
    if (gl_data.api_set == API_SET_1) {
        MPIDI_CH3I_NM_OFI_RC(MPIDI_CH3I_Register_anysource_notification
                             (MPID_nem_ofi_anysource_posted, MPID_nem_ofi_anysource_matched));
        MPIDI_Anysource_iprobe_fn = MPID_nem_ofi_anysource_iprobe;
        MPIDI_Anysource_improbe_fn = MPID_nem_ofi_anysource_improbe;
    } else {
        MPIDI_CH3I_NM_OFI_RC(MPIDI_CH3I_Register_anysource_notification
                             (MPID_nem_ofi_anysource_posted_2, MPID_nem_ofi_anysource_matched));
        MPIDI_Anysource_iprobe_fn = MPID_nem_ofi_anysource_iprobe_2;
        MPIDI_Anysource_improbe_fn = MPID_nem_ofi_anysource_improbe_2;
    }
    gl_data.pg_p = pg_p;

    /* ----------------------------------- */
    /* Post a persistent request to handle */
    /* ----------------------------------- */
    MPID_nem_ofi_create_req(&persistent_req, 1);
    persistent_req->dev.OnDataAvail = NULL;
    persistent_req->dev.next = NULL;
    REQ_OFI(persistent_req)->vc = NULL;
    REQ_OFI(persistent_req)->event_callback = MPID_nem_ofi_preposted_callback;
    FI_RC_RETRY(fi_trecv(gl_data.endpoint,
                         &REQ_OFI(persistent_req)->msg_bytes,
                         sizeof REQ_OFI(persistent_req)->msg_bytes,
                         gl_data.mr,
                         FI_ADDR_UNSPEC,
                         MPIDI_OFI_MSG_RTS,
                         GET_RCD_IGNORE_MASK(),
                         (void *) &(REQ_OFI(persistent_req)->ofi_context)), trecv);
    gl_data.persistent_req = persistent_req;

    /* --------------------------------- */
    /* Post recv for connection requests */
    /* --------------------------------- */
    MPID_nem_ofi_create_req(&conn_req, 1);
    conn_req->dev.user_buf = MPL_malloc(MPIDI_OFI_KVSAPPSTRLEN * sizeof(char), MPL_MEM_BUFFER);
    conn_req->dev.OnDataAvail = NULL;
    conn_req->dev.next = NULL;
    REQ_OFI(conn_req)->vc = NULL;       /* We don't know the source yet */
    REQ_OFI(conn_req)->event_callback = MPID_nem_ofi_conn_req_callback;
    FI_RC_RETRY(fi_trecv(gl_data.endpoint,
                         conn_req->dev.user_buf,
                         MPIDI_OFI_KVSAPPSTRLEN,
                         gl_data.mr,
                         FI_ADDR_UNSPEC,
                         MPIDI_OFI_CONN_REQ,
                         GET_RCD_IGNORE_MASK(), (void *) &(REQ_OFI(conn_req)->ofi_context)), trecv);
    gl_data.conn_req = conn_req;


  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CM_INIT);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_cm_finalize                                                 */
/* Clean up and cancel the requests initiated by the cm_init routine        */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_cm_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_CM_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_CM_FINALIZE);
    FI_RC(fi_cancel((fid_t) gl_data.endpoint,
                    &(REQ_OFI(gl_data.persistent_req)->ofi_context)), cancel);
    MPIR_STATUS_SET_CANCEL_BIT(gl_data.persistent_req->status, TRUE);
    MPIR_STATUS_SET_COUNT(gl_data.persistent_req->status, 0);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(gl_data.persistent_req));

    FI_RC(fi_cancel((fid_t) gl_data.endpoint, &(REQ_OFI(gl_data.conn_req)->ofi_context)), cancel);
    MPL_free(gl_data.conn_req->dev.user_buf);
    MPIR_STATUS_SET_CANCEL_BIT(gl_data.conn_req->status, TRUE);
    MPIR_STATUS_SET_COUNT(gl_data.conn_req->status, 0);
    MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(gl_data.conn_req));
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_CM_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_vc_connect                                                  */
/* Handle CH3/Nemesis VC connections                                        */
/*   * Query the VC address information.  In particular we are looking for  */
/*     the fabric address name.                                             */
/*   * Use fi_av_insert to register the address name with OFI               */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_vc_connect(MPIDI_VC_t * vc)
{
    int len, ret, mpi_errno = MPI_SUCCESS;
    char bc[MPIDI_OFI_KVSAPPSTRLEN], *addr = NULL;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_VC_CONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_VC_CONNECT);
    addr = MPL_malloc(gl_data.bound_addrlen, MPL_MEM_ADDRESS);
    MPIR_Assert(addr);
    MPIR_Assert(1 != VC_OFI(vc)->ready);

    if (!vc->pg || !vc->pg->getConnInfo) {
        goto fn_exit;
    }

    MPIDI_CH3I_NM_OFI_RC(vc->pg->getConnInfo(vc->pg_rank, bc, MPIDI_OFI_KVSAPPSTRLEN, vc->pg));
    ret = MPL_str_get_binary_arg(bc, "OFI", addr, gl_data.bound_addrlen, &len);
    MPIR_ERR_CHKANDJUMP((ret != MPL_SUCCESS && ret != MPL_ERR_STR_NOMEM) ||
                        (size_t) len != gl_data.bound_addrlen,
                        mpi_errno, MPI_ERR_OTHER, "**business_card");
    FI_RC(fi_av_insert(gl_data.av, addr, 1, &(VC_OFI(vc)->direct_addr), 0ULL, NULL), avmap);
    VC_OFI(vc)->ready = 1;

  fn_exit:
    MPL_free(addr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_VC_CONNECT);
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

int MPID_nem_ofi_vc_init(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *const vc_ch = &vc->ch;
    MPID_nem_ofi_vc_t *const vc_ofi = VC_OFI(vc);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_VC_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_VC_INIT);
    vc->sendNoncontig_fn = MPID_nem_ofi_SendNoncontig;
    vc_ch->iStartContigMsg = MPID_nem_ofi_iStartContigMsg;
    vc_ch->iSendContig = MPID_nem_ofi_iSendContig;
    vc_ch->iSendIov = MPID_nem_ofi_iSendIov;
    vc_ch->next = NULL;
    vc_ch->prev = NULL;
    vc_ofi->is_cmvc = 0;
    vc->comm_ops = &_g_comm_ops;

    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    if (NULL == vc->pg) {
        vc_ofi->is_cmvc = 1;
    } else {
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_VC_INIT);
    return mpi_errno;
}

/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_vc_destroy                                                  */
/* MPID_nem_ofi_vc_terminate                                                */
/* TODO:  Verify this code has no leaks                                     */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_vc_destroy(MPIDI_VC_t * vc)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_VC_DESTROY);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_VC_DESTROY);
    if (gl_data.cm_vcs && vc && (VC_OFI(vc)->is_cmvc == 1)) {
        if (vc->pg != NULL) {
            printf("ERROR: VC Destroy (%p) pg = %s\n", vc, (char *) vc->pg->id);
        }
        MPIDI_VC_t *prev = gl_data.cm_vcs;
        while (prev && prev != vc && VC_OFI(prev)->next != vc) {
            prev = VC_OFI(prev)->next;
        }

        MPIR_Assert(prev != NULL);

        if (VC_OFI(prev)->next == vc) {
            VC_OFI(prev)->next = VC_OFI(vc)->next;
        } else if (vc == gl_data.cm_vcs) {
            gl_data.cm_vcs = VC_OFI(vc)->next;
        } else {
            MPIR_Assert(0);
        }
    }
    VC_OFI(vc)->ready = 0;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_VC_DESTROY);
    return MPI_SUCCESS;
}

int MPID_nem_ofi_vc_terminate(MPIDI_VC_t * vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_VC_TERMINATE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_VC_TERMINATE);
    MPIDI_CH3I_NM_OFI_RC(MPIDI_CH3U_Handle_connection(vc, MPIDI_VC_EVENT_TERMINATED));
    VC_OFI(vc)->ready = 0;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_VC_TERMINATE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}



/* ------------------------------------------------------------------------ */
/* MPID_nem_ofi_connect_to_root                                             */
/*  * A new unconnected VC (cm/ephemeral VC) has been created.  This code   */
/*    connects the new VC to a rank in another process group.  The parent   */
/*    address is obtained by an out of band method and given to this        */
/*    routine as a business card                                            */
/*  * Read the business card address and insert the address                 */
/*  * Send a connection request to the parent.  The parent has posted a     */
/*    persistent request to handle incoming connection requests             */
/*    The connect message has the child's business card.                    */
/*  * Add the new VC to the list of ephemeral BC's (cm_vc's).  These VC's   */
/*    are not part of the process group, so they require special handling   */
/*    during the SendContig family of routines.                             */
/* ------------------------------------------------------------------------ */
int MPID_nem_ofi_connect_to_root(const char *business_card, MPIDI_VC_t * new_vc)
{
    int len, ret, mpi_errno = MPI_SUCCESS, str_errno = MPI_SUCCESS;
    int my_bc_len = MPIDI_OFI_KVSAPPSTRLEN;
    char *addr = NULL, *bc = NULL, *my_bc = NULL;
    MPIR_Request *sreq;
    uint64_t conn_req_send_bits;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_NM_CONNECT_TO_ROOT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_NM_CONNECT_TO_ROOT);
    addr = MPL_malloc(gl_data.bound_addrlen, MPL_MEM_ADDRESS);
    bc = MPL_malloc(MPIDI_OFI_KVSAPPSTRLEN, MPL_MEM_ADDRESS);
    MPIR_Assertp(addr);
    MPIR_Assertp(bc);
    my_bc = bc;
    if (!business_card || business_card[0] != 't') {
        mpi_errno = MPI_ERR_OTHER;
        goto fn_fail;
    }
    MPIDI_CH3I_NM_OFI_RC(MPIDI_GetTagFromPort(business_card, &new_vc->port_name_tag));
    ret = MPL_str_get_binary_arg(business_card, "OFI", addr, gl_data.bound_addrlen, &len);
    MPIR_ERR_CHKANDJUMP((ret != MPL_SUCCESS && ret != MPL_ERR_STR_NOMEM) ||
                        (size_t) len != gl_data.bound_addrlen,
                        mpi_errno, MPI_ERR_OTHER, "**business_card");
    FI_RC(fi_av_insert(gl_data.av, addr, 1, &(VC_OFI(new_vc)->direct_addr), 0ULL, NULL), avmap);

    VC_OFI(new_vc)->ready = 1;
    str_errno = MPL_str_add_int_arg(&bc, &my_bc_len, "tag", new_vc->port_name_tag);
    MPIR_ERR_CHKANDJUMP(str_errno, mpi_errno, MPI_ERR_OTHER, "**argstr_port_name_tag");
    MPIDI_CH3I_NM_OFI_RC(MPID_nem_ofi_get_business_card
                         (MPIR_Process.comm_world->rank, &bc, &my_bc_len));
    my_bc_len = MPIDI_OFI_KVSAPPSTRLEN - my_bc_len;

    MPID_nem_ofi_create_req(&sreq, 1);
    sreq->kind = MPIR_REQUEST_KIND__SEND;
    sreq->dev.OnDataAvail = NULL;
    sreq->dev.next = NULL;
    REQ_OFI(sreq)->event_callback = MPID_nem_ofi_connect_to_root_callback;
    REQ_OFI(sreq)->pack_buffer = my_bc;
    if (gl_data.api_set == API_SET_1) {
        conn_req_send_bits = init_sendtag(0, MPIR_Process.comm_world->rank, 0, MPIDI_OFI_CONN_REQ);
        FI_RC_RETRY(fi_tsend(gl_data.endpoint,
                             REQ_OFI(sreq)->pack_buffer,
                             my_bc_len,
                             gl_data.mr,
                             VC_OFI(new_vc)->direct_addr,
                             conn_req_send_bits, &(REQ_OFI(sreq)->ofi_context)), tsend);
    } else {
        conn_req_send_bits = init_sendtag_2(0, 0, MPIDI_OFI_CONN_REQ);
        FI_RC_RETRY(fi_tsenddata(gl_data.endpoint,
                                 REQ_OFI(sreq)->pack_buffer,
                                 my_bc_len,
                                 gl_data.mr,
                                 MPIR_Process.comm_world->rank,
                                 VC_OFI(new_vc)->direct_addr,
                                 conn_req_send_bits, &(REQ_OFI(sreq)->ofi_context)), tsend);
    }
    MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
    VC_OFI(new_vc)->is_cmvc = 1;
    VC_OFI(new_vc)->next = gl_data.cm_vcs;
    gl_data.cm_vcs = new_vc;
  fn_exit:
    MPL_free(addr);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_NM_CONNECT_TO_ROOT);
    return mpi_errno;
  fn_fail:
    MPL_free(my_bc);
    goto fn_exit;
}

int MPID_nem_ofi_get_business_card(int my_rank ATTRIBUTE((unused)),
                                   char **bc_val_p, int *val_max_sz_p)
{
    int mpi_errno = MPI_SUCCESS, str_errno = MPL_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_GET_BUSINESS_CARD);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_GET_BUSINESS_CARD);
    str_errno = MPL_str_add_binary_arg(bc_val_p,
                                       val_max_sz_p,
                                       "OFI", (char *) &gl_data.bound_addr, gl_data.bound_addrlen);
    if (str_errno) {
        MPIR_ERR_CHKANDJUMP(str_errno == MPL_ERR_STR_NOMEM, mpi_errno, MPI_ERR_OTHER, "**buscard_len");
        MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**buscard");
    }
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_GET_BUSINESS_CARD);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
