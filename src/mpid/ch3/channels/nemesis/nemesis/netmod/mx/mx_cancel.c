/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */



#include "mx_impl.h"
#include "my_papi_defs.h"

/* code in mpid_cancel_send */
/* 
 #ifdef ENABLE_COMM_OVERRIDES                  
 if (vc->comm_ops && vc->comm_ops->cancel_send)
 {
    int handled;
    handled = vc->comm_ops->cancel_send(vc, sreq);
    if (handled)
      goto fn_exit;
 }
 #endif
*/

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
    int handled = FALSE;
   
     if (!VC_CH(vc)->is_local)
     {
	mx_request = &(REQ_FIELD(sreq,mx_request));
	ret = mx_cancel(MPID_nem_mx_local_endpoint,mx_request,&result);
	MPIU_ERR_CHKANDJUMP1(ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_cancel", "**mx_cancel %s", mx_strerror(ret));
	
	if (result)
	{
	   sreq->status.cancelled = TRUE;
	   sreq->cc = 0;
	   MPIU_Object_set_ref(sreq, 1);       
	   MPID_nem_mx_pending_send_req--;
	}
	else
        {	    
	   sreq->status.cancelled = FALSE;
	}
	handled = TRUE;
     }
   
 fn_exit:
    return handled;
 fn_fail:
    goto fn_exit;
}


/* code in cancel_recv */
/* FIXME: The vc is only needed to find which function to call*/
/* This is otherwise any_source ready */
/*
#ifdef ENABLE_COMM_OVERRIDES
 {                                                              
      MPIDI_VC_t *vc;
      MPIU_Assert(rreq->dev.match.parts.rank != MPI_ANY_SOURCE);
      MPIDI_Comm_get_vc_set_active(rreq->comm, rreq->dev.match.parts.rank, &vc);
      if (vc->comm_ops && vc->comm_ops->cancel_recv)
      {
         int handled;
         handled = vc->comm_ops->cancel_recv(NULL, rreq);
         if (handled)
         {
            MPIDI_FUNC_EXIT(MPID_STATE_MPID_CANCEL_RECV);
            return MPI_SUCCESS;
         }
      }
  }
  #endif
*/

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_cancel_recv
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_cancel_recv(MPIDI_VC_t *vc, MPID_Request *rreq)
{
    mx_request_t *mx_request = NULL;
    mx_return_t   ret;
    uint32_t      result;
    int           mpi_errno = MPI_SUCCESS;
    int           handled = FALSE;
   
    mx_request = &(REQ_FIELD(rreq,mx_request));
    /* FIXME this test is probably not correct with multiple netmods        */
    /* We need to know to which netmod a recv request actually "belongs" to */
    if(mx_request != NULL)
    {
       ret = mx_cancel(MPID_nem_mx_local_endpoint,mx_request,&result);
       MPIU_ERR_CHKANDJUMP1(ret != MX_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**mx_cancel", "**mx_cancel %s", mx_strerror(ret));
       
       if (result)
       {	    
	  int found;
	  rreq->status.cancelled = TRUE;
	  found = MPIDI_CH3U_Recvq_DP(rreq);
	  MPIU_Assert(found);
	  rreq->status.count = 0;
	  MPID_REQUEST_SET_COMPLETED(rreq);
	  MPID_Request_release(rreq);       
       }
       else
       {
	  rreq->status.cancelled = FALSE;
	  MPIU_DBG_MSG_P(CH3_OTHER,VERBOSE,
			 "request 0x%08x already matched, unable to cancel", rreq->handle);
       }
       handled = TRUE;
     }
   
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
