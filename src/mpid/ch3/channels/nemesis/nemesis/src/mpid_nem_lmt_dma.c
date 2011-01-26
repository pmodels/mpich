#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"

MPIU_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if defined(HAVE_KNEM_IO_H)

#include "knem_io.h"

static int knem_fd = -1;
static int knem_has_dma = 0;

/* 4096 status index */
static volatile knem_status_t *knem_status = MAP_FAILED;
#define KNEM_STATUS_NR 4096 /* FIXME: randomly chosen */

/* Values of KNEM_ABI_VERSION less than this are the old interface (pre-0.7),
 * values greater than or equal to this are the newer interface.  At some point
 * in the future we should drop support for the old version to keep the code
 * simpler. */
#define MPICH_NEW_KNEM_ABI_VERSION (0x0000000c)

/* These are for maintaining a linked-list of outstanding requests on which we
   can make progress. */
struct lmt_dma_node {
    struct lmt_dma_node *next;
    MPIDI_VC_t *vc; /* seems like this should be in the request somewhere, but it's not */
    MPID_Request *req; /* do we need to store type too? */
    volatile knem_status_t *status_p;
};

/* MT: this stack is not thread-safe */
static struct lmt_dma_node *outstanding_head = NULL;

/* MT: this stack is not thread-safe */
static int free_idx; /* is always the index of the next free index */
static int index_stack[KNEM_STATUS_NR] = {0};

/* returns an index into knem_status that is available for use */
static int alloc_status_index(void)
{
    return index_stack[free_idx++];
}

/* returns the given index to the pool */
static void free_status_index(int index)
{
    index_stack[--free_idx] = index;
}


/* Opens the knem device and sets knem_fd accordingly.  Uses mpich2 errhandling
   conventions. */
#undef FUNCNAME
#define FUNCNAME open_knem_dev
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int open_knem_dev(void)
{
    int mpi_errno = MPI_SUCCESS;
    int err;
    int i;
    struct knem_cmd_info info;

    knem_fd = open(KNEM_DEVICE_FILENAME, O_RDWR);
    MPIU_ERR_CHKANDJUMP2(knem_fd < 0, mpi_errno, MPI_ERR_OTHER, "**shm_open",
                         "**shm_open %s %d", KNEM_DEVICE_FILENAME, errno);
    err = ioctl(knem_fd, KNEM_CMD_GET_INFO, &info);
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));
    MPIU_ERR_CHKANDJUMP2(info.abi != KNEM_ABI_VERSION, mpi_errno, MPI_ERR_OTHER,
                         "**abi_version_mismatch", "**abi_version_mismatch %D %D",
                         (unsigned long)KNEM_ABI_VERSION, (unsigned long)info.abi);

    knem_has_dma = (info.features & KNEM_FEATURE_DMA);

    knem_status = mmap(NULL, KNEM_STATUS_NR, PROT_READ|PROT_WRITE, MAP_SHARED, knem_fd, KNEM_STATUS_ARRAY_FILE_OFFSET);
    MPIU_ERR_CHKANDJUMP1(knem_status == MAP_FAILED, mpi_errno, MPI_ERR_OTHER, "**mmap",
                         "**mmap %d", errno);
    for (i = 0; i < KNEM_STATUS_NR; ++i) {
        index_stack[i] = i;
    }
fn_fail:
    return mpi_errno;
}

/* Sends as much data from the request as possible via the knem ioctl.
   s_cookiep is an output parameter */
#undef FUNCNAME
#define FUNCNAME do_dma_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_dma_send(MPIDI_VC_t *vc,  MPID_Request *sreq, int send_iov_n,
                       MPID_IOV send_iov[], knem_cookie_t *s_cookiep)
{
    int mpi_errno = MPI_SUCCESS;
    int i, err;
#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    struct knem_cmd_init_send_param sendcmd = {0};
#else
    struct knem_cmd_create_region cr;
#endif
    struct knem_cmd_param_iovec knem_iov[MPID_IOV_LIMIT];

    /* FIXME The knem module iovec is potentially different from the system
       iovec.  This causes all sorts of fun if you don't realize it and use the
       system iovec directly instead.  Eventually we need to either unify them
       or avoid this extra copy. */
    for (i = 0; i < send_iov_n; ++i) {
        knem_iov[i].base = (uintptr_t)send_iov[i] .MPID_IOV_BUF;
        knem_iov[i].len  = send_iov[i] .MPID_IOV_LEN;
    }

#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    sendcmd.send_iovec_array = (uintptr_t) &knem_iov[0];
    sendcmd.send_iovec_nr = send_iov_n;
    sendcmd.flags = 0;
    err = ioctl(knem_fd, KNEM_CMD_INIT_SEND, &sendcmd);
#else
    cr.iovec_array = (uintptr_t) &knem_iov[0];
    cr.iovec_nr = send_iov_n;
    cr.flags = KNEM_FLAG_SINGLEUSE;
    cr.protection = PROT_READ;
    err = ioctl(knem_fd, KNEM_CMD_CREATE_REGION, &cr);
#endif
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));
#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    *s_cookiep = sendcmd.send_cookie;
#else
    *s_cookiep = cr.cookie;
