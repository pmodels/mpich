/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    int ret ;
    
    while(MPID_nem_mx_pending_send_req > 0)
	MPID_nem_mx_poll(FALSE);

    ret = mx_close_endpoint(MPID_nem_mx_local_endpoint);
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_close_endpoint", "**mx_close_endpoint %s", mx_strerror (ret));
    
    ret = mx_finalize();
    MPIU_ERR_CHKANDJUMP1 (ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_finalize", "**mx_finalize %s", mx_strerror (ret));   

   fn_exit:
     return mpi_errno;
   fn_fail:
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_mx_ckpt_shutdown ()
{
      return MPI_SUCCESS;
}

