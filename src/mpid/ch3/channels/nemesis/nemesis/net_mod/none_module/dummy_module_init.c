/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <dummy_module.h>

int
MPID_nem_dummy_module_init (MPID_nem_queue_ptr_t proc_recv_queue, 
		   MPID_nem_queue_ptr_t proc_free_queue, 
		   MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
		   MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
		   MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
		   MPIDI_PG_t *pg_p, int pg_rank,
		   char **bc_val_p, int *val_max_sz_p)
{
    return MPI_SUCCESS;
}

int
MPID_nem_dummy_module_get_business_card (int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    MPIU_Assertp (0);
    return MPI_SUCCESS;
}

int
MPID_nem_dummy_module_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    MPIU_Assertp (0);
    return MPI_SUCCESS;
}

int
MPID_nem_dummy_module_vc_init (MPIDI_VC_t *vc, const char *business_card)
{
    return MPI_SUCCESS;
}

int
MPID_nem_dummy_module_vc_destroy(MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}

int MPID_nem_dummy_module_vc_terminate (MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}
