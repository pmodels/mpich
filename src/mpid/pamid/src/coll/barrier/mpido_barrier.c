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
 * \file src/coll/barrier/mpido_barrier.c
 * \brief ???
 */

/* #define TRACE_ON */

#include <mpidimpl.h>

static void cb_barrier(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *) clientdata;
   TRACE_ERR("callback. enter: %d\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}

int MPIDO_Barrier(MPID_Comm *comm_ptr, int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Barrier\n");
   volatile unsigned active=1;
   MPIDI_Post_coll_t barrier_post;
   pami_xfer_t barrier;
   pami_algorithm_t my_barrier;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   int queryreq = 0;
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_BARRIER];
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (comm_ptr->rank == 0);
#endif

   if(unlikely(selected_type == MPID_COLL_USE_MPICH))
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using MPICH barrier\n");
      TRACE_ERR("Using MPICH Barrier\n");
      return MPIR_Barrier(comm_ptr, mpierrno);
   }

   barrier.cb_done = cb_barrier;
   barrier.cookie = (void *)&active;
   if(likely(selected_type == MPID_COLL_OPTIMIZED))
   {
      TRACE_ERR("Optimized barrier (%s) was pre-selected\n", mpid->opt_protocol_md[PAMI_XFER_BARRIER][0].name);
      my_barrier = mpid->opt_protocol[PAMI_XFER_BARRIER][0];
      my_md = &mpid->opt_protocol_md[PAMI_XFER_BARRIER][0];
      queryreq = mpid->must_query[PAMI_XFER_BARRIER][0];
   }
   else
   {
      TRACE_ERR("Barrier (%s) was specified by user\n", mpid->user_metadata[PAMI_XFER_BARRIER].name);
      my_barrier = mpid->user_selected[PAMI_XFER_BARRIER];
      my_md = &mpid->user_metadata[PAMI_XFER_BARRIER];
      queryreq = selected_type;
   }

   barrier.algorithm = my_barrier;
   /* There is no support for query-required barrier protocols here */
   MPID_assert(queryreq != MPID_COLL_ALWAYS_QUERY);
   MPID_assert(queryreq != MPID_COLL_CHECK_FN_REQUIRED);

   if(unlikely(verbose))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
     fprintf(stderr,"<%llx> Using protocol %s for barrier on %u\n", 
             threadID,
             my_md->name,
            (unsigned) comm_ptr->context_id);
   }
   MPIDI_Context_post(MPIDI_Context[0], &barrier_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&barrier);

   TRACE_ERR("advance spinning\n");
   MPIDI_Update_last_algorithm(comm_ptr, my_md->name);
   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("exiting mpido_barrier\n");
   return 0;
}


int MPIDO_Barrier_simple(MPID_Comm *comm_ptr, int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Barrier_optimized\n");
   volatile unsigned active=1;
   MPIDI_Post_coll_t barrier_post;
   pami_xfer_t barrier;
   pami_algorithm_t my_barrier;
   const pami_metadata_t *my_barrier_md;
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
 
   barrier.cb_done = cb_barrier;
   barrier.cookie = (void *)&active;
   my_barrier = mpid->coll_algorithm[PAMI_XFER_BARRIER][0][0];
   my_barrier_md = &mpid->coll_metadata[PAMI_XFER_BARRIER][0][0];
   barrier.algorithm = my_barrier;


   TRACE_ERR("%s barrier\n", MPIDI_Process.context_post.active>0?"posting":"invoking");
   MPIDI_Context_post(MPIDI_Context[0], &barrier_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&barrier);
   TRACE_ERR("barrier %s rc: %d\n", MPIDI_Process.context_post.active>0?"posted":"invoked", rc);

   TRACE_ERR("advance spinning\n");
   MPIDI_Update_last_algorithm(comm_ptr, my_barrier_md->name);
   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("Exiting MPIDO_Barrier_optimized\n");
   return 0;
}

int
MPIDO_CSWrapper_barrier(pami_xfer_t *barrier,
                        void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   int rc = MPIR_Barrier(comm_ptr, &mpierrno);
   if(barrier->cb_done && rc == 0)
     barrier->cb_done(NULL, barrier->cookie, PAMI_SUCCESS);
   return rc;

}

