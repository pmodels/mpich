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
 * \file src/coll/alltoallv/mpido_alltoallv.c
 * \brief ???
 */
/*#define TRACE_ON*/

#include <mpidimpl.h>

int MPIDO_Ialltoallv(const void *sendbuf,
                    const int *sendcounts,
                    const int *senddispls,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    const int *recvcounts,
                    const int *recvdispls,
                    MPI_Datatype recvtype,
                    MPID_Comm *comm_ptr,
                    MPID_Request **request)
{
   if(comm_ptr->rank == 0)
      TRACE_ERR("Entering MPIDO_Ialltoallv\n");

   /*if (unlikely((data_size == 0) || (user_selected_type == MPID_COLL_USE_MPICH)))*/
   {
      /*
       * If the mpich mpir non-blocking collectives are enabled, return without
       * first constructing the MPID_Request. This signals to the
       * MPIR_Ialltoalliv_impl() function to invoke the mpich nbc implementation
       * of MPI_Ialltoallv().
       */
      if (MPIDI_Process.mpir_nbc != 0)
       return 0;

      /*
       * MPIR_* nbc implementation is not enabled. Fake a non-blocking
       * MPIR_Ialltoallv() with a blocking MPIR_Alltoallv().
       */
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH blocking alltoallv algorithm\n");

      int mpierrno = 0;
      int rc = MPIR_Alltoallv_impl(sendbuf, sendcounts, senddispls, sendtype,
                                  recvbuf, recvcounts, recvdispls, recvtype,
                                  comm_ptr, &mpierrno);

      /*
       * The blocking alltoallv has completed - create and complete a
       * MPID_Request object so the MPIR_Ialltoallv_impl() function does not
       * perform an additional ialltoallv.
       */
      MPID_Request * mpid_request = MPID_Request_create_inline();
      mpid_request->kind = MPID_COLL_REQUEST;
      *request = mpid_request;
      MPIDI_Request_complete_norelease_inline(mpid_request);

      return rc;
   }


   TRACE_ERR("Leaving ialltoallv\n");
   return 0;
}
