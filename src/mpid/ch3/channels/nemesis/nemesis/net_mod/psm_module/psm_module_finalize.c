/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "psm_module_impl.h"
#include "psm.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_finalize()
{
   int mpi_errno = MPI_SUCCESS;

   if (MPID_nem_mem_region.ext_procs > 0)
   {
	psm_error_t ret ;
	
	while(MPID_nem_module_psm_pendings_sends > 0)
        {
              MPID_nem_psm_module_poll(MPID_NEM_POLL_OUT);
        }
        ret = psm_mq_finalize(MPID_nem_module_psm_mq);
	MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_mq_finalize", "**psm_mq_finalize %s", psm_error_get_string (ret));

        ret = psm_ep_close(MPID_nem_module_psm_local_endpoint, PSM_EP_CLOSE_GRACEFUL, 5 * SEC_IN_NS);
	MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_close_endpoint", "**psm_close_endpoint %s", psm_error_get_string (ret));

	MPIU_Free( MPID_nem_module_psm_endpoint_addrs );
        MPIU_Free( MPID_nem_module_psm_endpoint_ids );
        
        
	MPIU_Free( MPID_nem_module_psm_send_outstanding_request );
	MPIU_Free( MPID_nem_module_psm_recv_outstanding_request );
	
	ret = psm_finalize();
	MPIU_ERR_CHKANDJUMP1 (ret != PSM_OK, mpi_errno, MPI_ERR_OTHER, "**psm_finalize", "**psm_finalize %s", psm_error_get_string (ret));
   }   
   
   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_psm_module_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_psm_module_ckpt_shutdown ()
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
      return mpi_errno;
   fn_fail:
      goto fn_exit;
}

