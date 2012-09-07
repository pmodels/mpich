/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PTL_IMPL_H
#define PTL_IMPL_H

#include <mpid_nem_impl.h>
#include <portals4.h>

#define PTL_MAX_EAGER (64*1024) /* 64K */

#define PTL_LARGE_THRESHOLD (64*1024) /* 64K */

extern ptl_handle_ni_t MPIDI_nem_ptl_ni;
extern ptl_pt_index_t  MPIDI_nem_ptl_pt;
extern ptl_pt_index_t  MPIDI_nem_ptl_control_pt; /* portal for MPICH control messages */
extern ptl_handle_eq_t MPIDI_nem_ptl_eq;

extern ptl_handle_md_t MPIDI_nem_ptl_global_md;

#define MPID_NEM_PTL_MAX_OVERFLOW_DATA 32 /* that's way more than we need */
typedef struct MPID_nem_ptl_pack_overflow
{
    MPI_Aint len;
    MPI_Aint offset;
    char buf[MPID_NEM_PTL_MAX_OVERFLOW_DATA];
} MPID_nem_ptl_pack_overflow_t;

typedef int (* event_handler_fn)(const ptl_event_t *e);

#define MPID_NEM_PTL_NUM_CHUNK_BUFFERS 2

typedef struct {
    struct MPID_nem_ptl_pack_overflow overflow[MPID_NEM_PTL_NUM_CHUNK_BUFFERS];
    int noncontig;
    int large;
    ptl_handle_md_t md;
    ptl_handle_me_t me;
    void *chunk_buffer[MPID_NEM_PTL_NUM_CHUNK_BUFFERS];
    MPIDI_msg_sz_t bytes_put;
    event_handler_fn ack_handler;
    event_handler_fn put_handler;
    event_handler_fn get_handler;
    event_handler_fn reply_handler;
} MPID_nem_ptl_req_area;

/* macro for ptl private in req */
#define REQ_PTL(req) ((MPID_nem_ptl_req_area *)((req)->ch.netmod_area.padding))

#define MPID_nem_ptl_init_req(req_) do {                        \
        int i;                                                  \
        for (i = 0; i < MPID_NEM_PTL_NUM_CHUNK_BUFFERS; ++i) {  \
            REQ_PTL(req)->overflow[i].len  = 0;                 \
            REQ_PTL(req_)->chunk_buffer[i] = NULL;              \
        }                                                       \
        REQ_PTL(req_)->noncontig     = FALSE;                   \
        REQ_PTL(req_)->large         = FALSE;                   \
        REQ_PTL(req_)->md            = PTL_INVALID_HANDLE;      \
        REQ_PTL(req_)->me            = PTL_INVALID_HANDLE;      \
        REQ_PTL(req_)->ack_handler   = NULL;                    \
        REQ_PTL(req_)->put_handler   = NULL;                    \
        REQ_PTL(req_)->get_handler   = NULL;                    \
        REQ_PTL(req_)->reply_handler = NULL;                    \
    } while (0)

#define MPID_nem_ptl_request_create_req(sreq_, errno_, on_fail_) do {   \
        MPIDI_Request_create_sreq(sreq_, errno_, on_fail_);             \
        MPID_nem_ptl_init_req(req_);                                    \
    } while (0)

typedef struct {
    ptl_process_t id;
    ptl_pt_index_t pt;
    ptl_pt_index_t ptc;
    int id_initialized; /* TRUE iff id and pt have been initialized */
    MPIDI_msg_sz_t num_queued_sends; /* number of reqs for this vc in sendq */
} MPID_nem_ptl_vc_area;

/* macro for ptl private in VC */
#define VC_PTL(vc) ((MPID_nem_ptl_vc_area *)VC_CH((vc))->netmod_area.padding)

