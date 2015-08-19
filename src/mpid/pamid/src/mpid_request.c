/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_request.c
 * \brief Accessors and actors for MPID Requests
 */
#include <mpidimpl.h>

#ifndef MPID_REQUEST_PREALLOC
#if (MPICH_THREAD_GRANULARITY == MPICH_THREAD_GRANULARITY_GLOBAL)
#define  MPID_REQUEST_PREALLOC 16
#elif (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL)
#define  MPID_REQUEST_PREALLOC 512  //Have direct more reqyests for all threads
#else
#define MPID_REQUEST_PREALLOC 8
#endif
#endif

/**
 * \defgroup MPID_REQUEST MPID Request object management
 *
 * Accessors and actors for MPID Requests
 */


/* these are referenced by src/mpi/pt2pt/wait.c in PMPI_Wait! */
MPID_Request MPID_Request_direct[MPID_REQUEST_PREALLOC] __attribute__((__aligned__(64)));
MPIU_Object_alloc_t MPID_Request_mem =
  {
    0, 0, 0, 0, MPID_REQUEST, sizeof(MPID_Request),
    MPID_Request_direct,
    MPID_REQUEST_PREALLOC
  };


#if (MPIU_HANDLE_ALLOCATION_METHOD == MPIU_HANDLE_ALLOCATION_THREAD_LOCAL) && defined(__BGQ__)
void MPIDI_Request_allocate_pool()
{
  int i;
  MPID_Request *prev, *cur;
  /* batch allocate a linked list of requests */
  MPIU_THREAD_CS_ENTER(HANDLEALLOC,);
  prev = MPIU_Handle_obj_alloc_unsafe(&MPID_Request_mem);
  MPID_assert(prev != NULL);
  prev->mpid.next = NULL;
  for (i = 1; i < MPID_REQUEST_TLS_MAX; ++i) {
    cur = MPIU_Handle_obj_alloc_unsafe(&MPID_Request_mem);
    MPID_assert(cur != NULL);
    cur->mpid.next = prev;
    prev = cur;
  }
  MPIU_THREAD_CS_EXIT(HANDLEALLOC,);
  MPIDI_Process.request_handles[MPIDI_THREAD_ID()].head = cur;
  MPIDI_Process.request_handles[MPIDI_THREAD_ID()].count += MPID_REQUEST_TLS_MAX;
}
#endif


void
MPIDI_Request_uncomplete(MPID_Request *req)
{
  int count;
  MPIU_Object_add_ref(req);
  MPID_cc_incr(req->cc_ptr, &count);
}


void
MPID_Request_set_completed(MPID_Request *req)
{
  MPID_cc_set(&req->cc, 0);
  MPIDI_Progress_signal();
}
