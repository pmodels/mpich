/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PTLNM_IMPL_H
#define PTLNM_IMPL_H

#include <mpid_nem_impl.h>
#include <portals4.h>

#define PTLNM_MAX_EAGER (64*1024) /* 64K */

ptl_handle_ni_t MPIDI_nem_ptlnm_ni;
ptl_pt_index_t  MPIDI_nem_ptlnm_pt;
ptl_handle_eq_t MPIDI_nem_ptlnm_eq;

#define MPID_NEM_PTLNM_MAX_OVERFLOW_DATA 32 /* that's way more than we need */
typedef struct MPID_nem_ptlnm_pack_overflow
{
    MPI_Aint len;
    MPI_Aint offset;
    char buf[MPID_NEM_PTLNM_MAX_OVERFLOW_DATA];
} MPID_nem_ptlnm_pack_overflow_t;

typedef struct {
    struct MPID_nem_ptlnm_pack_overflow overflow;
    int noncontig;
} MPID_nem_ptlnm_req_area;

/* macro for ptlnm private in req */
#define REQ_PTLNM(req) ((MPID_nem_ptlnm_req_area *)((req)->ch.netmod_area.padding))


typedef struct {
    ptl_process_t id;
    ptl_pt_index_t pt;
    int id_initialized; /* TRUE iff id and pt have been initialized */
    MPIDI_msg_sz_t num_queued_sends; /* number of reqs for this vc in sendq */
} MPID_nem_ptlnm_vc_area;

/* macro for ptlnm private in VC */
#define VC_PTLNM(vc) ((MPID_nem_ptlnm_vc_area *)VC_CH((vc))->netmod_area.padding)

struct MPID_nem_ptlnm_sendbuf;

int MPID_nem_ptlnm_send_init(void);
int MPID_nem_ptlnm_send_finalize(void);
int MPID_nem_ptlnm_send_completed(struct MPID_nem_ptlnm_sendbuf *sb);
int MPID_nem_ptlnm_sendq_complete_with_error(MPIDI_VC_t *vc, int req_errno);
int MPID_nem_ptlnm_SendNoncontig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz);
int MPID_nem_ptlnm_iStartContigMsg(MPIDI_VC_t *vc, void *hdr, MPIDI_msg_sz_t hdr_sz, void *data, MPIDI_msg_sz_t data_sz,
                                   MPID_Request **sreq_ptr);
int MPID_nem_ptlnm_iSendContig(MPIDI_VC_t *vc, MPID_Request *sreq, void *hdr, MPIDI_msg_sz_t hdr_sz,
                               void *data, MPIDI_msg_sz_t data_sz);
int MPID_nem_ptlnm_poll_init(void);
int MPID_nem_ptlnm_poll_finalize(void);
int MPID_nem_ptlnm_poll(int is_blocking_poll);
int MPID_nem_ptlnm_vc_terminated(MPIDI_VC_t *vc);
int MPID_nem_ptlnm_get_id_from_bc(const char *business_card, ptl_process_t *id, ptl_pt_index_t *pt);

#endif /* PTLNM_IMPL_H */