/* Header bit fields
   bit   field
   ----  -------------------
   50    single/multiple
   49    large/small
   48    ssend
   24-47 match-bits for large messages (24 bits)
   0-23  source

   Note: This means we support no more than 2^24 processes.
*/
#define NPTL_SOURCE_OFFSET                          0
#define NPTL_MATCH_BITS_OFFSET                     24
#define NPTL_SSEND             ((ptl_hdr_data_t)1<<48)
#define NPTL_LARGE             ((ptl_hdr_data_t)1<<49)
#define NPTL_MULTIPLE          ((ptl_hdr_data_t)1<<50)

#define NPTL_SOURCE_MASK     ((((ptl_hdr_data_t)1<<24)-1)<<NPTL_SOURCE_OFFSET)
#define NPTL_MATCH_BITS_MASK ((((ptl_hdr_data_t)1<<24)-1)<<NPTL_MATCH_BITS_OFFSET)

#define NPTL_HEADER(flags, match_bits, source) ((flags) |                                                       \
                                                ((ptl_hdr_data_t)(match_bits))<<NPTL_MATCH_BITS_OFFSET |        \
                                                ((ptl_hdr_data_t)(source))<<NPTL_SOURCE_OFFSET)

#define NPTL_HEADER_SOURCE(hdr)     ((hdr & NPTL_SOURCE_MASK)>>NPTL_SOURCE_OFFSET)
#define NPTL_HEADER_MATCH_BITS(hdr) ((hdr & NPTL_MATCH_BITS_MASK)>>NPTL_MATCH_BITS_OFFSET)

/* This is the maximum number of processes we support */
#define NPTL_MAX_PROCS (NPTL_SOURCE_MASK+1)

/* create a match value */
#define NPTL_TAG_SHIFT 32 /* tag and ctx_id are limited to 32 bits each */
#define NPTL_MATCH(tag, context_id) (((ptl_match_bits)(tag) << NPTL_TAG_SHIFT)  | (ptl_match_bits)(context_id))
#define NPTL_MATCH_GET_TAG(match_bits) ((match_bits) >> NPTL_TAG_SHIFT)
#define NPTL_ANY_TAG_MASK NPTL_MATCH(~0, 0)

int MPID_nem_ptl_send_init(void);
int MPID_nem_ptl_send_finalize(void);
int MPID_nem_ptl_ev_send_handler(ptl_event_t *e);
int MPID_nem_ptl_sendq_complete_with_error(MPIDI_VC_t *vc, int req_errno);
int MPID_nem_ptl_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz);
int MPID_nem_ptl_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                   MPID_Request **sreq_ptr);
int MPID_nem_ptl_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                               void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_ptl_poll_init(void);
int MPID_nem_ptl_poll_finalize(void);
int MPID_nem_ptl_poll(int is_blocking_poll);
int MPID_nem_ptl_vc_terminated(MPIDI_VC_t *vc);
int MPID_nem_ptl_get_id_from_bc(const char *business_card, ptl_process_t *id, ptl_pt_index_t *pt, ptl_pt_index_t *ptc);

/* comm override functions */
int MPID_nem_ptl_recv_posted(struct MPIDI_VC *vc, struct MPID_Request *req);
/* isend is also used to implement send, rsend and irsend */
int MPID_nem_ptl_isend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                       MPID_Comm *comm, int context_offset, struct MPID_Request **request);
/* issend is also used to implement ssend */
int MPID_nem_ptl_issend(struct MPIDI_VC *vc, const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
                        MPID_Comm *comm, int context_offset, struct MPID_Request **request);
int MPID_nem_ptl_cancel_send(struct MPIDI_VC *vc,  struct MPID_Request *sreq);
int MPID_nem_ptl_cancel_recv(struct MPIDI_VC *vc,  struct MPID_Request *rreq);
int MPID_nem_ptl_probe(struct MPIDI_VC *vc,  int source, int tag, MPID_Comm *comm, int context_offset, MPI_Status *status);
int MPID_nem_ptl_iprobe(struct MPIDI_VC *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag,
                        MPI_Status *status);
int MPID_nem_ptl_improbe(struct MPIDI_VC *vc,  int source, int tag, MPID_Comm *comm, int context_offset, int *flag,
                         MPID_Request **message, MPI_Status *status);


#endif /* PTL_IMPL_H */
