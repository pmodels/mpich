/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mx_impl.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_cancel_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_cancel_send(MPIDI_VC_t *vc, MPID_Request *sreq)
{
    mx_request_t *mx_request = NULL;
    mx_return_t ret;
    uint32_t    result;
    int mpi_errno = MPI_SUCCESS;
    
    mx_request = &(REQ_FIELD(sreq,mx_request));
    ret = mx_cancel(MPID_nem_mx_local_endpoint,mx_request,&result);
    MPIU_ERR_CHKANDJUMP1(ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_cancel", "**mx_cancel %s", mx_strerror(ret));

    if (result)
    {
        sreq->status.cancelled = TRUE;
        MPID_nem_mx_pending_send_req--;
    }
    else
    {	    
        sreq->status.cancelled = FALSE;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_cancel_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_cancel_recv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    mx_request_t *mx_request = NULL;
    mx_return_t ret;
    uint32_t    result;
    int mpi_errno = MPI_SUCCESS;
    
    mx_request = &(REQ_FIELD(rreq,mx_request));
    ret = mx_cancel(MPID_nem_mx_local_endpoint,mx_request,&result);
    MPIU_ERR_CHKANDJUMP1(ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_cancel", "**mx_cancel %s", mx_strerror(ret));

    if (result)
    {	    
        rreq->status.cancelled = TRUE;
    }
    else
    {
        rreq->status.cancelled = FALSE;
    }

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
