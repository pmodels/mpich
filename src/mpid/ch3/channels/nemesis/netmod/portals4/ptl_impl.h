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

ptl_handle_ni_t MPIDI_nem_ptl_ni;
ptl_pt_index_t  MPIDI_nem_ptl_pt;
ptl_handle_eq_t MPIDI_nem_ptl_eq;

#define MPID_NEM_PTL_MAX_OVERFLOW_DATA 32 /* that's way more than we need */
typedef struct MPID_nem_ptl_pack_overflow
{
    MPI_Aint len;
    MPI_Aint offset;
    char buf[MPID_NEM_PTL_MAX_OVERFLOW_DATA];
} MPID_nem_ptl_pack_overflow_t;

typedef struct {
    struct MPID_nem_ptl_pack_overflow overflow;
    int noncontig;
} MPID_nem_ptl_req_area;

/* macro for ptl private in req */
#define REQ_PTL(req) ((MPID_nem_ptl_req_area *)((req)->ch.netmod_area.padding))


typedef struct {
    ptl_process_t id;
    ptl_pt_index_t pt;
    int id_initialized; /* TRUE iff id and pt have been initialized */
    MPIDI_msg_sz_t num_queued_sends; /* number of reqs for this vc in sendq */
} MPID_nem_ptl_vc_area;

/* macro for ptl private in VC */
#define VC_PTL(vc) ((MPID_nem_ptl_vc_area *)VC_CH((vc))->netmod_area.padding)

struct MPID_nem_ptl_sendbuf;

int MPID_nem_ptl_send_init(void);
int MPID_nem_ptl_send_finalize(void);
int MPID_nem_ptl_send_completed(struct MPID_nem_ptl_sendbuf *sb);
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
int MPID_nem_ptl_get_id_from_bc(const char *business_card, ptl_process_t *id, ptl_pt_index_t *pt);

#endif /* PTL_IMPL_H */
