/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef IB_MODULE_H
#define IB_MODULE_H

int MPID_nem_ib_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
        MPID_nem_queue_ptr_t proc_free_queue, 
        MPID_nem_cell_ptr_t proc_elements,
        int num_proc_elements, 
        MPID_nem_cell_ptr_t module_elements, 
        int num_module_elements,
        MPID_nem_queue_ptr_t *module_free_queue, 
        int ckpt_restart,
        MPIDI_PG_t *pg_p, 
        int pg_rank, 
        char **bc_val_p, 
        int *val_max_sz_p);

int MPID_nem_ib_module_finalize (void);

int MPID_nem_ib_module_poll (MPID_nem_poll_dir_t in_or_out);

int MPID_nem_ib_module_send (MPIDI_VC_t *vc, 
        MPID_nem_cell_ptr_t cell, 
        int datalen);

int MPID_nem_ib_module_get_business_card (int my_rank, char **bc_val_p, 
        int *val_max_sz_p);

int MPID_nem_ib_module_ckpt_shutdown (void);

int MPID_nem_ib_module_connect_to_root (const char *business_card, 
        MPIDI_VC_t *new_vc);

int MPID_nem_ib_module_vc_init (MPIDI_VC_t *vc);

int MPID_nem_ib_module_vc_destroy(MPIDI_VC_t *vc);

int MPID_nem_ib_module_vc_terminate (MPIDI_VC_t *vc);

#endif  /* IB_MODULE_H */
