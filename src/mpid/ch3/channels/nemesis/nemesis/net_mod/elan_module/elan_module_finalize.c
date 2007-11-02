/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "elan_module_impl.h"
#include <elan/elan.h>


#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_finalize()
{
   int mpi_errno = MPI_SUCCESS;
 
    if (MPID_nem_mem_region.ext_procs > 0)
     {
	 while ( !MPID_nem_elan_event_queue_empty(MPID_nem_module_elan_pending_event_queue))
	  {
	     MPID_nem_elan_module_poll(MPID_NEM_POLL_OUT);
	  }

#ifdef LIBELAN_HAVE_FINI
        if ( elan_base->capabilities & LIBELAN_HAVE_FINI )
	    elan_fini(elan_base->state, elan_base->allGroup, LIBELAN_FINI_GROUP);
#endif
	 
        elan_disable_network(elan_base->state);
	MPIU_Free(rxq_ptr_array);
	MPIU_Free(MPID_nem_module_elan_cells);
     }
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_elan_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_elan_module_ckpt_shutdown ()
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;
}