#endif

fn_fail:
fn_exit:
    return mpi_errno;
}

/* s_cookie is an input parameter */
#undef FUNCNAME
#define FUNCNAME do_dma_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int do_dma_recv(int iov_n, MPID_IOV iov[], knem_cookie_t s_cookie, int nodma, volatile knem_status_t **status_p_p, knem_status_t *current_status_p)
{
    int mpi_errno = MPI_SUCCESS;
    int i, err;

#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    struct knem_cmd_init_async_recv_param recvcmd = {0};
#else
    struct knem_cmd_inline_copy icopy;
#endif
    struct knem_cmd_param_iovec knem_iov[MPID_IOV_LIMIT];

    /* FIXME The knem module iovec is potentially different from the system
       iovec.  This causes all sorts of fun if you don't realize it and use the
       system iovec directly instead.  Eventually we need to either unify them
       or avoid this extra copy. */
    for (i = 0; i < iov_n; ++i) {
        knem_iov[i].base = (uintptr_t)iov[i] .MPID_IOV_BUF;
        knem_iov[i].len  = iov[i] .MPID_IOV_LEN;
    }

#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    recvcmd.recv_iovec_array = (uintptr_t) &knem_iov[0];
    recvcmd.recv_iovec_nr = iov_n;
    recvcmd.status_index = alloc_status_index();
    recvcmd.send_cookie = s_cookie;
    recvcmd.flags = nodma ? 0 : KNEM_FLAG_DMA | KNEM_FLAG_ASYNCDMACOMPLETE;
    err = ioctl(knem_fd, KNEM_CMD_INIT_ASYNC_RECV, &recvcmd);
#else
    icopy.local_iovec_array = (uintptr_t) &knem_iov[0];
    icopy.local_iovec_nr = iov_n;
    icopy.remote_cookie = s_cookie;
    icopy.remote_offset = 0;
    icopy.write = 0;
    icopy.async_status_index = alloc_status_index();
    icopy.flags = nodma ? 0 : KNEM_FLAG_DMA | KNEM_FLAG_ASYNCDMACOMPLETE;
    err = ioctl(knem_fd, KNEM_CMD_INLINE_COPY, &icopy);
#endif
    MPIU_ERR_CHKANDJUMP2(err < 0, mpi_errno, MPI_ERR_OTHER, "**ioctl",
                         "**ioctl %d %s", errno, MPIU_Strerror(errno));

#if KNEM_ABI_VERSION < MPICH_NEW_KNEM_ABI_VERSION
    *status_p_p = &knem_status[recvcmd.status_index];
    *current_status_p = KNEM_STATUS_PENDING;
#else
    *status_p_p = &knem_status[icopy.async_status_index];
    *current_status_p = icopy.current_status;
#endif

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Much like initiate_lmt except it won't send an RTS message.  Used to
   implement initiate_lmt and handle_cookie.  This will send as much data from
   the request in a single shot as possible.
   
   s_cookiep is an output parameter. */
#undef FUNCNAME
#define FUNCNAME send_sreq_data
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int send_sreq_data(MPIDI_VC_t *vc, MPID_Request *sreq, knem_cookie_t *s_cookiep)
{
    int mpi_errno = MPI_SUCCESS;
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
        sreq->dev.iov[0].MPID_IOV_BUF = (char *)sreq->dev.user_buf + dt_true_lb;
        sreq->dev.iov[0].MPID_IOV_LEN = data_sz;
        sreq->dev.iov_count = 1;
    }
    else {
        /* use the segment routines to handle the iovec creation */
        if (sreq->dev.segment_ptr == NULL) {
            sreq->dev.iov_count = MPID_IOV_LIMIT;
            sreq->dev.iov_offset = 0;

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


            /* FIXME we should write our own function that isn't dependent on
               the in-request iov array.  This will let us use IOVs that are
               larger than MPID_IOV_LIMIT. */
            mpi_errno = MPIDI_CH3U_Request_load_send_iov(sreq, &sreq->dev.iov[0],
                                                         &sreq->dev.iov_count);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    mpi_errno = do_dma_send(vc, sreq, sreq->dev.iov_count, sreq->dev.iov, s_cookiep);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

fn_exit:
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME check_req_complete
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int check_req_complete(MPIDI_VC_t *vc, MPID_Request *req, int *complete)
{
    int mpi_errno = MPI_SUCCESS;
    int (*reqFn)(MPIDI_VC_t *, MPID_Request *, int *);
    reqFn = req->dev.OnDataAvail;
    if (reqFn) {
        *complete = 0;
        mpi_errno = reqFn(vc, req, complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else {
        *complete = 1;
        MPIDI_CH3U_Request_complete(req);
    }

fn_fail:
    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_initiate_lmt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request *sreq)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);
    
    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);

    MPIU_CHKPMEM_MALLOC(sreq->ch.s_cookie, knem_cookie_t *, sizeof(knem_cookie_t), mpi_errno, "s_cookie");

    mpi_errno = send_sreq_data(vc, sreq, sreq->ch.s_cookie);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    MPID_nem_lmt_send_RTS(vc, rts_pkt, sreq->ch.s_cookie, sizeof(knem_cookie_t));

fn_exit:
    MPIU_CHKPMEM_COMMIT();
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_INITIATE_LMT);
    return mpi_errno;
fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
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
    int nodma;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPIDI_msg_sz_t data_sz;
    MPID_Datatype * dt_ptr;
    volatile knem_status_t *status;
    knem_status_t current_status;
    struct lmt_dma_node *node = NULL;
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

    nodma = !knem_has_dma || data_sz < MPIR_PARAM_NEM_LMT_DMA_THRESHOLD;

    if (dt_contig) {
        /* handle the iov creation ourselves */
        rreq->dev.iov[0].MPID_IOV_BUF = (char *)rreq->dev.user_buf + dt_true_lb;
        rreq->dev.iov[0].MPID_IOV_LEN = data_sz;
        rreq->dev.iov_count = 1;
    }
    else {
        if (rreq->dev.segment_ptr == NULL) {
            /* segment_ptr may be non-null when this is a continuation of a
               many-part message that we couldn't fit in one single flight of
               iovs. */
            MPIU_Assert(rreq->dev.segment_ptr == NULL);
            rreq->dev.segment_ptr = MPID_Segment_alloc();
            MPIU_ERR_CHKANDJUMP1((rreq->dev.segment_ptr == NULL), mpi_errno,
                                 MPI_ERR_OTHER, "**nomem",
                                 "**nomem %s", "MPID_Segment_alloc");
            MPID_Segment_init(rreq->dev.user_buf, rreq->dev.user_count,
                              rreq->dev.datatype, rreq->dev.segment_ptr, 0);
            rreq->dev.segment_first = 0;
            rreq->dev.segment_size = data_sz;

            /* see load_send_iov FIXME above */
            mpi_errno = MPIDI_CH3U_Request_load_recv_iov(rreq);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }

    MPIU_Assert(s_cookie.MPID_IOV_LEN == sizeof(knem_cookie_t));
    MPIU_Assert(s_cookie.MPID_IOV_BUF != NULL);
    mpi_errno = do_dma_recv(rreq->dev.iov_count, rreq->dev.iov,
                            *((knem_cookie_t *)s_cookie.MPID_IOV_BUF), nodma,
                            &status, &current_status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* TODO refactor this block and MPID_nem_lmt_dma_progress (and anywhere
     * else) to share a common function.  This advancement/completion code is
     * duplication. */
    if (current_status != KNEM_STATUS_PENDING) {
        /* complete the request if all data has been sent, remove it from the list */
        int complete = 0;

        MPIU_ERR_CHKANDJUMP1(current_status == KNEM_STATUS_FAILED, mpi_errno, MPI_ERR_OTHER,
                             "**recv_status", "**recv_status %d", current_status);

        mpi_errno = check_req_complete(vc, rreq, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        free_status_index(status - knem_status);

        if (complete) {
            /* request was completed by the OnDataAvail fn */
            MPID_nem_lmt_send_DONE(vc, rreq); /* tell the other side to complete its request */
            MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");

        }
        else {
            /* There is more data to send.  We must inform the sender that we have
               completely received the current batch and that the next batch should
               be sent. */
            MPID_nem_lmt_send_COOKIE(vc, rreq, NULL, 0);
        }
    }

    /* XXX DJG FIXME this looks like it always pushes! */
    /* push request if not complete for progress checks later */
    node = MPIU_Malloc(sizeof(struct lmt_dma_node));
    node->vc = vc;
    node->req = rreq;
    node->status_p = status;
    node->next = outstanding_head;
    outstanding_head = node;
    ++MPID_nem_local_lmt_pending;

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

    /* free cookie from RTS packet */
    MPIU_Free(sreq->ch.s_cookie);

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
        /* There is more data to send. */
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
        int complete;
        knem_cookie_t s_cookie;

        /* This function will invoke the OnDataAvail function to load more data. */
        mpi_errno = check_req_complete(vc, req, &complete);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* If we were complete we should have received a DONE message instead
           of a COOKIE message. */
        MPIU_Assert(!complete);

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

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_progress(void)
{
    int mpi_errno = MPI_SUCCESS;
    struct lmt_dma_node *prev = NULL;
    struct lmt_dma_node *free_me = NULL;
    struct lmt_dma_node *cur = outstanding_head;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_PROGRESS);
    
    /* Iterate over a linked-list of (req,status_idx)-tuples looking for
       completed/failed requests.  Currently knem only provides status to the
       receiver, so all of these requests are recv requests. */
    while (cur) {
        switch (*cur->status_p) {
            case KNEM_STATUS_SUCCESS:
                {
                    /* complete the request if all data has been sent, remove it from the list */
                    int complete = 0;
                    mpi_errno = check_req_complete(cur->vc, cur->req, &complete);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                    free_status_index(cur->status_p - knem_status);

                    if (complete) {
                        /* request was completed by the OnDataAvail fn */
                        MPID_nem_lmt_send_DONE(cur->vc, cur->req); /* tell the other side to complete its request */
                        MPIU_DBG_MSG(CH3_CHANNEL, VERBOSE, ".... complete");

                    }
                    else {
                        /* There is more data to send.  We must inform the sender that we have
                           completely received the current batch and that the next batch should
                           be sent. */
                        MPID_nem_lmt_send_COOKIE(cur->vc, cur->req, NULL, 0);
                    }

                    /* Right now we always free the cur element, even if the
                       request is incomplete because it simplifies the logic. */
                    if (cur == outstanding_head) {
                        outstanding_head = cur->next;
                        prev = NULL;
                        free_me = cur;
                        cur = cur->next;
                    }
                    else {
                        prev->next = cur->next;
                        free_me = cur;
                        cur = cur->next;
                    }
                    if (free_me) MPIU_Free(free_me);
                    --MPID_nem_local_lmt_pending;
                    continue;
                }
                break;
            case KNEM_STATUS_FAILED:
                /* set the error status for the request, complete it then dequeue the entry */
                cur->req->status.MPI_ERROR = MPI_SUCCESS;
                MPIU_ERR_SET1(cur->req->status.MPI_ERROR, MPI_ERR_OTHER, "**recv_status", "**recv_status %d", *cur->status_p);

                MPIDI_CH3U_Request_complete(cur->req);

                if (cur == outstanding_head) {
                    outstanding_head = cur->next;
                    prev = NULL;
                    free_me = cur;
                    cur = cur->next;
                }
                else {
                    prev->next = cur->next;
                    free_me = cur;
                    cur = cur->next;
                }

                if (free_me) MPIU_Free(free_me);
                --MPID_nem_local_lmt_pending;
                continue;
                
                break;
            case KNEM_STATUS_PENDING:
                /* nothing to do here */
                break;
            default:
                MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**invalid_knem_status",
                                     "**invalid_knem_status %d", *cur->status_p);
                break;
        }

        prev = cur;
        cur = cur->next;
    }

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_PROGRESS);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_dma_vc_terminated
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPID_nem_lmt_dma_vc_terminated(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_DMA_VC_TERMINATED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_DMA_VC_TERMINATED);

    /* Do nothing.  KNEM should abort any ops with dead processes. */

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_DMA_VC_TERMINATED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* --------------------------------------------------------------------------
   The functions below are nops, stubs that might be used in later versions of
   this code.
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

#endif /* HAVE_KNEM_IO_H */
