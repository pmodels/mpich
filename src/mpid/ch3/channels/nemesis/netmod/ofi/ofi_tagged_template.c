/*
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#if (API_SET != API_SET_1) && (API_SET != API_SET_2)
#error Undefined API SET
#endif


/* ------------------------------------------------------------------------ */
/* Receive done callback                                                    */
/* Handle an incoming receive completion event                              */
/* ------------------------------------------------------------------------ */
static inline int
ADD_SUFFIX(do_isend)(struct MPIDI_VC *vc,
         const void *buf,
         MPI_Aint count,
         MPI_Datatype datatype,
         int dest,
         int tag,
         MPIR_Comm * comm,
         int context_offset,
         struct MPIR_Request **request,
         int should_create_req,
         uint64_t send_type)
{
    int dt_contig, mpi_errno = MPI_SUCCESS;
    MPI_Aint dt_true_lb;
    intptr_t data_sz;
    MPIR_Datatype *dt_ptr;
    MPIR_BEGIN_FUNC_VERBOSE(__func__);

    VC_READY_CHECK(vc);
    *request = NULL;

    MPIDI_Datatype_get_info(count, datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (likely((send_type != MPID_SYNC_SEND) &&
                dt_contig &&
                (data_sz <= gl_data.max_buffered_send)))
    {
        if (should_create_req == MPID_CREATE_REQ)
            MPID_nem_ofi_create_req_lw(request, 1);

        mpi_errno = ADD_SUFFIX(send_lightweight)(vc, (char *) buf + dt_true_lb, data_sz,
                                                 dest, tag, comm, context_offset);
    }
    else
        mpi_errno = ADD_SUFFIX(send_normal)(vc, buf, count, datatype, dest, tag, comm,
                                context_offset, request, dt_contig,
                                data_sz, dt_ptr, dt_true_lb, send_type);

    MPIR_END_FUNC_VERBOSE_RC(MPID_STATE_DO_ISEND);
}

int ADD_SUFFIX(MPID_nem_ofi_send)(struct MPIDI_VC *vc,
                      const void *buf,
                      MPI_Aint count,
                      MPI_Datatype datatype,
                      int dest,
                      int tag, MPIR_Comm * comm, int context_offset, struct MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_BEGIN_FUNC_VERBOSE(__func__);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest, tag,
                         comm, context_offset, request, MPID_DONT_CREATE_REQ, MPID_NORMAL_SEND);
    MPIR_END_FUNC_VERBOSE(__func__);
    return mpi_errno;
}

int ADD_SUFFIX(MPID_nem_ofi_isend)(struct MPIDI_VC *vc,
                       const void *buf,
                       MPI_Aint count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPIR_Comm * comm, int context_offset, struct MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_BEGIN_FUNC_VERBOSE(__func__);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_CREATE_REQ, MPID_NORMAL_SEND);
    MPIR_END_FUNC_VERBOSE(__func__);
    return mpi_errno;
}

int ADD_SUFFIX(MPID_nem_ofi_ssend)(struct MPIDI_VC *vc,
                       const void *buf,
                       MPI_Aint count,
                       MPI_Datatype datatype,
                       int dest,
                       int tag, MPIR_Comm * comm, int context_offset, struct MPIR_Request **request)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_BEGIN_FUNC_VERBOSE(__func__);
    mpi_errno = ADD_SUFFIX(do_isend)(vc, buf, count, datatype, dest,
                         tag, comm, context_offset, request, MPID_CREATE_REQ, MPID_SYNC_SEND);
    MPIR_END_FUNC_VERBOSE(__func__);
    return mpi_errno;
}

int ADD_SUFFIX(MPID_nem_ofi_recv_posted)(struct MPIDI_VC *vc, struct MPIR_Request *rreq)
{
    int mpi_errno = MPI_SUCCESS, dt_contig, src, tag;
    uint64_t match_bits = 0, mask_bits = 0;
    fi_addr_t remote_proc = 0;
    intptr_t data_sz;
    MPI_Aint dt_true_lb;
    MPIR_Datatype*dt_ptr;
    MPIR_Context_id_t context_id;
    char *recv_buffer;
    MPIR_BEGIN_FUNC_VERBOSE(__func__);

    /* ------------------------ */
    /* Initialize the request   */
    /* ------------------------ */
    if (REQ_OFI(rreq)->match_state != PEEK_FOUND)
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
        recv_buffer = (char *) MPL_malloc(data_sz, MPL_MEM_BUFFER);
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
    if (REQ_OFI(rreq)->match_state == PEEK_FOUND) {
        msgflags = FI_CLAIM;
        REQ_OFI(rreq)->match_state = PEEK_INIT;
    }
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
    MPIR_END_FUNC_VERBOSE_RC(__func__);
}


void ADD_SUFFIX(MPID_nem_ofi_anysource_posted)(MPIR_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_BEGIN_FUNC_VERBOSE(__func__);
    mpi_errno = ADD_SUFFIX(MPID_nem_ofi_recv_posted)(NULL, rreq);
    MPIR_Assert(mpi_errno == MPI_SUCCESS);
    MPIR_END_FUNC_VERBOSE(__func__);
}
