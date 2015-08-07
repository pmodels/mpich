#if (API_SET != API_SET_1) && (API_SET != API_SET_2)
#error Undefined API SET
#endif


/* ------------------------------------------------------------------------ */
/* Receive done callback                                                    */
/* Handle an incoming receive completion event                              */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_recv_callback)
static inline
int ADD_SUFFIX(MPID_nem_ofi_recv_callback)(cq_tagged_entry_t * wc, MPID_Request * rreq)
{
    int err0, err1, src, mpi_errno = MPI_SUCCESS;
    uint64_t ssend_bits;
    MPIDI_msg_sz_t sz;
    MPIDI_VC_t *vc;
    MPID_Request *sync_req;
    BEGIN_FUNC(FCNAME);
    /* ---------------------------------------------------- */
    /* Populate the MPI Status and unpack noncontig buffer  */
    /* ---------------------------------------------------- */
    rreq->status.MPI_ERROR = MPI_SUCCESS;
#if API_SET == API_SET_1
    rreq->status.MPI_SOURCE = get_source(wc->tag);
#elif API_SET == API_SET_2
    rreq->status.MPI_SOURCE = wc->data;
#endif
    src = rreq->status.MPI_SOURCE;
    rreq->status.MPI_TAG = get_tag(wc->tag);

    REQ_OFI(rreq)->req_started = 1;
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);

    if (REQ_OFI(rreq)->pack_buffer) {
        MPIDI_CH3U_Buffer_copy(REQ_OFI(rreq)->pack_buffer,
                               MPIR_STATUS_GET_COUNT(rreq->status),
                               MPI_BYTE, &err0, rreq->dev.user_buf,
                               rreq->dev.user_count, rreq->dev.datatype, &sz, &err1);
        MPIR_STATUS_SET_COUNT(rreq->status, sz);
        MPIU_Free(REQ_OFI(rreq)->pack_buffer);
        if (err0 || err1) {
            rreq->status.MPI_ERROR = MPI_ERR_TYPE;
        }
    }

    if ((wc->tag & MPID_PROTOCOL_MASK) == MPID_SYNC_SEND) {
        /* ---------------------------------------------------- */
        /* Ack the sync send and wait for the send request      */
        /* completion(when callback executed.  A protocol bit   */
        /* MPID_SYNC_SEND_ACK is set in the tag bits to provide */
        /* separation of MPI messages and protocol messages     */
        /* ---------------------------------------------------- */
        vc = REQ_OFI(rreq)->vc;
        if (!vc) {      /* MPI_ANY_SOURCE -- Post message from status, complete the VC */
            vc = rreq->comm->dev.vcrt->vcr_table[src];
            MPIU_Assert(vc);
        }
#if API_SET == API_SET_1
        ssend_bits = init_sendtag(rreq->dev.match.parts.context_id,
                                  rreq->comm->rank, rreq->status.MPI_TAG, MPID_SYNC_SEND_ACK);
#elif API_SET == API_SET_2
        ssend_bits = init_sendtag_2(rreq->dev.match.parts.context_id,
                                    rreq->status.MPI_TAG, MPID_SYNC_SEND_ACK);
#endif
        MPID_nem_ofi_create_req(&sync_req, 1);
        sync_req->dev.OnDataAvail = NULL;
        sync_req->dev.next = NULL;
        REQ_OFI(sync_req)->event_callback = MPID_nem_ofi_sync_recv_callback;
        REQ_OFI(sync_req)->parent = rreq;
#if API_SET == API_SET_1
        FI_RC_RETRY(fi_tsend(gl_data.endpoint,
#elif API_SET == API_SET_2
        FI_RC_RETRY(fi_tsenddata(gl_data.endpoint,
#endif
                         NULL,
                         0,
                         gl_data.mr,
#if API_SET == API_SET_2
                         rreq->comm->rank,
#endif
                         VC_OFI(vc)->direct_addr,
                         ssend_bits, &(REQ_OFI(sync_req)->ofi_context)), tsend);
    }
    else {
        /* ---------------------------------------------------- */
        /* Non-syncronous send, complete normally               */
        /* by removing from the CH3 queue and completing the    */
        /* request object                                       */
        /* ---------------------------------------------------- */
        MPIDI_CH3U_Recvq_DP(rreq);
        MPIDI_CH3I_NM_OFI_RC(MPID_Request_complete(rreq));
    }
    END_FUNC_RC(FCNAME);
}


#undef FCNAME
#define FCNAME DECL_FUNC(do_isend)
static inline int
ADD_SUFFIX(do_isend)(struct MPIDI_VC *vc,
         const void *buf,
         int count,
         MPI_Datatype datatype,
         int dest,
         int tag,
         MPID_Comm * comm,
         int context_offset, struct MPID_Request **request, uint64_t type)
{
    int err0, err1, dt_contig, mpi_errno = MPI_SUCCESS;
    char *send_buffer;
    uint64_t match_bits, ssend_match, ssend_mask;
    MPI_Aint dt_true_lb;
    MPID_Request *sreq = NULL, *sync_req = NULL;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype *dt_ptr;
    BEGIN_FUNC(FCNAME);
    VC_READY_CHECK(vc);

    /* ---------------------------------------------------- */
    /* Create the MPI request                               */
    /* ---------------------------------------------------- */
    MPID_nem_ofi_create_req(&sreq, 2);
    sreq->kind = MPID_REQUEST_SEND;
    sreq->dev.OnDataAvail = NULL;
    REQ_OFI(sreq)->event_callback = MPID_nem_ofi_send_callback;
    REQ_OFI(sreq)->vc = vc;

    /* ---------------------------------------------------- */
    /* Create the pack buffer (if required), and allocate   */
    /* a send request                                       */
    /* ---------------------------------------------------- */
#if API_SET == API_SET_1
    match_bits = init_sendtag(comm->context_id + context_offset, comm->rank, tag, type);
#elif API_SET == API_SET_2
    match_bits = init_sendtag_2(comm->context_id + context_offset, tag, type);
#endif

    sreq->dev.match.parts.tag = match_bits;
    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    send_buffer = (char *) buf + dt_true_lb;
    if (!dt_contig) {
        send_buffer = (char *) MPIU_Malloc(data_sz);
        MPIR_ERR_CHKANDJUMP1(send_buffer == NULL, mpi_errno,
                             MPI_ERR_OTHER, "**nomem", "**nomem %s", "Send buffer alloc");
        MPIDI_CH3U_Buffer_copy(buf, count, datatype, &err0,
                               send_buffer, data_sz, MPI_BYTE, &data_sz, &err1);
        REQ_OFI(sreq)->pack_buffer = send_buffer;
    }

    if (type == MPID_SYNC_SEND) {
        /* ---------------------------------------------------- */
        /* For syncronous send, we post a receive to catch the  */
        /* match ack, but use the tag protocol bits to avoid    */
        /* matching with MPI level messages.                    */
        /* ---------------------------------------------------- */
        int c = 1;
        MPID_cc_incr(sreq->cc_ptr, &c);
        MPID_nem_ofi_create_req(&sync_req, 1);
        sync_req->dev.OnDataAvail = NULL;
        sync_req->dev.next = NULL;
        REQ_OFI(sync_req)->event_callback = MPID_nem_ofi_sync_recv_callback;
        REQ_OFI(sync_req)->parent = sreq;
#if API_SET == API_SET_1
        ssend_match = init_recvtag(&ssend_mask, comm->context_id + context_offset, dest, tag);
#elif API_SET == API_SET_2
        ssend_match = init_recvtag_2(&ssend_mask, comm->context_id + context_offset, tag);
#endif
        ssend_match |= MPID_SYNC_SEND_ACK;
        FI_RC_RETRY(fi_trecv(gl_data.endpoint,    /* endpoint    */
                           NULL,        /* recvbuf     */
                           0,   /* data sz     */
                           gl_data.mr,  /* dynamic mr  */
                           VC_OFI(vc)->direct_addr,     /* remote proc */
                           ssend_match, /* match bits  */
                           0ULL,        /* mask bits   */
                           &(REQ_OFI(sync_req)->ofi_context)), trecv);
    }
#if API_SET == API_SET_1
    FI_RC_RETRY(fi_tsend(gl_data.endpoint,  /* Endpoint                       */
#elif API_SET == API_SET_2
    FI_RC_RETRY(fi_tsenddata(gl_data.endpoint,  /* Endpoint                       */
#endif
        send_buffer,       /* Send buffer(packed or user)    */
        data_sz,   /* Size of the send               */
        gl_data.mr,        /* Dynamic memory region          */
#if API_SET == API_SET_2
        comm->rank,
#endif
        VC_OFI(vc)->direct_addr,   /* Use the address of this VC     */
        match_bits,        /* Match bits                     */
        &(REQ_OFI(sreq)->ofi_context)), tsend);
    *request = sreq;
    END_FUNC_RC(FCNAME);
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_send)
int ADD_SUFFIX(MPID_nem_ofi_send)(struct MPIDI_VC *vc,
                      const void *buf,
                      int count,
                      MPI_Datatype datatype,
                      int dest,
                      int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    BEGIN_FUNC(FCNAME);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest, tag,
                         comm, context_offset, request, MPID_NORMAL_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_isend)
int ADD_SUFFIX(MPID_nem_ofi_isend)(struct MPIDI_VC *vc,
                       const void *buf,
                       int count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_NORMAL_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_ssend)
int ADD_SUFFIX(MPID_nem_ofi_ssend)(struct MPIDI_VC *vc,
                       const void *buf,
                       int count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_SYNC_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_issend)
int ADD_SUFFIX(MPID_nem_ofi_issend)(struct MPIDI_VC *vc,
                        const void *buf,
                        int count,
                        MPI_Datatype datatype,
                        int dest,
                        int tag,
                        MPID_Comm * comm, int context_offset, struct MPID_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = do_isend(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_SYNC_SEND);
    END_FUNC(FCNAME);
    return mpi_errno;
}


#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_recv_posted)
int ADD_SUFFIX(MPID_nem_ofi_recv_posted)(struct MPIDI_VC *vc, struct MPID_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS, dt_contig, src, tag;
    uint64_t match_bits = 0, mask_bits = 0;
    fi_addr_t remote_proc = 0;
    MPIDI_msg_sz_t data_sz;
    MPI_Aint dt_true_lb;
    MPID_Datatype *dt_ptr;
    MPIU_Context_id_t context_id;
    char *recv_buffer;
    BEGIN_FUNC(FCNAME);

    /* ------------------------ */
    /* Initialize the request   */
    /* ------------------------ */
    MPID_nem_ofi_init_req(rreq);
    REQ_OFI(rreq)->event_callback = ADD_SUFFIX(MPID_nem_ofi_recv_callback);
    REQ_OFI(rreq)->vc = vc;

    /* ---------------------------------------------------- */
    /* Fill out the match info, and allocate the pack buffer */
    /* a send request                                       */
    /* ---------------------------------------------------- */
    src = rreq->dev.match.parts.rank;
    tag = rreq->dev.match.parts.tag;
    context_id = rreq->dev.match.parts.context_id;
#if API_SET == API_SET_1
    match_bits = init_recvtag(&mask_bits, context_id, src, tag);
#elif API_SET == API_SET_2
    match_bits = init_recvtag_2(&mask_bits, context_id, tag);
#endif
    OFI_ADDR_INIT(src, vc, remote_proc);
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);
    if (dt_contig) {
        recv_buffer = (char *) rreq->dev.user_buf + dt_true_lb;
    }
    else {
        recv_buffer = (char *) MPIU_Malloc(data_sz);
        MPIR_ERR_CHKANDJUMP1(recv_buffer == NULL, mpi_errno, MPI_ERR_OTHER,
                             "**nomem", "**nomem %s", "Recv Pack Buffer alloc");
        REQ_OFI(rreq)->pack_buffer = recv_buffer;
    }

    /* ---------------- */
    /* Post the receive */
    /* ---------------- */
    uint64_t     msgflags;
    iovec_t      iov;
    msg_tagged_t msg;
    iov.iov_base = recv_buffer;
    iov.iov_len  = data_sz;
    if(REQ_OFI(rreq)->match_state == PEEK_FOUND)
      msgflags = FI_CLAIM;
    else
      msgflags = 0ULL;

    msg.msg_iov   = &iov;
    msg.desc      = NULL;
    msg.iov_count = 1;
    msg.addr      = remote_proc;
    msg.tag       = match_bits;
    msg.ignore    = mask_bits;
    msg.context   = (void *) &(REQ_OFI(rreq)->ofi_context);
    msg.data      = 0;
    FI_RC_RETRY(fi_trecvmsg(gl_data.endpoint,&msg,msgflags), trecv);
    END_FUNC_RC(FCNAME);
}


#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_posted)
void ADD_SUFFIX(MPID_nem_ofi_anysource_posted)(MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    mpi_errno = ADD_SUFFIX(MPID_nem_ofi_recv_posted)(NULL, rreq);
    MPIU_Assert(mpi_errno == MPI_SUCCESS);
    END_FUNC(FCNAME);
}
