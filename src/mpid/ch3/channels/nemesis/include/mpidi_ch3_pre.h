/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED)
#define MPICH_MPIDI_CH3_PRE_H_INCLUDED
#include "mpid_nem_pre.h"
#include "mpid_nem_generic_queue.h"

#if defined(HAVE_NETINET_IN_H)
    #include <netinet/in.h>
#elif defined(HAVE_WINSOCK2_H)
    #include <winsock2.h>
    #include <windows.h>
#endif

/*#define MPID_USE_SEQUENCE_NUMBERS*/
/*#define HAVE_CH3_PRE_INIT*/
/* #define MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS */
#define MPIDI_DEV_IMPLEMENTS_KVS

/* Nemesis packets */
#define MPIDI_CH3_PKT_ENUM                  \
    MPIDI_NEM_PKT_LMT_RTS,                  \
    MPIDI_NEM_PKT_LMT_CTS,                  \
    MPIDI_NEM_PKT_LMT_DONE,                 \
    MPIDI_NEM_PKT_LMT_COOKIE,               \
    MPIDI_NEM_PKT_CKPT_MARKER,              \
    MPIDI_NEM_PKT_NETMOD

typedef struct {
    struct MPID_nem_barrier_vars *barrier_vars; /* shared memory variables used in barrier */
    void *netmod_priv;      /* netmod communicator private data */
} MPIDI_CH3I_CH_comm_t;
    
typedef enum MPIDI_CH3I_VC_state
{
    MPIDI_CH3I_VC_STATE_UNCONNECTED,
    MPIDI_CH3I_VC_STATE_CONNECTING,
    MPIDI_CH3I_VC_STATE_CONNECTED,
    MPIDI_CH3I_VC_STATE_FAILED
}
MPIDI_CH3I_VC_state_t;

/* size of private data area in vc and req for network modules */
#define MPIDI_NEM_VC_NETMOD_AREA_LEN 128
#define MPIDI_NEM_REQ_NETMOD_AREA_LEN 192

/* define functions for access MPID_nem_lmt_rts_queue_t */
typedef GENERIC_Q_DECL(struct MPID_Request) MPID_nem_lmt_rts_queue_t;
#define MPID_nem_lmt_rtsq_empty(q) GENERIC_Q_EMPTY (q)
#define MPID_nem_lmt_rtsq_head(q) GENERIC_Q_HEAD (q)
#define MPID_nem_lmt_rtsq_enqueue(qp, ep) do {                                          \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPID_nem_lmt_rtsq_enqueue req=%p (handle=%#x), queue=%p",    \
                          ep, (ep)->handle, qp));                                       \
        GENERIC_Q_ENQUEUE (qp, ep, dev.next);                                           \
    } while (0)
#define MPID_nem_lmt_rtsq_dequeue(qp, epp)  do {                                        \
        GENERIC_Q_DEQUEUE (qp, epp, dev.next);                                          \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPID_nem_lmt_rtsq_dequeue req=%p (handle=%#x), queue=%p",    \
                          *(epp), *(epp) ? (*(epp))->handle : -1, qp));                 \
    } while (0)
#define MPID_nem_lmt_rtsq_search_remove(qp, req_id, epp) do {                           \
        GENERIC_Q_SEARCH_REMOVE(qp, _e->handle == (req_id), epp,                        \
                struct MPID_Request, dev.next);                                         \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                    "MPID_nem_lmt_rtsq_search_remove req=%p (handle=%#x), queue=%p",    \
                    *(epp), req_id, qp));                                               \
} while (0)

