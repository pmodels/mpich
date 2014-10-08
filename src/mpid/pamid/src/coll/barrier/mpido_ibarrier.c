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
 * \file src/coll/barrier/mpido_ibarrier.c
 * \brief ???
 */

/*#define TRACE_ON*/

#include <mpidimpl.h>

static void cb_ibarrier(void *ctxt, void *clientdata, pami_result_t err)
{
   MPID_Request *mpid_request = (MPID_Request *) clientdata;
   MPIDI_Request_complete_norelease_inline(mpid_request);
}

int MPIDO_Ibarrier(MPID_Comm *comm_ptr, MPID_Request **request)
{
   TRACE_ERR("Entering MPIDO_Ibarrier\n");

     /*
      * There is actually no current pami optimization for this
      * so just kick it back to MPICH if mpir_nbc is set, otherwise
      * call the blocking MPIR_Barrier().
      */
     if (MPIDI_Process.mpir_nbc != 0)
       return 0;

     /*
      * MPIR_* nbc implementation is not enabled. Fake a non-blocking
      * MPIR_Ibarrier() with a blocking MPIR_Barrier().
      */
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using MPICH barrier\n");
      TRACE_ERR("Using MPICH Barrier\n");

      int mpierrno = 0;
      int rc = MPIR_Barrier(comm_ptr, &mpierrno);

      MPID_Request * mpid_request = MPID_Request_create_inline();
      mpid_request->kind = MPID_COLL_REQUEST;
      *request = mpid_request;
      MPIDI_Request_complete_norelease_inline(mpid_request);

      return rc;
}
