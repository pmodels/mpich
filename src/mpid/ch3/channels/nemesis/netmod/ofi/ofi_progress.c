/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "ofi_impl.h"

#define NORMAL_PEEK    0
#define CLAIM_PEEK     1

/* ------------------------------------------------------------------------ */
/* This routine looks up the request that contains a context object         */
/* ------------------------------------------------------------------------ */
static inline MPID_Request *context_to_req(void *ofi_context)
{
    return (MPID_Request *) container_of(ofi_context, MPID_Request, ch.netmod_area.padding);
}

/* ------------------------------------------------------------------------ */
/* peek_callback called when a successful peek is completed                 */
/* ------------------------------------------------------------------------ */
#undef FCNAME
#define FCNAME DECL_FUNC(peek_callback)
static int peek_callback(cq_tagged_entry_t * wc, MPID_Request * rreq)
{
    int mpi_errno = MPI_SUCCESS;
    BEGIN_FUNC(FCNAME);
    REQ_OFI(rreq)->match_state = PEEK_FOUND;
    rreq->status.MPI_SOURCE    = get_source(wc->tag);
    rreq->status.MPI_TAG       = get_tag(wc->tag);
    MPIR_STATUS_SET_COUNT(rreq->status, wc->len);
    rreq->status.MPI_ERROR     = MPI_SUCCESS;
    END_FUNC(FCNAME);
    return mpi_errno;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_iprobe_impl)
int MPID_nem_ofi_iprobe_impl(struct MPIDI_VC *vc,
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
    REQ_OFI(rreq)->event_callback = peek_callback;
    REQ_OFI(rreq)->match_state    = PEEK_INIT;
    OFI_ADDR_INIT(source, vc, remote_proc);
    match_bits = init_recvtag(&mask_bits, comm->context_id + context_offset, source, tag);

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
        MPIDI_CH3_Request_destroy(rreq);
        *rreq_ptr = NULL;
      }
      MPID_nem_ofi_poll(MPID_NONBLOCKING_POLL);
      *flag = 0;
      goto fn_exit;
    }
    MPIU_ERR_CHKANDJUMP4((ret < 0), mpi_errno, MPI_ERR_OTHER,
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
int MPID_nem_ofi_iprobe(struct MPIDI_VC *vc,
                        int source,
                        int tag,
                        MPID_Comm * comm, int context_offset, int *flag, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = 0;
    rc = MPID_nem_ofi_iprobe_impl(vc, source, tag, comm, context_offset, flag, status, NULL);
    END_FUNC(FCNAME);
    return rc;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_improbe)
int MPID_nem_ofi_improbe(struct MPIDI_VC *vc,
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
    s = MPID_nem_ofi_iprobe_impl(vc, source, tag, comm, context_offset, flag, status, message);
    if (*flag) {
        status->MPI_ERROR = old_error;
        (*message)->kind = MPID_REQUEST_MPROBE;
    }
    END_FUNC(FCNAME);
    return s;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_iprobe)
int MPID_nem_ofi_anysource_iprobe(int tag,
                                  MPID_Comm * comm,
                                  int context_offset, int *flag, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = NORMAL_PEEK;
    rc = MPID_nem_ofi_iprobe(NULL, MPI_ANY_SOURCE, tag, comm, context_offset, flag, status);
    END_FUNC(FCNAME);
    return rc;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_anysource_improbe)
int MPID_nem_ofi_anysource_improbe(int tag,
                                   MPID_Comm * comm,
                                   int context_offset,
                                   int *flag, MPID_Request ** message, MPI_Status * status)
{
    int rc;
    BEGIN_FUNC(FCNAME);
    *flag = CLAIM_PEEK;
    rc = MPID_nem_ofi_improbe(NULL, MPI_ANY_SOURCE, tag, comm,
                              context_offset, flag, message, status);
    END_FUNC(FCNAME);
    return rc;
}

#undef FCNAME
#define FCNAME DECL_FUNC(MPID_nem_ofi_poll)
int MPID_nem_ofi_poll(int in_blocking_poll)
{
    int complete = 0, mpi_errno = MPI_SUCCESS;
    ssize_t ret;
    cq_tagged_entry_t wc;
    cq_err_entry_t error;
    MPIDI_VC_t *vc;
    MPID_Request *req;
    req_fn reqFn;
    BEGIN_FUNC(FCNAME);
    do {
        /* ----------------------------------------------------- */
        /* Poll the completion queue                             */
        /* The strategy here is                                  */
        /* ret>0 successfull poll, events returned               */
        /* ret==0 empty poll, no events/no error                 */
        /* ret<0, error, but some error instances should not     */
        /* cause MPI to terminate                                */
        /* ----------------------------------------------------- */
        ret = fi_cq_read(gl_data.cq,    /* Tagged completion queue       */
                         (void *) &wc,  /* OUT:  Tagged completion entry */
                         1);    /* Number of entries to poll     */
        if (ret > 0) {
            if (NULL != wc.op_context) {
                req = context_to_req(wc.op_context);
                if (REQ_OFI(req)->event_callback) {
                    MPI_RC(REQ_OFI(req)->event_callback(&wc, req));
                    continue;
                }
                reqFn = req->dev.OnDataAvail;
                if (reqFn) {
                    if (REQ_OFI(req)->pack_buffer) {
                        MPIU_Free(REQ_OFI(req)->pack_buffer);
                    }
                    vc = REQ_OFI(req)->vc;

                    complete = 0;
                    MPI_RC(reqFn(vc, req, &complete));
                    continue;
                }
                else {
                    MPIU_Assert(0);
                }
            }
            else {
                MPIU_Assert(0);
            }
        }
        else if (ret == -FI_EAGAIN)
          ;
        else if (ret < 0) {
            if (ret == -FI_EAVAIL) {
                ret = fi_cq_readerr(gl_data.cq, (void *) &error, 0);
                if (error.err == FI_EMSGSIZE) {
                    /* ----------------------------------------------------- */
                    /* This error message should only be delivered on send   */
                    /* events.  We want to ignore truncation errors          */
                    /* on the sender side, but complete the request anyway   */
                    /* Other kinds of requests, this is fatal.               */
                    /* ----------------------------------------------------- */
                    req = context_to_req(error.op_context);
                    if (req->kind == MPID_REQUEST_SEND) {
                        mpi_errno = REQ_OFI(req)->event_callback(NULL, req);
                    }
                    else if (req->kind == MPID_REQUEST_RECV) {
                        mpi_errno = REQ_OFI(req)->event_callback(&wc, req);
                        req->status.MPI_ERROR = MPI_ERR_TRUNCATE;
                        req->status.MPI_TAG = error.tag;
                    }
                    else {
                        mpi_errno = MPI_ERR_OTHER;
                    }
                }
            }
            else {
                MPIU_ERR_CHKANDJUMP4(1, mpi_errno, MPI_ERR_OTHER, "**ofi_poll",
                                     "**ofi_poll %s %d %s %s", __SHORT_FILE__,
                                     __LINE__, FCNAME, fi_strerror(-ret));
            }
        }
    } while (in_blocking_poll && (ret > 0));
    END_FUNC_RC(FCNAME);
}
