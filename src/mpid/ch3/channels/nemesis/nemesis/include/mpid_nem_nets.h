/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPID_NEM_NETS_H
#define MPID_NEM_NETS_H

typedef int (* MPID_nem_net_module_init_t) (MPID_nem_queue_ptr_t proc_recv_queue, 
                                            MPID_nem_queue_ptr_t proc_free_queue, 
                                            MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
                                            MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
                                            MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
                                            MPIDI_PG_t *pg_p, int pg_rank,
                                            char **bc_val_p, int *val_max_sz_p);     
typedef int (* MPID_nem_net_module_finalize_t) (void);
typedef int (* MPID_nem_net_module_ckpt_shutdown_t) (void);
typedef int (* MPID_nem_net_module_poll_t) (MPID_nem_poll_dir_t in_or_out);
typedef int (* MPID_nem_net_module_send_t) (MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen);
typedef int (* MPID_nem_net_module_get_business_card_t) (int my_rank, char **bc_val_p, int *val_max_sz_p);
typedef int (* MPID_nem_net_module_connect_to_root_t) (const char *business_card, MPIDI_VC_t *new_vc);
typedef int (* MPID_nem_net_module_vc_init_t) (MPIDI_VC_t *vc);
typedef int (* MPID_nem_net_module_vc_destroy_t) (MPIDI_VC_t *vc);
typedef int (* MPID_nem_net_module_vc_terminate_t) (MPIDI_VC_t *vc);

typedef void (* MPID_nem_net_module_vc_dbg_print_sendq_t) (FILE *stream, MPIDI_VC_t *vc);

extern MPID_nem_net_module_init_t MPID_nem_net_module_init;
extern MPID_nem_net_module_finalize_t MPID_nem_net_module_finalize;
extern MPID_nem_net_module_ckpt_shutdown_t MPID_nem_net_module_ckpt_shutdown;
extern MPID_nem_net_module_poll_t MPID_nem_net_module_poll;
extern MPID_nem_net_module_send_t MPID_nem_net_module_send;
extern MPID_nem_net_module_get_business_card_t MPID_nem_net_module_get_business_card;
extern MPID_nem_net_module_connect_to_root_t MPID_nem_net_module_connect_to_root;
extern MPID_nem_net_module_vc_init_t MPID_nem_net_module_vc_init;
extern MPID_nem_net_module_vc_destroy_t MPID_nem_net_module_vc_destroy;
extern MPID_nem_net_module_vc_terminate_t MPID_nem_net_module_vc_terminate;
extern MPID_nem_net_module_vc_dbg_print_sendq_t  MPID_nem_net_module_vc_dbg_print_sendq;

int MPID_nem_net_init(void);

#endif /* MPID_NEM_NETS_H */
