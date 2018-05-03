/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if defined(HAVE_VMSPLICE)

/* must come first for now */
#define _GNU_SOURCE
#include <fcntl.h>
#include <sys/uio.h>

#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"


/* These are for maintaining a linked-list of outstanding requests on which we
   can make progress. */
struct lmt_vmsplice_node {
    struct lmt_vmsplice_node *next;
    int pipe_fd;
    MPIR_Request *req;
};

/* MT: this stack is not thread-safe */
static struct lmt_vmsplice_node *outstanding_head = NULL;

/* Returns true if the IOV has been completely xferred, false otherwise.
  
   iov_count and iov_offset are pointers so that this function can manipulate
   them */
static int adjust_partially_xferred_iov(MPL_IOV iov[], int *iov_offset,
                                        int *iov_count, int bytes_xferred)
{
    int i;
    int complete = 1;

    for (i = *iov_offset; i < (*iov_offset + *iov_count); ++i)
    {
        if (bytes_xferred < iov[i].MPL_IOV_LEN)
        {
            iov[i].MPL_IOV_BUF = (char *)iov[i].MPL_IOV_BUF + bytes_xferred;
            iov[i].MPL_IOV_LEN -= bytes_xferred;
            /* iov_count should be equal to the number of iov's remaining */
            *iov_count -= (i - *iov_offset);
            *iov_offset = i;
            complete = 0;
            break;
        }
        bytes_xferred -= iov[i].MPL_IOV_LEN;
    }

    return complete;
}

static inline int check_req_complete(MPIDI_VC_t *vc, MPIR_Request *req, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
    reqFn = req->dev.OnDataAvail;
    if (reqFn) {
        *complete = 0;

        /* XXX DJG FIXME this feels like a hack */
        req->dev.iov_count = MPL_IOV_LIMIT;
        req->dev.iov_offset = 0;

        mpi_errno = reqFn(vc, req, complete);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else {
        *complete = 1;
        mpi_errno = MPID_Request_complete(req);
        if (mpi_errno != MPI_SUCCESS) {
            MPIR_ERR_POP(mpi_errno);
        }
    }

fn_fail:
    return mpi_errno;
}

/* fills in req->dev.iov{,_offset,_count} based on the datatype info in the
   request, creating a segment if necessary */
static int populate_iov_from_req(MPIR_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    int dt_contig;
    MPI_Aint dt_true_lb;
    intptr_t data_sz;
    MPIR_Datatype* dt_ptr;

    /* find out contig/noncontig, size, and lb for the datatype */
    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig) {
        /* handle the iov creation ourselves */
        req->dev.iov[0].MPL_IOV_BUF = (char *)req->dev.user_buf + dt_true_lb;
        req->dev.iov[0].MPL_IOV_LEN = data_sz;
        req->dev.iov_count = 1;
    }
    else {
        /* use the segment routines to handle the iovec creation */
        MPIR_Assert(req->dev.segment_ptr == NULL);

        req->dev.iov_count = MPL_IOV_LIMIT;
        req->dev.iov_offset = 0;

        /* XXX DJG FIXME where is this segment freed? */
        req->dev.segment_ptr = MPIR_Segment_alloc();
        MPIR_ERR_CHKANDJUMP1((req->dev.segment_ptr == NULL), mpi_errno,
                             MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPIR_Segment_alloc");
        MPIR_Segment_init(req->dev.user_buf, req->dev.user_count,
                          req->dev.datatype, req->dev.segment_ptr);
        req->dev.segment_first = 0;
        req->dev.segment_size = data_sz;


        /* FIXME we should write our own function that isn't dependent on
           the in-request iov array.  This will let us use IOVs that are
           larger than MPL_IOV_LIMIT. */
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(req, &req->dev.iov[0],
                                                     &req->dev.iov_count);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

fn_fail:
    return mpi_errno;
}

static int do_vmsplice(MPIR_Request *sreq, int pipe_fd, MPL_IOV iov[],
                       int *iov_offset, int *iov_count, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t err;

#if 1
    err = vmsplice(pipe_fd, &iov[*iov_offset], *iov_count, SPLICE_F_NONBLOCK);
#else
    err = writev(pipe_fd, &iov[*iov_offset], *iov_count);
#endif

    if (err < 0) {
        if (errno == EAGAIN) goto fn_exit;
        MPIR_ERR_CHKANDJUMP2(errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**vmsplice",
                             "**vmsplice %d %s", errno, MPIR_Strerror(errno));
    }

    *complete = adjust_partially_xferred_iov(iov, iov_offset, iov_count, err);
    if (*complete) {
        /* look for additional data to send and reload IOV if there is more */
        mpi_errno = check_req_complete(sreq->ch.vc, sreq, complete);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (*complete) {
            err = close(pipe_fd);
            MPIR_ERR_CHKANDJUMP(err < 0, mpi_errno, MPI_ERR_OTHER, "**close");
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
        }
    }

fn_fail:
fn_exit:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_initiate_lmt
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPIR_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    int complete = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_INITIATE_LMT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_INITIATE_LMT);

    /* re-use the same pipe per-pair,per-sender */
    if (vc_ch->lmt_copy_buf_handle == NULL) {
        int err;
        char *pipe_name;
        MPIDI_CH3I_VC *vc_ch = &vc->ch;

        pipe_name = tempnam(NULL, "lmt_");
        MPIR_ERR_CHKANDJUMP2(!pipe_name, mpi_errno, MPI_ERR_OTHER, "**tempnam",
                             "**tempnam %d %s", errno, MPIR_Strerror(errno));

        vc_ch->lmt_copy_buf_handle = MPL_strdup(pipe_name);
        /* XXX DJG hack */
#undef free
        free(pipe_name);

        err = mkfifo(vc_ch->lmt_copy_buf_handle, 0660);
        MPIR_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**mkfifo",
                             "**mkfifo %d %s", errno, MPIR_Strerror(errno));
    }

    /* can't start sending data yet, need full RTS/CTS handshake */

    MPID_nem_lmt_send_RTS(vc, rts_pkt, vc_ch->lmt_copy_buf_handle,
                          strlen(vc_ch->lmt_copy_buf_handle)+1);

fn_fail:
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_INITIATE_LMT);
    return mpi_errno;
}

