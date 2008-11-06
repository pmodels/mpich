#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"

/* TODO there might be a better way to do this, the current system will lead to
   an unresolved symbols link error if dma is chosen and knem_io.h can't be
   found. */
#if defined(HAVE_KNEM_IO_H)

#include "knem_io.h"

/* FIXME this should be moved into the knem_io.h header */
typedef uint64_t knem_cookie_t;

static int knem_fd = -1;

/* Opens the knem device and sets knem_fd accordingly.  Uses mpich2 errhandling
   conventions. */
static int open_knem_dev()
{
    int mpi_errno = MPI_SUCCESS;
    int err;
    uint32_t abi = -1;

    knem_fd = open(KNEM_DEVICE_FILENAME, O_RDWR);
    MPIU_ERR_CHKANDJUMP2(knem_fd < 0, mpi_errno, MPI_ERR_OTHER, "**shm_open",
                         "**shm_open %s %d", KNEM_DEVICE_FILENAME, errno);
    err = ioctl(knem_fd, KNEM_CMD_GET_ABI_VERSION, &abi);
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));
    MPIU_ERR_CHKANDJUMP2(abi != KNEM_ABI_VERSION, mpi_errno, MPI_ERR_OTHER,
                         "**abi_version_mismatch", "**abi_version_mismatch %D %D",
                         (unsigned long)KNEM_ABI_VERSION, (unsigned long)abi);
fn_fail:
    return mpi_errno;
}

/* Sends as much data from the request as possible via the knem ioctl.
   s_cookiep is an output parameter */
static int do_dma_send(MPIDI_VC_t *vc,  MPID_Request *sreq, int send_iov_n,
                       MPID_IOV send_iov[], knem_cookie_t *s_cookiep)
{
    int mpi_errno = MPI_SUCCESS;
    int i, err;
    struct knem_cmd_init_large_send_param sendcmd = {0};
    struct knem_cmd_param_iovec knem_iov[MPID_IOV_LIMIT];

    /* FIXME The knem module iovec is potentially different from the system
       iovec.  This causes all sorts of fun if you don't realize it and use the
       system iovec directly instead.  Eventually we need to either unify them
       or avoid this extra copy. */
    for (i = 0; i < send_iov_n; ++i) {
        knem_iov[i].base = (uintptr_t)send_iov[i] .MPID_IOV_BUF;
        knem_iov[i].len  = send_iov[i] .MPID_IOV_LEN;
    }

    sendcmd.send_iovec_array = (uintptr_t) &knem_iov[0];
    sendcmd.send_iovec_nr = send_iov_n;
    err = ioctl(knem_fd, KNEM_CMD_INIT_LARGE_SEND, &sendcmd);
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));
    *s_cookiep = sendcmd.send_cookie;

fn_fail:
fn_exit:
    return mpi_errno;
}

