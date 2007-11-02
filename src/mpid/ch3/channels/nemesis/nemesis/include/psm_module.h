/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef PSM_MODULE_H
#define PSM_MODULE_H

int MPID_nem_psm_module_init (MPID_nem_queue_ptr_t proc_recv_queue, MPID_nem_queue_ptr_t proc_free_queue, MPID_nem_cell_ptr_t proc_elements,
		    int num_proc_elements, MPID_nem_cell_ptr_t module_elements, int num_module_elements,
		    MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		    MPIDI_PG_t *pg_p, int pg_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_psm_module_finalize (void);
int MPID_nem_psm_module_ckpt_shutdown (void);
int MPID_nem_psm_module_poll(MPID_nem_poll_dir_t in_or_out);
int MPID_nem_psm_module_send (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
int MPID_nem_psm_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p);
int MPID_nem_psm_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc);
int MPID_nem_psm_module_vc_init (MPIDI_VC_t *vc, const char *business_card);
int MPID_nem_psm_module_vc_destroy(MPIDI_VC_t *vc);
int MPID_nem_psm_module_vc_terminate (MPIDI_VC_t *vc);

int  MPID_nem_psm_module_test (void);

int  MPID_mem_psm_module_register_mem (void *p, int len);
int  MPID_nem_psm_module_deregister_mem (void *p, int len);

/* completion counter is atomically decremented when operation completes */
int  MPID_nem_psm_module_get (void *target_p, void *source_p, int len, MPIDI_VC_t *source_vc, int *completion_ctr);
int  MPID_nem_psm_module_put (void *target_p, void *source_p, int len, MPIDI_VC_t *target_vc, int *completion_ctr);

/* large message transfer functions */
int  MPID_nem_psm_module_lmt_send_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *dest, struct iovec *cookie);
int  MPID_nem_psm_module_lmt_recv_pre (struct iovec *iov, size_t n_iov, MPIDI_VC_t *src, struct iovec *cookie);
int  MPID_nem_psm_module_lmt_start_send (MPIDI_VC_t *dest, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_psm_module_lmt_start_recv (MPIDI_VC_t *src, struct iovec s_cookie, struct iovec r_cookie, int *completion_ctr);
int  MPID_nem_psm_module_lmt_send_post (struct iovec cookie);
int  MPID_nem_psm_module_lmt_recv_post (struct iovec cookie);


//#define LMT_COMPLETE 0
//#define LMT_FAILURE 1
//#define LMT_AGAIN 2

#endif