static int do_readv(MPIR_Request *rreq, int pipe_fd, MPL_IOV iov[],
                    int *iov_offset, int *iov_count, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    ssize_t nread;

    nread = readv(pipe_fd, &rreq->dev.iov[rreq->dev.iov_offset], rreq->dev.iov_count);
    MPIR_ERR_CHKANDJUMP2(nread < 0 && errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**read",
                         "**readv %d %s", errno, MPIR_Strerror(errno));

    if (nread < 0) {
        if (errno == EAGAIN) goto fn_exit;
        MPIR_ERR_CHKANDJUMP2(errno != EAGAIN, mpi_errno, MPI_ERR_OTHER, "**vmsplice",
                             "**vmsplice %d %s", errno, MPIR_Strerror(errno));
    }

    *complete = adjust_partially_xferred_iov(iov, iov_offset, iov_count, nread);
    if (*complete) {
        /* look for additional data to send and reload IOV if there is more */
        mpi_errno = check_req_complete(rreq->ch.vc, rreq, complete);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);

        if (*complete) {
            nread = close(pipe_fd);
            MPIR_ERR_CHKANDJUMP(nread < 0, mpi_errno, MPI_ERR_OTHER, "**close");
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");
        }
    }

fn_fail:
fn_exit:
    return mpi_errno;
}

/* This function is called when an RTS message comes in. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_start_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_start_recv(MPIDI_VC_t *vc, MPIR_Request *rreq, MPL_IOV s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    int complete = 0;
    struct lmt_vmsplice_node *node = NULL;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    int pipe_fd;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_RECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_RECV);

    if (vc_ch->lmt_recv_copy_buf_handle == NULL) {
        MPIR_Assert(s_cookie.MPL_IOV_BUF != NULL);
        vc_ch->lmt_recv_copy_buf_handle = MPL_strdup(s_cookie.MPL_IOV_BUF);
    }

    /* XXX DJG FIXME in a real version we would want to cache the fd on the vc
       so that we don't have two open's on the critical path every time. */
    pipe_fd = open(vc_ch->lmt_recv_copy_buf_handle, O_NONBLOCK|O_RDONLY);
    MPIR_ERR_CHKANDJUMP1(pipe_fd < 0, mpi_errno, MPI_ERR_OTHER, "**open",
                         "**open %s", MPIR_Strerror(errno));

    MPID_nem_lmt_send_CTS(vc, rreq, NULL, 0);

    mpi_errno = populate_iov_from_req(rreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    mpi_errno = do_readv(rreq, pipe_fd, rreq->dev.iov, &rreq->dev.iov_offset,
                         &rreq->dev.iov_count, &complete);

    /* push request if not complete for progress checks later */
    if (!complete) {
        node = MPL_malloc(sizeof(struct lmt_vmsplice_node), MPL_MEM_OTHER);
        node->pipe_fd = pipe_fd;
        node->req = rreq;
        node->next = outstanding_head;
        outstanding_head = node;
        ++MPID_nem_local_lmt_pending;
    }

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_RECV);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* XXX DJG FIXME at some point this should poll, much like the newtcp module.
   But then we have that whole pollfd array to manage, which we don't really
   need until this proof-of-concept proves itself. */