/* s_cookie is an input parameter */
static int do_dma_recv(int iov_n, MPID_IOV iov[], knem_cookie_t s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int i, err;
    struct knem_cmd_init_large_recv_param recvcmd = {0};
    struct knem_cmd_param_iovec knem_iov[MPID_IOV_LIMIT];

    /* FIXME The knem module iovec is potentially different from the system
       iovec.  This causes all sorts of fun if you don't realize it and use the
       system iovec directly instead.  Eventually we need to either unify them
       or avoid this extra copy. */
    for (i = 0; i < iov_n; ++i) {
        knem_iov[i].base = (uintptr_t)iov[i] .MPID_IOV_BUF;
        knem_iov[i].len  = iov[i] .MPID_IOV_LEN;
    }

    recvcmd.recv_iovec_array = (uintptr_t) &knem_iov[0];
    recvcmd.recv_iovec_nr = iov_n;
    recvcmd.send_cookie = s_cookie;

    err = ioctl(knem_fd, KNEM_CMD_INIT_LARGE_RECV, &recvcmd);
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Much like initiate_lmt except it won't send an RTS message.  Used to
   implement initiate_lmt and handle_cookie.  This will send as much data from
   the request in a single shot as possible.
   
   s_cookiep is an output parameter. */
static int send_sreq_data(MPIDI_VC_t *vc, MPID_Request *sreq, knem_cookie_t *s_cookiep)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_IOV send_iov[MPID_IOV_LIMIT];
    int send_iov_n = MPID_IOV_LIMIT;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype * dt_ptr;

    /* MT: this code assumes only one thread can be at this point at a time */
    if (knem_fd < 0) {
        mpi_errno = open_knem_dev();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* find out contig/noncontig, size, and lb for the datatype */
    MPIDI_Datatype_get_info(sreq->dev.user_count, sreq->dev.datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig) {
        /* handle the iov creation ourselves */
        send_iov[0].MPID_IOV_BUF = (char *)sreq->dev.user_buf + dt_true_lb;
        send_iov[0].MPID_IOV_LEN = data_sz;
        send_iov_n = 1;
    }
    else {
        /* use the segment routines to handle the iovec creation */
        if (sreq->dev.segment_ptr == NULL) {
            /* segment_ptr may be non-null when this is a continuation of a
               many-part message that we couldn't fit in one single flight of
               iovs. */
            sreq->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1((sreq->dev.segment_ptr == NULL), mpi_errno,
                                 MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPID_Segment_alloc");
            MPID_Segment_init(sreq->dev.user_buf, sreq->dev.user_count,
                              sreq->dev.datatype, sreq->dev.segment_ptr, 0);
            sreq->dev.segment_first = 0;
            sreq->dev.segment_size = data_sz;
        }

        /* See the FIXME in the start_recv function about the load_recv_iov
           function.  The same info applies here. */
        mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &send_iov[0], &send_iov_n);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    mpi_errno = do_dma_send(vc, sreq, send_iov_n, send_iov, s_cookiep);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_initiate_lmt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    knem_cookie_t s_cookie;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);

    mpi_errno = send_sreq_data(vc, sreq, &s_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_nem_lmt_send_RTS(vc, rts_pkt, &s_cookie, sizeof(s_cookie));

fn_fail:
fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);
    return mpi_errno;
}

/* This function is called initially when an RTS message comes in, but may also
   be called by the COOKIE handler in the non-contiguous case to process
   additional IOVs. */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_start_recv(MPIDI_VC_t *vc, MPID_Request *rreq, MPID_IOV s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int i, err;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype * dt_ptr;
    int complete = 0;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_START_RECV);

    /* MT: this code assumes only one thread can be at this point at a time */
    if (knem_fd < 0) {
        mpi_errno = open_knem_dev();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    /* find out contig/noncontig, size, and lb for the datatype */
    MPIDI_Datatype_get_info(rreq->dev.user_count, rreq->dev.datatype,
                            dt_contig, data_sz, dt_ptr, dt_true_lb);

    if (dt_contig) {
        /* handle the iov creation ourselves */
        rreq->dev.iov[0].MPID_IOV_BUF = (char *)rreq->dev.user_buf + dt_true_lb;
        rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
        rreq->dev.iov_count = 1;
    }
    else {
        /* use the segment routines to handle the iovec creation */
        MPIU_Assert(rreq->dev.segment_ptr == NULL);
        rreq->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno,
                             MPI_ERR_OTHER, "**nomem",
                             "**nomem %s", "MPID_Segment_alloc");
        MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count,
                          rreq->dev.datatype, rreq->dev.segment_ptr, 0);
        rreq->dev.segment_first = 0;
        rreq->dev.segment_size = data_sz;

        mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

    MPIU_Assert(s_cookie.MPID_IOV_LEN == sizeof(knem_cookie_t));
    MPIU_Assert(s_cookie.MPID_IOV_BUF != NULL);
    mpi_errno = do_dma_recv(rreq->dev.iov_count, rreq->dev.iov,
                            *((knem_cookie_t *)s_cookie.MPID_IOV_BUF));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* At this point, the transfer is complete with the current module
       implementation.  After the proof of concept stage we'll change it to use
       a scheme that polls a memory address.  Note that we still need to handle
       the case where the message required greater than MPID_IOV_LIMIT iovs. */
    reqFn = rreq->dev.OnDataAvail;
    if (!reqFn) {
        MPIDI_CH3U_Request_complete(rreq);
        MPID_nem_lmt_send_DONE(vc, rreq); /* tell the other side to complete its request */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        goto fn_exit;
    }

    complete = 0;
    mpi_errno = reqFn(vc, rreq, &complete);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
    if (complete) {
        /* request was completed by the OnDataAvail fn */
        MPID_nem_lmt_send_DONE(vc, rreq); /* tell the other side to complete its request */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        goto fn_exit;
    }
    else {
        /* There is more data to send.  We must inform the sender that we have
           completely received the current batch and that the next batch should
           be sent. */
        MPID_nem_lmt_send_COOKIE(vc, rreq, NULL, 0);
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_START_RECV);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_done_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_done_send(MPIDI_VC_t *vc, MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    int complete = 0;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_DONE_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_DONE_SEND);

    /* We shouldn't ever need to handle the more IOVs case here.  The DONE
       message should only be sent when all of the data is truly transferred.
       However in the interest of robustness, we'll start to handle it and
       assert if it looks like we were supposed to send more data for some
       reason. */
    reqFn = sreq->dev.OnDataAvail;
    if (!reqFn) {
        MPIDI_CH3U_Request_complete(sreq);
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        goto fn_exit;
    }

    complete = 0;
    mpi_errno = reqFn(vc, sreq, &complete);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        
    if (complete) {
        /* request was completed by the OnDataAvail fn */
        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");
        goto fn_exit;
    }
    else {
        /* There is more data to send.  We must inform the sender that we have
           completely received the current batch and that the next batch should
           be sent. */
        MPIU_Assert(("should never be incomplete!", 0));
    }

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_DONE_SEND);
fn_exit:
    return MPI_SUCCESS;
fn_fail:
    goto fn_exit;
}

/* called when a COOKIE message is received */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_handle_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_HANDLE_COOKIE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_HANDLE_COOKIE);

    if (cookie.MPID_IOV_LEN == 0 && cookie.MPID_IOV_BUF == NULL) {
        /* req is a send request, we need to initiate another knem request and
           send a COOKIE message back to the receiver indicating the lid
           returned from the ioctl. */
        knem_cookie_t s_cookie;
        /* the data was already reloaded by the OnDataAvail function, just do
           the dma send directly */
        mpi_errno = do_dma_send(vc, req, req->dev.iov_count, &req->dev.iov[0], &s_cookie);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        MPID_nem_lmt_send_COOKIE(vc, req, &s_cookie, sizeof(knem_cookie_t));
    }
    else {
        /* req is a receive request and we need to continue receiving using the
           lid provided in the cookie iov. */
        mpi_errno = MPID_nem_lmt_dma_start_recv(vc, req, cookie);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }

fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_HANDLE_COOKIE);
    return MPI_SUCCESS;
}


/* --------------------------------------------------------------------------
   The functions below are nops, stubs that might be used in later versions of
   this code.  Some set of TBD progress functions will need to be implemented
   as well once the recv ioctl is non-blocking.
   -------------------------------------------------------------------------- */

/* called when a CTS message is received */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_start_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_START_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_START_SEND);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_START_SEND);
    return mpi_errno;
}

/* called when a DONE message is received for a receive request */
#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_done_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_done_recv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_DONE_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_DONE_RECV);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_DONE_RECV);
    return MPI_SUCCESS;
}

#endif