typedef struct MPIDI_CH3I_VC
{
    int pg_rank;
    struct MPID_Request *recv_active;

    int is_local;
    unsigned short send_seqno;
    MPID_nem_fbox_mpich_t *fbox_out;
    MPID_nem_fbox_mpich_t *fbox_in;
    MPID_nem_queue_ptr_t recv_queue;
    MPID_nem_queue_ptr_t free_queue;

#ifdef ENABLE_CHECKPOINTING
    MPIDI_msg_sz_t ckpt_msg_len;
    void *ckpt_msg_buf;
#endif

    /* temp buffer to store partially received header */
    MPIDI_msg_sz_t pending_pkt_len;
    union MPIDI_CH3_Pkt *pending_pkt;

    /* can be used by netmods to put this vc on a send queue or list */
    struct MPIDI_VC *next;
    struct MPIDI_VC *prev;

    /* contig function pointers.  Netmods should set these. */
    /* iStartContigMsg -- sends a message consisting of a header (hdr) and contiguous data (data), possibly of 0 size.  If the
       message cannot be sent immediately, the function should create a request and return a pointer in sreq_ptr.  The network
       module should complete the request once the message has been completely sent. */
    int (* iStartContigMsg)(struct MPIDI_VC *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                            struct MPID_Request **sreq_ptr);
    /* iSentContig -- sends a message consisting of a header (hdr) and contiguous data (data), possibly of 0 size.  The
       network module should complete the request once the message has been completely sent. */
    int (* iSendContig)(struct MPIDI_VC *vc, struct MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                        void *data, MPIDI_msg_sz_t data_sz);

#ifdef ENABLE_CHECKPOINTING
    /* ckpt_pause_send -- netmod should stop sending on this vc and queue messages to be sent after ckpt_continue()*/
    int (* ckpt_pause_send_vc)(struct MPIDI_VC *vc);
    /* ckpt_continue -- Notify remote side to start sending again. */
    int (* ckpt_continue_vc)(struct MPIDI_VC *vc);
    /* ckpt_restart -- similar to ckpt_continue, except that the process has been restarted */
    int (* ckpt_restart_vc)(struct MPIDI_VC *vc);
#endif

    /* LMT function pointers */
    int (* lmt_initiate_lmt)(struct MPIDI_VC *vc, union MPIDI_CH3_Pkt *rts_pkt, struct MPID_Request *req);
    int (* lmt_start_recv)(struct MPIDI_VC *vc, struct MPID_Request *req, MPL_IOV s_cookie);
    int (* lmt_start_send)(struct MPIDI_VC *vc, struct MPID_Request *sreq, MPL_IOV r_cookie);
    int (* lmt_handle_cookie)(struct MPIDI_VC *vc, struct MPID_Request *req, MPL_IOV cookie);
    int (* lmt_done_send)(struct MPIDI_VC *vc, struct MPID_Request *req);
    int (* lmt_done_recv)(struct MPIDI_VC *vc, struct MPID_Request *req);
    int (* lmt_vc_terminated)(struct MPIDI_VC *vc);

    /* LMT shared memory copy-buffer ptr */
    struct MPID_nem_copy_buf *lmt_copy_buf;
    MPIU_SHMW_Hnd_t lmt_copy_buf_handle;
    MPIU_SHMW_Hnd_t lmt_recv_copy_buf_handle;
    int lmt_buf_num;
    MPIDI_msg_sz_t lmt_surfeit;
    struct {struct MPID_nem_lmt_shm_wait_element *head, *tail;} lmt_queue;
    struct MPID_nem_lmt_shm_wait_element *lmt_active_lmt;
    int lmt_enqueued; /* FIXME: used for debugging */
    MPID_nem_lmt_rts_queue_t lmt_rts_queue;

    /* Pointer to per-vc packet handlers */
    MPIDI_CH3_PktHandler_Fcn **pkt_handler;
    int num_pkt_handlers;
    
    union
    {
        char padding[MPIDI_NEM_VC_NETMOD_AREA_LEN];

        /* Temporary helper field for ticket #1679.  Should force proper pointer
         * alignment on finnicky platforms like SPARC.  Proper fix is to stop
         * this questionable type aliasing altogether. */
        void *align_helper;
    } netmod_area;
    

    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    void *sendq_head;
} MPIDI_CH3I_VC;

