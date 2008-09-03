/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#include "mpid_nem_datatypes.h"

#ifdef ENABLE_NO_SCHED_YIELD
#define SCHED_YIELD() do { } while(0)
#else
#include <sched.h>
#define SCHED_YIELD() sched_yield()
#endif

int MPID_nem_lmt_shm_pending = FALSE;

/* Progress queue */

typedef struct lmt_shm_prog_element
{
    MPIDI_VC_t *vc;
    struct lmt_shm_prog_element *next;
    struct lmt_shm_prog_element *prev;
} lmt_shm_prog_element_t;

static struct {lmt_shm_prog_element_t *head;} lmt_shm_progress_q = {NULL};

#define LMT_SHM_L_EMPTY() GENERIC_L_EMPTY(lmt_shm_progress_q)
#define LMT_SHM_L_HEAD() GENERIC_L_HEAD(lmt_shm_progress_q)
#define LMT_SHM_L_REMOVE(ep) GENERIC_L_REMOVE(&lmt_shm_progress_q, ep, next, prev)
#define LMT_SHM_L_ADD(ep) GENERIC_L_ADD(&lmt_shm_progress_q, ep, next, prev)

typedef struct MPID_nem_lmt_shm_wait_element
{
    int (* progress)(MPIDI_VC_t *vc, MPID_Request *req, int *done);
    MPID_Request *req;
    struct MPID_nem_lmt_shm_wait_element *next;
} MPID_nem_lmt_shm_wait_element_t;

#define LMT_SHM_Q_EMPTY(qp) GENERIC_Q_EMPTY(qp)
#define LMT_SHM_Q_HEAD(qp) GENERIC_Q_HEAD(qp)
#define LMT_SHM_Q_ENQUEUE(qp, ep) GENERIC_Q_ENQUEUE(qp, ep, next)
#define LMT_SHM_Q_ENQUEUE_AT_HEAD(qp, ep) GENERIC_Q_ENQUEUE_AT_HEAD(qp, ep, next)
#define LMT_SHM_Q_DEQUEUE(qp, epp) GENERIC_Q_DEQUEUE(qp, epp, next)
#define LMT_SHM_Q_SEARCH_REMOVE(qp, req_id, epp) GENERIC_Q_SEARCH_REMOVE(qp, _e->req->handle == (req_id), epp, \
                                                                         MPID_nem_lmt_shm_wait_element_t, next)
#define CHECK_Q(qp) do{\
    if (LMT_SHM_Q_EMPTY(*(qp))) break;\
    if (LMT_SHM_Q_HEAD(*(qp))->next == NULL && (qp)->head != (qp)->tail)\
    {\
	printf ("ERROR\n");\
	while(1);\
    }\
    \
    }while(0)


/* copy buffer in shared memory */
#define NUM_BUFS 8
#define MPID_NEM_COPY_BUF_LEN (32 * 1024)

typedef union
{
    int val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_int_t;

typedef union
{
    MPIDI_msg_sz_t val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_msg_sz_t;

typedef union
{
    struct
    {
        int rank;
        int remote_req_id;
    } val;
    char padding[MPID_NEM_CACHE_LINE_LEN];
} MPID_nem_cacheline_owner_info_t;

typedef struct MPID_nem_copy_buf
{
    MPID_nem_cacheline_owner_info_t owner_info;
    MPID_nem_cacheline_int_t sender_present; /* is the sender currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t receiver_present; /* is the receiver currently in the lmt progress function for this buffer */
    MPID_nem_cacheline_int_t len[NUM_BUFS];
    char underflow_buf[MPID_NEM_CACHE_LINE_LEN]; /* used when not all data could be unpacked from previous buffer */
    char buf[NUM_BUFS][MPID_NEM_COPY_BUF_LEN];
} MPID_nem_copy_buf_t;
/* copy buffer flag values */
#define BUF_EMPTY 0
#define BUF_FULL  1

#define NO_OWNER -1

/* pipeline values : if the data size is less than PIPELINE_THRESHOLD,
   then copy no more than PIPELINE_MAX_SIZE at a time. */
#define PIPELINE_MAX_SIZE (16 * 1024)
#define PIPELINE_THRESHOLD (128 * 1024)


static inline int lmt_shm_progress_vc(MPIDI_VC_t *vc, int *done);
static int lmt_shm_send_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int lmt_shm_recv_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done);
static int MPID_nem_allocate_shm_region(volatile MPID_nem_copy_buf_t **buf_p, char *handle[]);
static int MPID_nem_attach_shm_region(volatile MPID_nem_copy_buf_t **buf_p, const char handle[]);
static int MPID_nem_detach_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[]);
static int MPID_nem_delete_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[]);

