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
 * \file src/coll/alltoall/mpido_ialltoall.c
 * \brief ???
 */

/*#define TRACE_ON*/

#include <mpidimpl.h>

int MPIDO_Ialltoall(const void *sendbuf,
                    int sendcount,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int recvcount,
                    MPI_Datatype recvtype,
                    MPID_Comm *comm_ptr,
                    MPID_Request **request)
{
   TRACE_ERR("Entering MPIDO_Ialltoall\n");

   /*if (unlikely((data_size == 0) || (user_selected_type == MPID_COLL_USE_MPICH)))*/
   {
      /*
       * If the mpich mpir non-blocking collectives are enabled, return without
       * first constructing the MPID_Request. This signals to the
       * MPIR_Ialltoall_impl() function to invoke the mpich nbc implementation
       * of MPI_Ialltoall().
       */
      if (MPIDI_Process.mpir_nbc != 0)
       return 0;

      /*
       * MPIR_* nbc implementation is not enabled. Fake a non-blocking
       * MPIR_Ialltoall() with a blocking MPIR_Alltoall().
       */
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH blocking alltoall algorithm\n");

      int mpierrno = 0;
      int rc = MPIR_Alltoall_impl(sendbuf, sendcount, sendtype,
                                  recvbuf, recvcount, recvtype,
                                  comm_ptr, &mpierrno);

      /*
       * The blocking allitoall has completed - create and complete a
       * MPID_Request object so the MPIR_Ialltoall_impl() function does not
       * perform an additional ialltoall.
       */
      MPID_Request * mpid_request = MPID_Request_create_inline();
      mpid_request->kind = MPID_COLL_REQUEST;
      *request = mpid_request;
      MPIDI_Request_complete_norelease_inline(mpid_request);

      return rc;
   }

   TRACE_ERR("Leaving MPIDO_Ialltoall\n");
   return 0;
}
