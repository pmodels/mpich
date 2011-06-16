/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED)
#define MPICH_MPIDI_CH3_IMPL_H_INCLUDED

#include "mpidi_ch3_conf.h"
#include "mpidimpl.h"
#include "mpiu_os_wrappers.h"
#include "mpid_nem_generic_queue.h"

#if defined(HAVE_ASSERT_H)
#include <assert.h>
#endif

extern void *MPIDI_CH3_packet_buffer;
extern int MPIDI_CH3I_my_rank;

typedef GENERIC_Q_DECL(struct MPID_Request) MPIDI_CH3I_shm_sendq_t;
extern MPIDI_CH3I_shm_sendq_t MPIDI_CH3I_shm_sendq;
extern struct MPID_Request *MPIDI_CH3I_shm_active_send;

/* Send queue macros */
/* MT - not thread safe! */
#define MPIDI_CH3I_Sendq_empty(q) GENERIC_Q_EMPTY (q)
#define MPIDI_CH3I_Sendq_head(q) GENERIC_Q_HEAD (q)
#define MPIDI_CH3I_Sendq_enqueue(qp, ep) do {                                           \
        /* add refcount so req doesn't get freed before it's dequeued */                \
        MPIR_Request_add_ref(ep);                                                       \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPIDI_CH3I_Sendq_enqueue req=%p (handle=%#x), queue=%p",     \
                          ep, (ep)->handle, qp));                                       \
        GENERIC_Q_ENQUEUE (qp, ep, dev.next);                                           \
    } while (0)
#define MPIDI_CH3I_Sendq_dequeue(qp, ep)  do {                                          \
        GENERIC_Q_DEQUEUE (qp, ep, dev.next);                                           \
        MPIU_DBG_MSG_FMT(CH3_CHANNEL, VERBOSE, (MPIU_DBG_FDEST,                         \
                          "MPIDI_CH3I_Sendq_dequeuereq=%p (handle=%#x), queue=%p",      \
                          *(ep), *(ep) ? (*(ep))->handle : -1, qp));                    \
        MPID_Request_release(*(ep));                                                    \
    } while (0)
#define MPIDI_CH3I_Sendq_enqueue_multiple_no_refcount(qp, ep0, ep1)             \
    /* used to move reqs from one queue to another, so we don't update */       \
    /* the refcounts */                                                         \
    GENERIC_Q_ENQUEUE_MULTIPLE(qp, ep0, ep1, dev.next)

int MPIDI_CH3I_Progress_init(void);
int MPIDI_CH3I_Progress_finalize(void);
int MPIDI_CH3I_Shm_send_progress(void);
int MPIDI_CH3I_Complete_sendq_with_error(MPIDI_VC_t * vc);

int MPIDI_CH3I_SendNoncontig( MPIDI_VC_t *vc, MPID_Request *sreq, void *header, MPIDI_msg_sz_t hdr_sz );

int MPID_nem_lmt_shm_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPID_Request *req);
int MPID_nem_lmt_shm_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie);
int MPID_nem_lmt_shm_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie);
int MPID_nem_lmt_shm_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie);
int MPID_nem_lmt_shm_done_send(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_shm_done_recv(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_shm_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_lmt_dma_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPID_Request *req);
int MPID_nem_lmt_dma_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie);
int MPID_nem_lmt_dma_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie);
int MPID_nem_lmt_dma_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie);
int MPID_nem_lmt_dma_done_send(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_dma_done_recv(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_dma_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_lmt_vmsplice_initiate_lmt(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *rts_pkt, MPID_Request *req);
int MPID_nem_lmt_vmsplice_start_recv(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV s_cookie);
int MPID_nem_lmt_vmsplice_start_send(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV r_cookie);
int MPID_nem_lmt_vmsplice_handle_cookie(MPIDI_VC_t *vc, MPID_Request *req, MPID_IOV cookie);
int MPID_nem_lmt_vmsplice_done_send(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_vmsplice_done_recv(MPIDI_VC_t *vc, MPID_Request *req);
int MPID_nem_lmt_vmsplice_vc_terminated(MPIDI_VC_t *vc);

int MPID_nem_handle_pkt(MPIDI_VC_t *vc, char *buf, MPIDI_msg_sz_t buflen);

/* macro to access the channel_private fields in the vc */
#define VC_CH(vc) ((MPIDI_CH3I_VC *)(vc)->channel_private)

struct MPIDI_VC;
struct MPID_Request;
struct MPID_nem_copy_buf;
union MPIDI_CH3_Pkt;
struct MPID_nem_lmt_shm_wait_element;
struct MPIDI_CH3_PktGeneric;

typedef struct MPIDI_CH3I_VC
{
    int pg_rank;
    struct MPID_Request *recv_active;

    int is_local;
    unsigned short send_seqno;
    MPID_nem_fbox_mpich2_t *fbox_out;
    MPID_nem_fbox_mpich2_t *fbox_in;
    MPID_nem_queue_ptr_t recv_queue;
    MPID_nem_queue_ptr_t free_queue;

#ifdef ENABLE_CHECKPOINTING
    MPIDI_msg_sz_t ckpt_msg_len;
    void *ckpt_msg_buf;
#endif

    /* temp buffer to store partially received header */
    MPIDI_msg_sz_t pending_pkt_len;
    struct MPIDI_CH3_PktGeneric *pending_pkt;

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
    int (* lmt_start_recv)(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV s_cookie);
    int (* lmt_start_send)(struct MPIDI_VC *vc, struct MPID_Request *sreq, MPID_IOV r_cookie);
    int (* lmt_handle_cookie)(struct MPIDI_VC *vc, struct MPID_Request *req, MPID_IOV cookie);
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

    /* Pointer to per-vc packet handlers */
    MPIDI_CH3_PktHandler_Fcn **pkt_handler;
    int num_pkt_handlers;
    
    struct
    {
        char padding[MPID_NEM_VC_NETMOD_AREA_LEN];
    } netmod_area;
    

    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    void *sendq_head;
} MPIDI_CH3I_VC;


#endif /* !defined(MPICH_MPIDI_CH3_IMPL_H_INCLUDED) */
