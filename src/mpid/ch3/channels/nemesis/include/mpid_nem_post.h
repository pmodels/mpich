/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_POST_H_INCLUDED
#define MPID_NEM_POST_H_INCLUDED

#include "mpidu_shm.h"

struct MPIDI_PG;
union MPIDI_CH3_Pkt;

/* if local_lmt_pending is true, local_lmt_progress will be called in the
   progress loop */
extern int MPID_nem_local_lmt_pending;
extern int (*MPID_nem_local_lmt_progress)(void);

/* FIXME: Do not export these definitions all the way into mpiimpl.h (which
   will happen if this is included in mpidpost.h or mpidpre.h) */
int MPID_nem_init(int rank, struct MPIDI_PG *pg_p, int has_parent);
int MPID_nem_finalize(void);
int MPID_nem_vc_init(struct MPIDI_VC *vc);
int MPID_nem_vc_destroy(struct MPIDI_VC *vc);
int MPID_nem_get_business_card(int myRank, char *value, int length);
int MPID_nem_connect_to_root(const char *port_name, struct MPIDI_VC *new_vc);
int MPID_nem_lmt_shm_progress(void);
int MPID_nem_lmt_dma_progress(void);
int MPID_nem_lmt_vmsplice_progress(void);

#ifdef ENABLE_CHECKPOINTING
extern int MPIDI_nem_ckpt_start_checkpoint;
extern int MPIDI_nem_ckpt_finish_checkpoint;
int MPIDI_nem_ckpt_init(void);
int MPIDI_nem_ckpt_finalize(void);
int MPIDI_nem_ckpt_start(void);
int MPIDI_nem_ckpt_finish(void);
int MPIDI_nem_ckpt_pkthandler_init(MPIDI_CH3_PktHandler_Fcn *pktArray[], int arraySize);
#endif

/* one-sided */

typedef struct MPID_nem_mpich_win
{
    char *handle;               /* shared-memory segment handle */
    int   proc;                 /* rank of owner */
    void *home_address;         /* address of window at owner */
    int   len;                  /* size of window */
    void *local_address;        /* address of window at this process */
}
MPID_nem_mpich_win_t;

#if !defined (MPID_NEM_INLINE) || !MPID_NEM_INLINE
int MPID_nem_mpich_send_header(void* buf, int size, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich_sendv(MPL_IOV **iov, int *n_iov, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich_sendv_header(MPL_IOV **iov, int *n_iov, void *ext_header,
                                intptr_t ext_header_sz, struct MPIDI_VC *vc, int *again);
void MPID_nem_mpich_send_seg(MPIR_Segment segment, intptr_t *segment_first, intptr_t segment_sz, struct MPIDI_VC *vc, int *again);
void MPID_nem_mpich_send_seg_header(MPIR_Segment segment, intptr_t *segment_first, intptr_t segment_size,
                                    void *header, intptr_t header_sz, void *ext_header,
                                    intptr_t ext_header_sz, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich_test_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox, int in_blocking_progress);
int MPID_nem_mpich_test_recv_wait(MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);
int MPID_nem_recv_seqno_matches(MPID_nem_queue_ptr_t qhead) ;
int MPID_nem_mpich_blocking_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox);
int MPID_nem_mpich_release_cell(MPID_nem_cell_ptr_t cell, struct MPIDI_VC *vc);
int MPID_nem_mpich_enqueue_fastbox(int local_rank);
int MPID_nem_mpich_dequeue_fastbox(int local_rank);
#endif /* MPID_NEM_INLINE */


#endif /* MPID_NEM_POST_H_INCLUDED */