#define MPIDI_CH3_VC_DECL struct MPIDI_CH3I_VC ch;

struct MPIDI_CH3I_Request
{
    struct MPIDI_VC     *vc;
    int                  noncontig;
    MPIDI_msg_sz_t       header_sz;

    MPI_Request          lmt_req_id;     /* request id of remote side */
    struct MPID_Request *lmt_req;        /* pointer to original send/recv request */
    MPIDI_msg_sz_t       lmt_data_sz;    /* data size to be transferred, after checking for truncation */
    MPL_IOV             lmt_tmp_cookie; /* temporary storage for received cookie */
    void                *s_cookie;       /* temporary storage for the cookie data in case the packet can't be sent immediately */

    union
    {
        char padding[MPIDI_NEM_REQ_NETMOD_AREA_LEN];

        /* Temporary helper field for ticket #1679.  Should force proper pointer
         * alignment on finnicky platforms like SPARC.  Proper fix is to stop
         * this questionable type aliasing altogether. */
        void *align_helper;
    } netmod_area;
};

/*
 * MPIDI_CH3_REQUEST_DECL (additions to MPID_Request)
 */
#define MPIDI_CH3_REQUEST_DECL struct MPIDI_CH3I_Request ch;


#if 0
#define DUMP_REQUEST(req) do {                                                          \
        int i;                                                                          \
        MPIDI_DBG_PRINTF((55, FCNAME, "request %p\n", (req)));                          \
        MPIDI_DBG_PRINTF((55, FCNAME, "  handle = %d\n", (req)->handle));		\
        MPIDI_DBG_PRINTF((55, FCNAME, "  ref_count = %d\n", (req)->ref_count));         \
        MPIDI_DBG_PRINTF((55, FCNAME, "  cc = %d\n", (req)->cc));			\
        for (i = 0; i < (req)->iov_count; ++i)                                          \
            MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov[%d] = (%p, %d)\n", i,		\
                              (req)->dev.iov[i+(req)->dev.iov_offset].MPL_IOV_BUF,     \
                              (req)->dev.iov[i+(req)->dev.iov_offset].MPL_IOV_LEN));  \
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.iov_count = %d\n",                             \
                      (req)->dev.iov_count));                                           \
    MPIDI_DBG_PRINTF((55, FCNAME, "  dev.state = 0x%x\n", (req)->dev.state));           \
    MPIDI_DBG_PRINTF((55, FCNAME, "    type = %d\n",                                    \
		      MPIDI_Request_get_type(req)));                                    \
    } while (0)
#else
#define DUMP_REQUEST(req) do { } while (0)
#endif


#define MPIDI_POSTED_RECV_ENQUEUE_HOOK(req) MPIDI_CH3I_Posted_recv_enqueued(req)
#define MPIDI_POSTED_RECV_DEQUEUE_HOOK(req) MPIDI_CH3I_Posted_recv_dequeued(req)

/*
 * MPID_Progress_state - device/channel dependent state to be passed between 
 * MPID_Progress_{start,wait,end}
 *
 */
typedef struct MPIDI_CH3I_Progress_state
{
    int completion_count;
}
MPIDI_CH3I_Progress_state;

#define MPIDI_CH3_PROGRESS_STATE_DECL MPIDI_CH3I_Progress_state ch;

extern OPA_int_t MPIDI_CH3I_progress_completion_count;

#define MPIDI_CH3I_INCR_PROGRESS_COMPLETION_COUNT do {                                  \
        OPA_write_barrier();                                                            \
        OPA_incr_int(&MPIDI_CH3I_progress_completion_count);                            \
        MPIU_DBG_MSG_D(CH3_PROGRESS,VERBOSE,                                            \
                       "just incremented MPIDI_CH3I_progress_completion_count=%d",      \
                       OPA_load_int(&MPIDI_CH3I_progress_completion_count));            \
    } while(0)

#endif /* !defined(MPICH_MPIDI_CH3_PRE_H_INCLUDED) */

