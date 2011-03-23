/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
 */

#include "newmad_impl.h"
#include "my_papi_defs.h"

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_cancel_send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_cancel_send(MPIDI_VC_t *vc, MPID_Request *sreq)
{
    nm_sr_request_t *nmad_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    int ret;
    
    nmad_req = &(REQ_FIELD(sreq,newmad_req));    
    ret = nm_sr_scancel(mpid_nem_newmad_session,nmad_req);

    if (ret ==  NM_ESUCCESS)
    {
        sreq->status.cancelled = TRUE;
	mpid_nem_newmad_pending_send_req--;
    }
    else
    {	    
        sreq->status.cancelled = FALSE;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_cancel_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_newmad_cancel_recv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    nm_sr_request_t *nmad_req = NULL;
    int mpi_errno = MPI_SUCCESS;
    int ret;
    
    nmad_req = &(REQ_FIELD(rreq,newmad_req));    
    ret = nm_sr_rcancel(mpid_nem_newmad_session,nmad_req);

    if (ret ==  NM_ESUCCESS)
    {	    
        rreq->status.cancelled = TRUE;
    }
    else
    {
        rreq->status.cancelled = FALSE;
    }

 fn_exit:
    return mpi_errno;
 fn_fail:  ATTRIBUTE((unused))
    goto fn_exit;
}