/* number of iterations to wait for the other side to process a buffer */
#define LMT_POLLS_BEFORE_YIELD 1000

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_initiate_lmt
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt, MPID_Request *req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_msg_sz_t data_sz;
    int dt_contig;
    MPI_Aint dt_true_lb;
    MPID_Datatype * dt_ptr;
    MPID_nem_pkt_lmt_rts_t * const rts_pkt = (MPID_nem_pkt_lmt_rts_t *)pkt;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);

    MPID_nem_lmt_send_RTS(vc, rts_pkt, NULL, 0);

    MPIDI_Datatype_get_info(req->dev.user_count, req->dev.datatype, dt_contig, data_sz, dt_ptr, dt_true_lb);
    req->ch.lmt_data_sz = data_sz;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_INITIATE_LMT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_start_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int done = FALSE;
    MPIU_CHKPMEM_DECL(2);
    MPID_nem_lmt_shm_wait_element_t *e;
    int queue_initially_empty;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);

    if (vc_ch->lmt_copy_buf == NULL)
    {
        int i;
        mpi_errno = MPID_nem_allocate_shm_region(&vc_ch->lmt_copy_buf, &vc_ch->lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        vc_ch->lmt_copy_buf->sender_present.val   = 0;
        vc_ch->lmt_copy_buf->receiver_present.val = 0;

        for (i = 0; i < NUM_BUFS; ++i)
            vc_ch->lmt_copy_buf->len[i].val = 0;

        vc_ch->lmt_copy_buf->owner_info.val.rank          = NO_OWNER;
        vc_ch->lmt_copy_buf->owner_info.val.remote_req_id = MPI_REQUEST_NULL;
    }

    /* send CTS with handle for copy buffer */
    MPID_nem_lmt_send_CTS(vc, req, vc_ch->lmt_copy_buf_handle, (int)strlen(vc_ch->lmt_copy_buf_handle) + 1);

    queue_initially_empty = LMT_SHM_Q_EMPTY(vc_ch->lmt_queue) && vc_ch->lmt_active_lmt == NULL;

    MPIU_CHKPMEM_MALLOC (e, MPID_nem_lmt_shm_wait_element_t *, sizeof (MPID_nem_lmt_shm_wait_element_t), mpi_errno, "lmt wait queue element");
    e->progress = lmt_shm_recv_progress;
    e->req = req;
    LMT_SHM_Q_ENQUEUE(&vc_ch->lmt_queue, e); /* MT: not thread safe */

    /* make progress on that vc */
    mpi_errno = lmt_shm_progress_vc(vc, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* MT: not thread safe, another thread may have enqueued another
       lmt after we did, and added this vc to the progress list.  In
       that case we would be adding the vc twice. */
    if (!done && queue_initially_empty)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_prog_element_t *pe;

        MPIU_CHKPMEM_MALLOC (pe, lmt_shm_prog_element_t *, sizeof (lmt_shm_prog_element_t), mpi_errno, "lmt progress queue element");
        pe->vc = vc;
        LMT_SHM_L_ADD(pe);
        MPID_nem_lmt_shm_pending = TRUE;
        MPIU_Assert(!vc_ch->lmt_enqueued);
        vc_ch->lmt_enqueued = TRUE;
    }

    MPIU_Assert(LMT_SHM_Q_EMPTY(vc_ch->lmt_queue) || !LMT_SHM_L_EMPTY());

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_RECV);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_start_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie)
{
    int mpi_errno = MPI_SUCCESS;
    int done = FALSE;
    int queue_initially_empty;
    MPID_nem_lmt_shm_wait_element_t *e;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIU_CHKPMEM_DECL(3);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);

    if (vc_ch->lmt_copy_buf == NULL)
    {
        MPIU_CHKPMEM_MALLOC (vc_ch->lmt_copy_buf_handle, char *, r_cookie.MPID_IOV_LEN, mpi_errno, "copy buf handle");
        MPID_NEM_MEMCPY(vc_ch->lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN);

        mpi_errno = MPID_nem_attach_shm_region(&vc_ch->lmt_copy_buf, vc_ch->lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    else if (strncmp(vc_ch->lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN) < 0)
    {
        /* Each side allocated its own buffer, lexicographically lower valued buffer handle is deleted */

        mpi_errno = MPID_nem_delete_shm_region(vc_ch->lmt_copy_buf, vc_ch->lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        vc_ch->lmt_copy_buf = NULL;

        MPIU_CHKPMEM_MALLOC (vc_ch->lmt_copy_buf_handle, char *, r_cookie.MPID_IOV_LEN, mpi_errno, "copy buf handle");
        MPID_NEM_MEMCPY(vc_ch->lmt_copy_buf_handle, r_cookie.MPID_IOV_BUF, r_cookie.MPID_IOV_LEN);

        mpi_errno = MPID_nem_attach_shm_region(&vc_ch->lmt_copy_buf, vc_ch->lmt_copy_buf_handle);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        /* put the pending receive req back on the queue to try again later */
        LMT_SHM_Q_ENQUEUE_AT_HEAD(&vc_ch->lmt_queue, vc_ch->lmt_active_lmt); /* MT: not thread safe */
        vc_ch->lmt_active_lmt = NULL;
    }

    queue_initially_empty = LMT_SHM_Q_EMPTY(vc_ch->lmt_queue) && vc_ch->lmt_active_lmt == NULL;

    MPIU_CHKPMEM_MALLOC (e, MPID_nem_lmt_shm_wait_element_t *, sizeof (MPID_nem_lmt_shm_wait_element_t), mpi_errno, "lmt wait queue element");
    e->progress = lmt_shm_send_progress;
    e->req = req;
    LMT_SHM_Q_ENQUEUE(&vc_ch->lmt_queue, e); /* MT: not thread safe */

    /* make progress on that vc */
    mpi_errno = lmt_shm_progress_vc(vc, &done);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    /* MT: not thread safe, another thread may have enqueued another
       lmt after we did, and added this vc to the progress list.  In
       that case we would be adding the vc twice. */
    if (!done && queue_initially_empty)
    {
        /* lmt send didn't finish, enqueue it to be completed later */
        lmt_shm_prog_element_t *pe;

        MPIU_CHKPMEM_MALLOC (pe, lmt_shm_prog_element_t *, sizeof (lmt_shm_prog_element_t), mpi_errno, "lmt progress queue element");
        pe->vc = vc;
        LMT_SHM_L_ADD(pe);
        MPID_nem_lmt_shm_pending = TRUE;
        MPIU_Assert(!vc_ch->lmt_enqueued);
        vc_ch->lmt_enqueued = TRUE;
   }

    MPIU_Assert(LMT_SHM_Q_EMPTY(vc_ch->lmt_queue) || !LMT_SHM_L_EMPTY());


    MPIU_CHKPMEM_COMMIT();
 fn_return:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_START_SEND);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_return;
}

#undef FUNCNAME
#define FUNCNAME get_next_req
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int get_next_req(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc_ch->lmt_copy_buf;
    int prev_owner_rank;
    MPID_Request *req;
    MPIDI_STATE_DECL(MPID_STATE_GET_NEXT_REQ);

    MPIDI_FUNC_ENTER(MPID_STATE_GET_NEXT_REQ);

    prev_owner_rank = MPIDU_Atomic_cas_int(&copy_buf->owner_info.val.rank, NO_OWNER, MPIDI_Process.my_pg_rank);

    if (prev_owner_rank == MPIDI_Process.my_pg_rank)
    {
        /* last lmt is not complete (receiver still receiving */
        goto fn_exit;
    }

    if (prev_owner_rank == NO_OWNER)
    {
        int i;
        /* successfully grabbed idle copy buf */
        MPIDU_Shm_write_barrier();
        for (i = 0; i < NUM_BUFS; ++i)
            copy_buf->len[i].val = 0;

        MPIDU_Shm_write_barrier();

        LMT_SHM_Q_DEQUEUE(&vc_ch->lmt_queue, &vc_ch->lmt_active_lmt);
        copy_buf->owner_info.val.remote_req_id = vc_ch->lmt_active_lmt->req->ch.lmt_req_id;
    }
    else
    {
        /* copy buf is owned by the remote side */
        /* remote side chooses next transfer */
        int i = 0;

        MPIDU_Shm_read_barrier();
        while (copy_buf->owner_info.val.remote_req_id == MPI_REQUEST_NULL)
        {
            if (i == LMT_POLLS_BEFORE_YIELD)
            {
                SCHED_YIELD();
                i = 0;
            }
            ++i;
        }

        MPIDU_Shm_read_barrier();
        LMT_SHM_Q_SEARCH_REMOVE(&vc_ch->lmt_queue, copy_buf->owner_info.val.remote_req_id, &vc_ch->lmt_active_lmt);

        if (vc_ch->lmt_active_lmt == NULL)
            /* request not found  */
            goto fn_exit;
    }

    req = vc_ch->lmt_active_lmt->req;
    if (req->dev.segment_ptr == NULL)
    {
        /* Check to see if we've already allocated a seg for this req.
           This can happen if both sides allocated copy buffers, and
           we decided to use the remote side's buffer. */
        req->dev.segment_ptr = MPID_Segment_alloc();
        MPIU_ERR_CHKANDJUMP1((req->dev.segment_ptr == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPID_Segment_alloc");
        MPID_Segment_init(req->dev.user_buf, req->dev.user_count, req->dev.datatype, req->dev.segment_ptr, 0);
        req->dev.segment_first = 0;
    }
    vc_ch->lmt_buf_num = 0;
    vc_ch->lmt_surfeit = 0;

    MPIU_Assert((vc_ch->lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc_ch->lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->handle));

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_GET_NEXT_REQ);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* The message is copied in a pipelined fashion.  There are NUM_BUFS
   buffers to copy through.  The sender waits until there is an empty
   buffer, then fills it in and marks the number of bytes copied.
   Note that because segment_pack() copies on basic-datatype granularity,
   (i.e., won't copy three bytes of an int) we may not fill the entire
   buffer each time. */

#undef FUNCNAME
#define FUNCNAME lmt_shm_send_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int lmt_shm_send_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc_ch->lmt_copy_buf;
    MPIDI_msg_sz_t first;
    MPIDI_msg_sz_t last;
    int buf_num;
    MPIDI_msg_sz_t data_sz, copy_limit;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_SEND_PROGRESS);

    MPIU_Assert((vc_ch->lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc_ch->lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->handle));

    copy_buf->sender_present.val = TRUE;

    MPIU_Assert(req == vc_ch->lmt_active_lmt->req);
/*     MPIU_Assert(MPIDI_Request_get_type(req) == MPIDI_REQUEST_TYPE_SEND); */

    data_sz = req->ch.lmt_data_sz;
    buf_num = vc_ch->lmt_buf_num;
    first = req->dev.segment_first;

    do
    {
        int i;
        /* If the buffer is full, wait.  If the receiver is actively
           working on this transfer, yield the processor and keep
           waiting, otherwise wait for a bounded amount of time. */
        i = 0;
        while (copy_buf->len[buf_num].val != 0)
        {
            if (i == LMT_POLLS_BEFORE_YIELD)
            {
                if (copy_buf->receiver_present.val)
                {
                    SCHED_YIELD();
                    i = 0;
                }
                else
                {
                    req->dev.segment_first = first;
                    vc_ch->lmt_buf_num = buf_num;
                    *done = FALSE;
                    goto fn_exit;
                }
            }

            ++i;
        }

        MPIDU_Shm_read_write_barrier();


        /* we have a free buffer, fill it */
        if (data_sz <= PIPELINE_THRESHOLD)
            copy_limit = PIPELINE_MAX_SIZE;
        else
            copy_limit = MPID_NEM_COPY_BUF_LEN;
        last = (data_sz - first <= copy_limit) ? data_sz : first + copy_limit;
	MPID_Segment_pack(req->dev.segment_ptr, first, &last, (void *)copy_buf->buf[buf_num]); /* cast away volatile */
        MPIDU_Shm_write_barrier();
        copy_buf->len[buf_num].val = last - first;

        first = last;
        buf_num = (buf_num+1) % NUM_BUFS;
    }
    while (last < data_sz);

    *done = TRUE;
    MPIDI_CH3U_Request_complete(req);

 fn_exit:
    copy_buf->sender_present.val = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_SEND_PROGRESS);
    return mpi_errno;
}

/* Continued from note for lmt_shm_send_progress() above.  The
   receiver uses segment_unpack(), and just like segment_pack() it may
   not copy all the data you ask it to.  This means that we may leave
   some data in the buffer uncopied.  To handle this, we copy the
   remaining data to just before the next buffer.  Then when the next
   buffer is filled, we start copying where we just copied the
   leftover data from last time.  */

#undef FUNCNAME
#define FUNCNAME lmt_shm_recv_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int lmt_shm_recv_progress(MPIDI_VC_t *vc, MPID_Request *req, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    volatile MPID_nem_copy_buf_t * const copy_buf = vc_ch->lmt_copy_buf;
    MPIDI_msg_sz_t first;
    MPIDI_msg_sz_t last, expected_last;
    int buf_num;
    MPIDI_msg_sz_t data_sz, len;
    int i;
    MPIDI_msg_sz_t surfeit;
    char *src_buf;
    char tmpbuf[MPID_NEM_CACHE_LINE_LEN];
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_RECV_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_RECV_PROGRESS);
    
    MPIU_Assert((vc_ch->lmt_copy_buf->owner_info.val.rank == MPIDI_Process.my_pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->ch.lmt_req_id) ||
                (vc_ch->lmt_copy_buf->owner_info.val.rank == vc->pg_rank &&
                 vc_ch->lmt_copy_buf->owner_info.val.remote_req_id == vc_ch->lmt_active_lmt->req->handle));

    copy_buf->receiver_present.val = TRUE;

    surfeit = vc_ch->lmt_surfeit;
    data_sz = req->ch.lmt_data_sz;
    buf_num = vc_ch->lmt_buf_num;
    first = req->dev.segment_first;

    do
    {
        /* If the buffer is empty, wait.  If the sender is actively
           working on this transfer, yield the processor and keep
           waiting, otherwise wait for a bounded amount of time. */
        i = 0;
        while ((len = copy_buf->len[buf_num].val) == 0)
        {
            if (i == LMT_POLLS_BEFORE_YIELD)
            {
                if (copy_buf->sender_present.val)
                {
                    SCHED_YIELD();
                    i = 0;
                }
                else
                {
                    req->dev.segment_first = first;
                    vc_ch->lmt_buf_num = buf_num;
                    vc_ch->lmt_surfeit = surfeit;
                    *done = FALSE;
                    goto fn_exit;
                }
            }

            ++i;
        }

        MPIDU_Shm_read_barrier();

        /* unpack data including any leftover from the previous buffer */
        src_buf = ((char *)copy_buf->buf[buf_num]) - surfeit; /* cast away volatile */
        last = expected_last = (data_sz - first <= surfeit + len) ? data_sz : first + surfeit + len;

	MPID_Segment_unpack(req->dev.segment_ptr, first, &last, src_buf);

        if (surfeit && buf_num > 0)
        {
            /* we had leftover data from the previous buffer, we can
               now mark that buffer as empty */

            MPIDU_Shm_read_write_barrier();
            copy_buf->len[(buf_num-1)].val = 0;
            /* Make sure we copied at least the leftover data from last time */
            MPIU_Assert(last - first > surfeit);
       }

        if (last < expected_last)
        {
            /* we have leftover data in the buffer that we couldn't copy out */
            char *surfeit_ptr;

            surfeit_ptr = (char *)src_buf + last - first;
            surfeit = expected_last - last;

            if (buf_num == NUM_BUFS-1)
            {
                /* if we're wrapping back to buf 0, then we can copy it directly */
                memcpy(((char *)copy_buf->buf[0]) - surfeit, surfeit_ptr, surfeit);

                MPIDU_Shm_read_write_barrier();
                copy_buf->len[buf_num].val = 0;
            }
            else
            {
                /* otherwise, we need to copy to a tmpbuf first to make sure the src and dest addresses don't overlap */
                memcpy(tmpbuf, surfeit_ptr, surfeit);
                memcpy(((char *)copy_buf->buf[buf_num+1]) - surfeit, tmpbuf, surfeit);
            }
        }
        else
        {
            /* all data was unpacked, we can mark this buffer as empty */
            surfeit = 0;

            MPIDU_Shm_read_write_barrier();
            copy_buf->len[buf_num].val = 0;	
        }

        first = last;
        buf_num = (buf_num+1) % NUM_BUFS;
    }
    while (last < data_sz);

    for (i = 0; i < NUM_BUFS; ++i)
        copy_buf->len[i].val = 0;

    copy_buf->owner_info.val.remote_req_id = MPI_REQUEST_NULL;
    MPIDU_Shm_write_barrier();
    copy_buf->owner_info.val.rank          = NO_OWNER;

    *done = TRUE;
    MPIDI_CH3U_Request_complete(req);

 fn_exit:
    copy_buf->receiver_present.val = FALSE;
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_RECV_PROGRESS);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_handle_cookie
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_HANDLE_COOKIE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_HANDLE_COOKIE);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_HANDLE_COOKIE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_done_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_done_send(MPIDI_VC_t *vc, MPID_Request *req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_DONE_SEND);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_DONE_SEND);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_DONE_SEND);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_done_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_done_recv(MPIDI_VC_t *vc, MPID_Request *req)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_DONE_RECV);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_DONE_RECV);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_DONE_RECV);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_progress_vc
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static inline int lmt_shm_progress_vc(MPIDI_VC_t *vc, int *done)
{
    int mpi_errno = MPI_SUCCESS;
    int done_req = FALSE;
    MPID_nem_lmt_shm_wait_element_t *we;
    MPIDI_CH3I_VC *vc_ch = (MPIDI_CH3I_VC *)vc->channel_private;
    MPIDI_STATE_DECL(MPID_STATE_LMT_SHM_PROGRESS_VC);

    MPIDI_FUNC_ENTER(MPID_STATE_LMT_SHM_PROGRESS_VC);

    *done = FALSE;

    if (vc_ch->lmt_active_lmt == NULL)
    {
        mpi_errno = get_next_req(vc);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (vc_ch->lmt_active_lmt == NULL)
        {
            /* couldn't find an appropriate request, try again later */
            goto fn_exit;
        }
    }

    we = vc_ch->lmt_active_lmt;
    mpi_errno = we->progress(vc, we->req, &done_req);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    if (done_req)
    {
        MPIU_Free(vc_ch->lmt_active_lmt);
        vc_ch->lmt_active_lmt = NULL;

        if (LMT_SHM_Q_EMPTY(vc_ch->lmt_queue))
            *done = TRUE;
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_LMT_SHM_PROGRESS_VC);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_lmt_shm_progress
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_lmt_shm_progress()
{
    int mpi_errno = MPI_SUCCESS;
    lmt_shm_prog_element_t *pe;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);

    pe = LMT_SHM_L_HEAD();

    while (pe)
    {
        int done = FALSE;

        mpi_errno = lmt_shm_progress_vc(pe->vc, &done);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);

        if (done)
        {
            lmt_shm_prog_element_t *f;
            MPIU_Assert(LMT_SHM_Q_EMPTY(((MPIDI_CH3I_VC *)pe->vc->channel_private)->lmt_queue));
            MPIU_Assert(((MPIDI_CH3I_VC *)pe->vc->channel_private)->lmt_active_lmt == NULL);
            MPIU_Assert(((MPIDI_CH3I_VC *)pe->vc->channel_private)->lmt_enqueued);
            ((MPIDI_CH3I_VC *)pe->vc->channel_private)->lmt_enqueued = FALSE;

            f = pe;
            pe = pe->next;
            LMT_SHM_L_REMOVE(f);
            MPIU_Free(f);
        }
        else
            pe = pe->next;
    }

    if (LMT_SHM_L_EMPTY())
        MPID_nem_lmt_shm_pending = FALSE;

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LMT_SHM_PROGRESS);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_allocate_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_allocate_shm_region(volatile MPID_nem_copy_buf_t **buf_p, char *handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);

    if (*buf_p)
    {
        /* we're already attached */
        goto fn_exit;
    }

    mpi_errno = MPID_nem_allocate_shared_memory((char **)buf_p, sizeof (MPID_nem_copy_buf_t), handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ALLOCATE_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_attach_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_attach_shm_region(volatile MPID_nem_copy_buf_t **buf_p, const char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);

    if(*buf_p)
    {
        /* we're already attached */
        goto fn_exit;
    }

    mpi_errno = MPID_nem_attach_shared_memory((char **)buf_p, sizeof (MPID_nem_copy_buf_t), handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_remove_shared_memory(handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_ATTACH_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_detach_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_detach_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);

    MPIU_Free(handle);

    mpi_errno = MPID_nem_detach_shared_memory ((char *)buf, sizeof (MPID_nem_copy_buf_t));
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DETACH_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_delete_shm_region
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
static int MPID_nem_delete_shm_region(volatile MPID_nem_copy_buf_t *buf, char handle[])
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);

    mpi_errno = MPID_nem_remove_shared_memory(handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

    mpi_errno = MPID_nem_detach_shm_region(buf, handle);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_DELETE_SHM_REGION);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
