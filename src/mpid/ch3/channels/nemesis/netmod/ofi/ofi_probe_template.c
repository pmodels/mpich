/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#if (API_SET != API_SET_1) && (API_SET != API_SET_2)
#error Undefined API SET
#endif

/* ------------------------------------------------------------------------ */
/* peek_callback called when a successful peek is completed                 */
/* ------------------------------------------------------------------------ */
static int ADD_SUFFIX(peek_callback) (cq_tagged_entry_t * wc, MPIR_Request * rreq) {
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_PEEK_CALLBACK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_PEEK_CALLBACK);
    REQ_OFI(rreq)->match_state = PEEK_FOUND;
#if API_SET == API_SET_1
    rreq->status.MPI_SOURCE = get_source(wc->tag);
#elif API_SET == API_SET_2
    rreq->status.MPI_SOURCE = wc->data;
#endif
    rreq->status.MPI_TAG = get_tag(wc->tag);
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);
    rreq->status.MPI_ERROR = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_PEEK_CALLBACK);
    return mpi_errno;
}

int ADD_SUFFIX(MPID_nem_ofi_iprobe_impl) (struct MPIDI_VC * vc,
                                          int source,
                                          int tag,
                                          MPIR_Comm * comm,
                                          int context_offset,
                                          int *flag, MPI_Status * status, MPIR_Request ** rreq_ptr)
{
    int ret, mpi_errno = MPI_SUCCESS;
    fi_addr_t remote_proc = 0;
    uint64_t match_bits, mask_bits;
    size_t len;
    MPIR_Request rreq_s, *rreq;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_IPROBE_IMPL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_IPROBE_IMPL);
    if (rreq_ptr) {
        MPIDI_CH3I_NM_OFI_RC(MPID_nem_ofi_create_req(&rreq, 1));
        rreq->kind = MPIR_REQUEST_KIND__RECV;

        *rreq_ptr = rreq;
        rreq->comm = comm;
        rreq->dev.match.parts.rank = source;
        rreq->dev.match.parts.tag = tag;
        rreq->dev.match.parts.context_id = comm->context_id;
        MPIR_Comm_add_ref(comm);
    } else {
        rreq = &rreq_s;
        rreq->dev.OnDataAvail = NULL;
    }

    REQ_OFI(rreq)->pack_buffer = NULL;
    REQ_OFI(rreq)->event_callback = ADD_SUFFIX(peek_callback);
    REQ_OFI(rreq)->match_state = PEEK_INIT;
    OFI_ADDR_INIT(source, vc, remote_proc);
#if API_SET == API_SET_1
    match_bits = init_recvtag(&mask_bits, comm->recvcontext_id + context_offset, source, tag);
#elif API_SET == API_SET_2
    match_bits = init_recvtag_2(&mask_bits, comm->recvcontext_id + context_offset, tag);
#endif

    /* ------------------------------------------------------------------------- */
    /* fi_recvmsg with FI_PEEK:                                                  */
    /* Initiate a search for a match in the hardware or software queue.          */
    /* The search can complete immediately with -ENOMSG.                         */
    /* I successful, libfabric will enqueue a context entry into the completion  */
    /* queue to make the search nonblocking.  This code will poll until the      */
    /* entry is enqueued.                                                        */
    /* ------------------------------------------------------------------------- */
    msg_tagged_t msg;
    uint64_t msgflags = FI_PEEK;
    msg.msg_iov = NULL;
    msg.desc = NULL;
    msg.iov_count = 0;
    msg.addr = remote_proc;
    msg.tag = match_bits;
    msg.ignore = mask_bits;
    msg.context = (void *) &(REQ_OFI(rreq)->ofi_context);
    msg.data = 0;
    if (*flag == CLAIM_PEEK)
        msgflags |= FI_CLAIM;
    ret = fi_trecvmsg(gl_data.endpoint, &msg, msgflags);
    if (ret == -ENOMSG) {
        if (rreq_ptr) {
            MPIR_Request_free(rreq);
            *rreq_ptr = NULL;
            *flag = 0;
        }
        MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
        goto fn_exit;
    }
    MPIR_ERR_CHKANDJUMP4((ret < 0), mpi_errno, MPI_ERR_OTHER,
                         "**ofi_peek", "**ofi_peek %s %d %s %s",
                         __SHORT_FILE__, __LINE__, __func__, fi_strerror(-ret));

    while (PEEK_INIT == REQ_OFI(rreq)->match_state)
        MPID_nem_ofi_poll(MPID_BLOCKING_POLL);

    if (PEEK_NOT_FOUND == REQ_OFI(rreq)->match_state) {
        if (rreq_ptr) {
            MPIR_Request_free(rreq);
            *rreq_ptr = NULL;
            *flag = 0;
        }
        MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
        goto fn_exit;
    }

    if (status != MPI_STATUS_IGNORE)
        *status = rreq->status;

    if (rreq_ptr)
        MPIR_Request_add_ref(rreq);
    *flag = 1;
  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_IPROBE_IMPL);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


int ADD_SUFFIX(MPID_nem_ofi_iprobe) (struct MPIDI_VC * vc,
                                     int source,
                                     int tag,
                                     MPIR_Comm * comm, int context_offset, int *flag,
                                     MPI_Status * status) {
    int rc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_IPROBE);
    *flag = 0;
    rc = ADD_SUFFIX(MPID_nem_ofi_iprobe_impl) (vc, source,
                                               tag, comm, context_offset, flag, status, NULL);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_IPROBE);
    return rc;
}

int ADD_SUFFIX(MPID_nem_ofi_improbe) (struct MPIDI_VC * vc,
                                      int source,
                                      int tag,
                                      MPIR_Comm * comm,
                                      int context_offset,
                                      int *flag, MPIR_Request ** message, MPI_Status * status) {
    int old_error;
    int s;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_IMPROBE);
    *flag = CLAIM_PEEK;
    if (status != MPI_STATUS_IGNORE) {
        old_error = status->MPI_ERROR;
    }
    s = ADD_SUFFIX(MPID_nem_ofi_iprobe_impl) (vc, source,
                                              tag, comm, context_offset, flag, status, message);
    if (*flag) {
        if (status != MPI_STATUS_IGNORE) {
            status->MPI_ERROR = old_error;
        }
        (*message)->kind = MPIR_REQUEST_KIND__MPROBE;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_IMPROBE);
    return s;
}

int ADD_SUFFIX(MPID_nem_ofi_anysource_iprobe) (int tag,
                                               MPIR_Comm * comm,
                                               int context_offset, int *flag, MPI_Status * status) {
    int rc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IPROBE);
    *flag = NORMAL_PEEK;
    rc = ADD_SUFFIX(MPID_nem_ofi_iprobe) (NULL, MPI_ANY_SOURCE,
                                          tag, comm, context_offset, flag, status);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IPROBE);
    return rc;
}

int ADD_SUFFIX(MPID_nem_ofi_anysource_improbe) (int tag,
                                                MPIR_Comm * comm,
                                                int context_offset,
                                                int *flag, MPIR_Request ** message,
                                                MPI_Status * status) {
    int rc;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IMPROBE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IMPROBE);
    *flag = CLAIM_PEEK;
    rc = ADD_SUFFIX(MPID_nem_ofi_improbe) (NULL, MPI_ANY_SOURCE, tag, comm,
                                           context_offset, flag, message, status);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_OFI_ANYSOURCE_IMPROBE);
    return rc;
}
