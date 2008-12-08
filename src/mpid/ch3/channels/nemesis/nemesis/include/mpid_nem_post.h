/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_POST_H
#define MPID_NEM_POST_H

struct MPIDI_PG;

/* FIXME: Do not export these definitions all the way into mpiimpl.h (which
   will happen if this is included in mpidpost.h or mpidpre.h) */
int MPID_nem_init(int rank, struct MPIDI_PG *pg_p, int has_parent);
int _MPID_nem_init(int rank, struct MPIDI_PG *pg_p, int ckpt_restart, int has_parent);
int MPID_nem_finalize(void);
int MPID_nem_ckpt_shutdown(void);
int MPID_nem_barrier_init(int num_processes, MPID_nem_barrier_t *barrier_region);
int MPID_nem_barrier(int num_processes, int rank);
int MPID_nem_vc_init(struct MPIDI_VC *vc);
int MPID_nem_vc_destroy(struct MPIDI_VC *vc);
int MPID_nem_get_business_card(int myRank, char *value, int length);
int MPID_nem_connect_to_root(const char *port_name, struct MPIDI_VC *new_vc);
int MPID_nem_lmt_shm_progress(void);
int MPID_nem_vc_terminate(struct MPIDI_VC *vc);

#ifdef ENABLED_CHECKPOINTING
int MPID_nem_ckpt_init (int ckpt_restart);
void MPID_nem_ckpt_finalize (void);
void MPID_nem_ckpt_maybe_take_checkpoint (void);
void MPID_nem_ckpt_got_marker (MPID_nem_cell_ptr_t *cell, int *in_fbox);
void MPID_nem_ckpt_log_message (MPID_nem_cell_ptr_t cell);
void MPID_nem_ckpt_send_markers (void);
int MPID_nem_ckpt_replay_message (MPID_nem_cell_ptr_t *cell);
void MPID_nem_ckpt_free_msg_log (void);
#endif


/* one-sided */

typedef struct MPID_nem_mpich2_win
{
    char *handle;               /* shared-memory segment handle */
    int   proc;                 /* rank of owner */
    void *home_address;         /* address of window at owner */
    int   len;                  /* size of window */
    void *local_address;        /* address of window at this process */
}
MPID_nem_mpich2_win_t;

int MPID_nem_mpich2_alloc_win (void **buf, int len, MPID_nem_mpich2_win_t **win);
int MPID_nem_mpich2_free_win (MPID_nem_mpich2_win_t *win);
int MPID_nem_mpich2_attach_win (void **buf, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_detach_win (MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_serialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t *win, int *len);
int MPID_nem_mpich2_deserialize_win (void *buf, int buf_len, MPID_nem_mpich2_win_t **win);

int MPID_nem_mpich2_win_put (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_get (void *s_buf, void *d_buf, int len, MPID_nem_mpich2_win_t *remote_win);
int MPID_nem_mpich2_win_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, MPID_nem_mpich2_win_t *remote_win);

int MPID_nem_mpich2_register_memory (void *buf, int len);
int MPID_nem_mpich2_deregister_memory (void *buf, int len);

int MPID_nem_mpich2_put (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr);
int MPID_nem_mpich2_putv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		       int *completion_ctr);
int MPID_nem_mpich2_get (void *s_buf, void *d_buf, int len, int proc, int *completion_ctr);
int MPID_nem_mpich2_getv (struct iovec **s_iov, int *s_niov, struct iovec **d_iov, int *d_niov, int proc,
		       int *completion_ctr);

     
#define MPID_nem_mpich2_win_put_val(val, d_buf, remote_win) do {			\
    char *_d_buf = d_buf;								\
											\
    _d_buf += (char *)(remote_win)->local_address - (char *)(remote_win)->home_address;	\
											\
    *(typeof (val) *)_d_buf = val;							\
} while (0)
     
#define MPID_nem_mpich2_win_get_val(s_buf, val, remote_win) do {			\
    char *_s_buf = s_buf;								\
											\
    _s_buf += (char *)(remote_win)->local_address - (char *)(remote_win)->home_address;	\
											\
    *(val) = *(typeof (*(val)) *)_s_buf;						\
} while (0)


#if !defined (MPID_NEM_INLINE) || !MPID_NEM_INLINE
int MPID_nem_mpich2_send_ckpt_marker(unsigned short wave, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich2_send_header(void* buf, int size, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich2_sendv(struct iovec **iov, int *n_iov, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich2_sendv_header(struct iovec **iov, int *n_iov, struct MPIDI_VC *vc, int *again);
void MPID_nem_mpich2_send_seg(MPID_Segment segment, MPIDI_msg_sz_t *segment_first, MPIDI_msg_sz_t segment_sz, struct MPIDI_VC *vc, int *again);
void MPID_nem_mpich2_send_seg_header(MPID_Segment segment, MPIDI_msg_sz_t *segment_first, MPIDI_msg_sz_t segment_size,
                                     void *header, MPIDI_msg_sz_t header_sz, struct MPIDI_VC *vc, int *again);
int MPID_nem_mpich2_test_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox);
int MPID_nem_mpich2_test_recv_wait(MPID_nem_cell_ptr_t *cell, int *in_fbox, int timeout);
int MPID_nem_recv_seqno_matches(MPID_nem_queue_ptr_t qhead) ;
int MPID_nem_mpich2_blocking_recv(MPID_nem_cell_ptr_t *cell, int *in_fbox);
int MPID_nem_mpich2_release_cell(MPID_nem_cell_ptr_t cell, struct MPIDI_VC *vc);
int MPID_nem_mpich2_enqueue_fastbox(int local_rank);
int MPID_nem_mpich2_dequeue_fastbox(int local_rank);
#endif /* MPID_NEM_INLINE */


#endif /* MPID_NEM_POST_H */
