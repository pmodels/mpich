/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * (C) 2006 by Argonne National Laboratory.
 *     See COPYRIGHT in top-level directory. 
 * 
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de           
 * Bordeaux. All rights reserved. Permission is hereby granted to use,          
 * reproduce, prepare derivative works, and to redistribute to others.          
 */


#include "mx_impl.h"

#define NUM_REQ (1024)

static MPID_nem_mx_internal_req_t *MPID_nem_mx_internal_req_queue_head;

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_internal_req_queue_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_internal_req_queue_init(void)
{
    MPID_nem_mx_internal_req_t *curr_req;
    MPID_nem_mx_internal_req_t *next_req;
    int mpi_errno = MPI_SUCCESS;
    int index;
    
    MPID_nem_mx_internal_req_queue_head = (MPID_nem_mx_internal_req_t *)MPIU_Malloc(sizeof(MPID_nem_mx_internal_req_t));
    curr_req = MPID_nem_mx_internal_req_queue_head;

    for(index = 0 ; index < (NUM_REQ - 1) ; index++)
    {
	next_req = (MPID_nem_mx_internal_req_t *)MPIU_Malloc(sizeof(MPID_nem_mx_internal_req_t));
	curr_req->next = next_req;
	curr_req = next_req;
    }
    curr_req->next = NULL;
   
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_internal_req_queue_destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_internal_req_queue_destroy(void)
{
    MPID_nem_mx_internal_req_t *curr_req;
    MPID_nem_mx_internal_req_t *req_to_del;
    int mpi_errno = MPI_SUCCESS;

    curr_req = MPID_nem_mx_internal_req_queue_head;
    
    if(curr_req->next == NULL)
    {
	MPIU_Free(curr_req);
    }
    else
    {
	do
	{
	    req_to_del = curr_req;
	    curr_req = curr_req->next;
	    MPIU_Free(req_to_del);
	}
	while(curr_req != NULL);
    }    
    
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_internal_req_dequeue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_internal_req_dequeue(MPID_nem_mx_internal_req_t **param_req)
{
   int mpi_errno = MPI_SUCCESS;

   *param_req = MPID_nem_mx_internal_req_queue_head;   
   MPID_nem_mx_internal_req_queue_head = (*param_req)->next;
   (*param_req)->next = NULL;
   
   if(MPID_nem_mx_internal_req_queue_head == NULL)
   {
       MPID_nem_mx_internal_req_t *curr_req;
       MPID_nem_mx_internal_req_t *next_req;
       int index;

       MPID_nem_mx_internal_req_queue_head = (MPID_nem_mx_internal_req_t *)MPIU_Malloc(sizeof(MPID_nem_mx_internal_req_t));
       curr_req = MPID_nem_mx_internal_req_queue_head;
       for(index = 0 ; index < (NUM_REQ - 1) ; index++)
       {
	   next_req = (MPID_nem_mx_internal_req_t *)MPIU_Malloc(sizeof(MPID_nem_mx_internal_req_t));
	   curr_req->next = next_req;
	   curr_req = next_req;
       }       
      curr_req->next = NULL;
   }

 fn_exit:
   return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
   goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPID_nem_mx_internal_req_enqueue
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_mx_internal_req_enqueue(MPID_nem_mx_internal_req_t *req)
{
    int mpi_errno = MPI_SUCCESS;
    
    MPIU_Assert(MPID_nem_mx_internal_req_queue_head != NULL);
    req->next = MPID_nem_mx_internal_req_queue_head;
    MPID_nem_mx_internal_req_queue_head = req;
    
 fn_exit:
    return mpi_errno;
 fn_fail: ATTRIBUTE((unused))
    goto fn_exit;
}


