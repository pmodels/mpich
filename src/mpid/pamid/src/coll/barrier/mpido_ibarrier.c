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

   if(unlikely(comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] == MPID_COLL_USE_MPICH))
   {
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

   MPIDI_Post_coll_t barrier_post;
   pami_xfer_t barrier;
   pami_algorithm_t my_barrier;
   pami_metadata_t *my_barrier_md;
   int queryreq = 0;

   MPID_Request * mpid_request = MPID_Request_create_inline();
   mpid_request->kind = MPID_COLL_REQUEST;
   *request = mpid_request;

   barrier.cb_done = cb_ibarrier;
   barrier.cookie = (void *)mpid_request;

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized barrier (%s) was pre-selected\n", comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BARRIER][0].name);
      my_barrier = comm_ptr->mpid.opt_protocol[PAMI_XFER_BARRIER][0];
      my_barrier_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BARRIER][0];
      queryreq = comm_ptr->mpid.must_query[PAMI_XFER_BARRIER][0];
   }
   else
   {
      TRACE_ERR("Barrier (%s) was specified by user\n", comm_ptr->mpid.user_metadata[PAMI_XFER_BARRIER].name);
      my_barrier = comm_ptr->mpid.user_selected[PAMI_XFER_BARRIER];
      my_barrier_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_BARRIER];
      queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_BARRIER];
   }

   barrier.algorithm = my_barrier;
   /* There is no support for query-required barrier protocols here */
   MPID_assert_always(queryreq != MPID_COLL_ALWAYS_QUERY);
   MPID_assert_always(queryreq != MPID_COLL_CHECK_FN_REQUIRED);

   /* TODO Name needs fixed somehow */
   MPIDI_Update_last_algorithm(comm_ptr, my_barrier_md->name);
   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for barrier on %u\n",
              threadID,
              my_barrier_md->name,
              (unsigned) comm_ptr->context_id);
   }
   TRACE_ERR("%s barrier\n", MPIDI_Process.context_post.active>0?"posting":"invoking");
   MPIDI_Context_post(MPIDI_Context[0], &barrier_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&barrier);
   TRACE_ERR("barrier %s rc: %d\n", MPIDI_Process.context_post.active>0?"posted":"invoked", rc);

   MPID_Progress_wait_inline(1);

   TRACE_ERR("exiting mpido_ibarrier\n");
   return 0;
}
