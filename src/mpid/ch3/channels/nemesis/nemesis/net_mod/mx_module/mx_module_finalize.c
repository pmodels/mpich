/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_module_impl.h"
#include "myriexpress.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_finalize()
{
   int mpi_errno = MPI_SUCCESS;

   if (MPID_nem_mem_region.ext_procs > 0)
     {
	int ret ;
	
	while(MPID_nem_module_mx_pendings_sends > 0)
	  {
	     MPID_nem_mx_module_poll(MPID_NEM_POLL_OUT);
	  }
	ret = mx_close_endpoint(MPID_nem_module_mx_local_endpoint);
	MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_close_endpoint", "**mx_close_endpoint %s", mx_strerror (ret));

	MPIU_Free( MPID_nem_module_mx_endpoints_addr );
	MPIU_Free( MPID_nem_module_mx_send_outstanding_request );      
	MPIU_Free( MPID_nem_module_mx_recv_outstanding_request );      
	
	ret = mx_finalize();
	MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_finalize", "**mx_finalize %s", mx_strerror (ret));
     }   
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_module_ckpt_shutdown ()
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;
}

