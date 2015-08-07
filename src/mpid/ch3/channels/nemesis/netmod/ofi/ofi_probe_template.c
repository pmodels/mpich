#if (API_SET != API_SET_1) && (API_SET != API_SET_2)
#error Undefined API SET
#endif

/* ------------------------------------------------------------------------ */
/* peek_callback called when a successful peek is completed                 */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(peek_callback)
static int
ADD_SUFFIX(peek_callback)(cq_tagged_entry_t * wc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    REQ_OFI(rreq)->match_state = PEEK_FOUND;
#if API_SET == API_SET_1
    rreq->status.MPI_SOURCE    = get_source(wc->tag);
#elif API_SET == API_SET_2
    rreq->status.MPI_SOURCE    = wc->data;
#endif
    rreq->status.MPI_TAG       = get_tag(wc->tag);
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);
    rreq->status.MPI_ERROR     = MPI_SUCCESS;
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_iprobe_impl)
int ADD_SUFFIX(MPID_nem_ofi_iprobe_impl)(struct MPIDI_VC *vc,
                             int source,
                             int tag,
                             MPID_Comm * comm,
                             int context_offset,
                             int *flag, MPI_Status * status, MPID_Request ** rreq_ptr)
{
    int ret, mpi_errno = MPI_SUCCESS;
    fi_addr_t remote_proc = 0;
    uint64_t match_bits, mask_bits;
    size_t len;
    MPID_Request rreq_s, *rreq;

    BEGIN_FUNC(FCNAME);
    if (rreq_ptr) {
        MPIDI_Request_create_rreq(rreq, mpi_errno, goto fn_exit);
        *rreq_ptr = rreq;
        rreq->comm = comm;
        rreq->dev.match.parts.rank = source;
        rreq->dev.match.parts.tag = tag;
        rreq->dev.match.parts.context_id = comm->context_id;
        MPIR_Comm_add_ref(comm);
    }
    else {
        rreq = &rreq_s;
        rreq->dev.OnDataAvail = NULL;
    }
    REQ_OFI(rreq)->event_callback = ADD_SUFFIX(peek_callback);
    REQ_OFI(rreq)->match_state    = PEEK_INIT;
    OFI_ADDR_INIT(source, vc, remote_proc);
#if API_SET == API_SET_1
    match_bits = init_recvtag(&mask_bits, comm->context_id + context_offset, source, tag);
#elif API_SET == API_SET_2
    match_bits = init_recvtag_2(&mask_bits, comm->context_id + context_offset, tag);
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
    uint64_t     msgflags = FI_PEEK;
    msg.msg_iov   = NULL;
    msg.desc      = NULL;
    msg.iov_count = 0;
    msg.addr      = remote_proc;
    msg.tag       = match_bits;
    msg.ignore    = mask_bits;
    msg.context   = (void *) &(REQ_OFI(rreq)->ofi_context);
    msg.data      = 0;
    if(*flag == CLAIM_PEEK)
      msgflags|=FI_CLAIM;
    ret = fi_trecvmsg(gl_data.endpoint,&msg,msgflags);
    if(ret == -ENOMSG) {
      if (rreq_ptr) {
        MPID_Request_release(rreq);
        *rreq_ptr = NULL;
        *flag = 0;
      }
      MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
      goto fn_exit;
    }
    MPIR_ERR_CHKANDJUMP4((ret < 0), mpi_errno, MPI_ERR_OTHER,
                         "**ofi_peek", "**ofi_peek %s %d %s %s",
                         __SHORT_FILE__, __LINE__, FCNAME, fi_strerror(-ret));

    while (PEEK_INIT == REQ_OFI(rreq)->match_state)
        MPID_nem_ofi_poll(MPID_BLOCKING_POLL);
    *status = rreq->status;
    *flag = 1;
    END_FUNC_RC(FCNAME);
}


#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_iprobe)
int ADD_SUFFIX(MPID_nem_ofi_iprobe)(struct MPIDI_VC *vc,
                        int source,
                        int tag,
                        MPID_Comm * comm, int context_offset, int *flag, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = 0;
    rc = ADD_SUFFIX(MPID_nem_ofi_iprobe_impl)(vc, source,
                                              tag, comm, context_offset, flag, status, NULL);
    END_FUNC(FCNAME);
    return rc;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_improbe)
int ADD_SUFFIX(MPID_nem_ofi_improbe)(struct MPIDI_VC *vc,
                         int source,
                         int tag,
                         MPID_Comm * comm,
                         int context_offset,
                         int *flag, MPID_Request ** message, MPI_Status * status)
{
    int old_error = status->MPI_ERROR;
    int s;
    BEGIN_FUNC(FCNAME);
    *flag = NORMAL_PEEK;
    s = ADD_SUFFIX(MPID_nem_ofi_iprobe_impl)(vc, source,
                                             tag, comm, context_offset, flag, status, message);
    if (*flag) {
        status->MPI_ERROR = old_error;
        (*message)->kind = MPID_REQUEST_MPROBE;
    }
    END_FUNC(FCNAME);
    return s;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_iprobe)
int ADD_SUFFIX(MPID_nem_ofi_anysource_iprobe)(int tag,
                                  MPID_Comm * comm,
                                  int context_offset, int *flag, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = NORMAL_PEEK;
    rc = ADD_SUFFIX(MPID_nem_ofi_iprobe)(NULL, MPI_ANY_SOURCE,
                                         tag, comm, context_offset, flag, status);
    END_FUNC(FCNAME);
    return rc;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_improbe)
int ADD_SUFFIX(MPID_nem_ofi_anysource_improbe)(int tag,
                                   MPID_Comm * comm,
                                   int context_offset,
                                   int *flag, MPID_Request ** message, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = CLAIM_PEEK;
    rc = ADD_SUFFIX(MPID_nem_ofi_improbe)(NULL, MPI_ANY_SOURCE, tag, comm,
                              context_offset, flag, message, status);
    END_FUNC(FCNAME);
    return rc;
}