int MPID_nem_lmt_vmsplice_progress(void)
{
    int mpi_errno = MPI_SUCCESS;
    struct lmt_vmsplice_node *prev = NULL;
    struct lmt_vmsplice_node *free_me = NULL;
    struct lmt_vmsplice_node *cur = outstanding_head;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_PROGRESS);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_PROGRESS);
    
    while (cur) {
        int complete = 0;

        switch (MPIDI_Request_get_type(cur->req)) {
            case MPIDI_REQUEST_TYPE_RECV:
                mpi_errno = do_readv(cur->req, cur->pipe_fd, cur->req->dev.iov,
                                     &cur->req->dev.iov_offset,
                                     &cur->req->dev.iov_count, &complete);
                /* FIXME: set the error status of the req and complete it, rather than POP */
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            case MPIDI_REQUEST_TYPE_SEND:
                mpi_errno = do_vmsplice(cur->req, cur->pipe_fd, cur->req->dev.iov,
                                        &cur->req->dev.iov_offset,
                                        &cur->req->dev.iov_count, &complete);
                /* FIXME: set the error status of the req and complete it, rather than POP */
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                break;
            default:
                MPIR_ERR_INTERNALANDJUMP(mpi_errno, "unexpected request type");
                break;
        }

        if (complete) {
            MPL_DBG_MSG(MPIDI_CH3_DBG_CHANNEL, VERBOSE, ".... complete");

            /* remove the node from the list */
            if (cur == outstanding_head) {
                outstanding_head = cur->next;
                prev = NULL;
                free_me = cur;
                cur = cur->next;
            }
            else {
                prev->next = cur->next;
                prev = cur;
                free_me = cur;
                cur = cur->next;
            }
            if (free_me) MPL_free(free_me);
            --MPID_nem_local_lmt_pending;
        }

        if (!cur) break; /* we might have made cur NULL above */

        prev = cur;
        cur = cur->next;
    }

fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_PROGRESS);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* called when a CTS message is received */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_start_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_start_send(MPIDI_VC_t *vc, MPIR_Request *sreq, MPL_IOV r_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_SEND);
    int pipe_fd;
    int complete;
    struct lmt_vmsplice_node *node = NULL;
    int (*reqFn)(MPIDI_VC_t *, MPIR_Request *, int *);
    MPIDI_CH3I_VC *vc_ch = &vc->ch;

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_SEND);

    /* Must do this after the other side has opened for reading, otherwise we
       will error out with ENXIO.  This will be indicated by the receipt of a
       CTS message. */
    pipe_fd = open(vc_ch->lmt_copy_buf_handle, O_NONBLOCK|O_WRONLY);
    MPIR_ERR_CHKANDJUMP1(pipe_fd < 0, mpi_errno, MPI_ERR_OTHER, "**open",
                         "**open %s", MPIR_Strerror(errno));

    mpi_errno = populate_iov_from_req(sreq);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    /* send the first flight */
    sreq->ch.vc = vc; /* XXX DJG is this already assigned? */
    complete = 0;
    mpi_errno = do_vmsplice(sreq, pipe_fd, sreq->dev.iov,
                            &sreq->dev.iov_offset, &sreq->dev.iov_count, &complete);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    if (!complete) {
        /* push for later progress */
        node = MPL_malloc(sizeof(struct lmt_vmsplice_node), MPL_MEM_OTHER);
        node->pipe_fd = pipe_fd;
        node->req = sreq;
        node->next = outstanding_head;
        outstanding_head = node;
        ++MPID_nem_local_lmt_pending;
    }

fn_fail:
fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_START_SEND);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_MPID_nem_lmt_vmsplice_vc_terminated
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_MPID_nem_lmt_vmsplice_vc_terminated(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_CH3_MPID_NEM_LMT_VMSPLICE_VC_TERMINATED);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_CH3_MPID_NEM_LMT_VMSPLICE_VC_TERMINATED);

    /* FIXME: need to handle the case where a VC is terminated due to
       a process failure.  We need to remove any outstanding LMT ops
       for this VC. */

 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_CH3_MPID_NEM_LMT_VMSPLICE_VC_TERMINATED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* --------------------------------------------------------------------------
   The functions below are nops, stubs that might be used in later versions of
   this code.
   -------------------------------------------------------------------------- */

/* called when a DONE message is received for a receive request */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_done_recv
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_done_recv(MPIDI_VC_t *vc, MPIR_Request *rreq)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_RECV);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_RECV);

    /* nop */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_RECV);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_done_send
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_done_send(MPIDI_VC_t *vc, MPIR_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_SEND);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_SEND);

    /* nop */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_DONE_SEND);
    return MPI_SUCCESS;
}

/* called when a COOKIE message is received */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_vmsplice_handle_cookie
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_lmt_vmsplice_handle_cookie(MPIDI_VC_t *vc, MPIR_Request *req, MPL_IOV cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_LMT_VMSPLICE_HANDLE_COOKIE);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_LMT_VMSPLICE_HANDLE_COOKIE);

    /* nop */

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_LMT_VMSPLICE_HANDLE_COOKIE);
    return MPI_SUCCESS;
}

#endif
