/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"


static int nm_init(MPID_nem_queue_ptr_t proc_recv_queue, 
                   MPID_nem_queue_ptr_t proc_free_queue, 
                   MPID_nem_cell_ptr_t proc_elements,   int num_proc_elements,
                   MPID_nem_cell_ptr_t module_elements, int num_module_elements, 
                   MPID_nem_queue_ptr_t *module_free_queue, int ckpt_restart,
                   MPIDI_PG_t *pg_p, int pg_rank,
                   char **bc_val_p, int *val_max_sz_p)
{
    return MPI_SUCCESS;
}

static int nm_get_business_card(int my_rank, char **bc_val_p, int *val_max_sz_p)
{
    MPIU_Assertp(0);
    return MPI_SUCCESS;
}

static int nm_connect_to_root(const char *business_card, MPIDI_VC_t *new_vc)
{
    MPIU_Assertp(0);
    return MPI_SUCCESS;
}

static int nm_vc_init(MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}

static int nm_vc_destroy(MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}

static int nm_vc_terminate(MPIDI_VC_t *vc)
{
    return MPI_SUCCESS;
}

static int nm_send(MPIDI_VC_t *vc, MPID_nem_cell_ptr_t cell, int datalen)
{
    MPIU_Assertp(0);
    return MPI_SUCCESS;
}

static int nm_poll(int in_blocking_poll)
{  
    return MPI_SUCCESS;
}

static int nm_ckpt_shutdown(void)
{
  return MPI_SUCCESS;
}

static int nm_finalize(void)
{
  return nm_ckpt_shutdown();    
}

MPID_nem_netmod_funcs_t MPIDI_nem_none_funcs = {
    nm_init,
    nm_finalize,
    nm_ckpt_shutdown,
    nm_poll,
    nm_send,
    nm_get_business_card,
    nm_connect_to_root,
    nm_vc_init,
    nm_vc_destroy,
    nm_vc_terminate
};
