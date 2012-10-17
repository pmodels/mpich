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
 * \file src/coll/gather/mpido_reduce.c
 * \brief ???
 */

/* #define TRACE_ON */
#include <mpidimpl.h>

static void reduce_cb_done(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_reduce enter, active: %u\n", (*active));
   (*active)--;
}

int MPIDO_Reduce(const void *sendbuf, 
                 void *recvbuf, 
                 int count, 
                 MPI_Datatype datatype,
                 MPI_Op op, 
                 int root, 
                 MPID_Comm *comm_ptr, 
                 int *mpierrno)

{
   MPID_Datatype *dt_null = NULL;
   MPI_Aint true_lb = 0;
   int dt_contig, tsize;
   int mu;
   char *sbuf, *rbuf;
   pami_data_function pop;
   pami_type_t pdt;
   int rc;
   int alg_selected = 0;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (comm_ptr->rank == 0);
#endif
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_REDUCE];

   rc = MPIDI_Datatype_to_pami(datatype, &pdt, op, &pop, &mu);
   if(unlikely(verbose))
      fprintf(stderr,"reduce - rc %u, dt: %p, op: %p, mu: %u, selectedvar %u != %u (MPICH)\n",
         rc, pdt, pop, mu, 
         (unsigned)selected_type, MPID_COLL_USE_MPICH);

   pami_xfer_t reduce;
   pami_algorithm_t my_reduce=0;
   const pami_metadata_t *my_reduce_md=NULL;
   int queryreq = 0;
   volatile unsigned reduce_active = 1;

   if(selected_type == MPID_COLL_USE_MPICH || rc != MPI_SUCCESS)
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH reduce algorithm\n");
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(count, datatype, dt_contig, tsize, dt_null, true_lb);
   rbuf = (char *)recvbuf + true_lb;
   sbuf = (char *)sendbuf + true_lb;
   if(sendbuf == MPI_IN_PLACE) 
   {
      if(unlikely(verbose))
         fprintf(stderr,"reduce MPI_IN_PLACE buffering\n");
      sbuf = rbuf;
   }

   reduce.cb_done = reduce_cb_done;
   reduce.cookie = (void *)&reduce_active;
   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      if((mpid->cutoff_size[PAMI_XFER_REDUCE][0] == 0) || 
          (mpid->cutoff_size[PAMI_XFER_REDUCE][0] >= tsize && mpid->cutoff_size[PAMI_XFER_REDUCE][0] > 0))
      {
        TRACE_ERR("Optimized Reduce (%s) was pre-selected\n",
         mpid->opt_protocol_md[PAMI_XFER_REDUCE][0].name);
        my_reduce    = mpid->opt_protocol[PAMI_XFER_REDUCE][0];
        my_reduce_md = &mpid->opt_protocol_md[PAMI_XFER_REDUCE][0];
        queryreq     = mpid->must_query[PAMI_XFER_REDUCE][0];
      }

   }
   else
   {
      TRACE_ERR("Optimized reduce (%s) was specified by user\n",
      mpid->user_metadata[PAMI_XFER_REDUCE].name);
      my_reduce    =  mpid->user_selected[PAMI_XFER_REDUCE];
      my_reduce_md = &mpid->user_metadata[PAMI_XFER_REDUCE];
      queryreq     = selected_type;
   }
   reduce.algorithm = my_reduce;
   reduce.cmd.xfer_reduce.sndbuf = sbuf;
   reduce.cmd.xfer_reduce.rcvbuf = rbuf;
   reduce.cmd.xfer_reduce.stype = pdt;
   reduce.cmd.xfer_reduce.rtype = pdt;
   reduce.cmd.xfer_reduce.stypecount = count;
   reduce.cmd.xfer_reduce.rtypecount = count;
   reduce.cmd.xfer_reduce.op = pop;
   reduce.cmd.xfer_reduce.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);


   if(queryreq == MPID_COLL_ALWAYS_QUERY || queryreq == MPID_COLL_CHECK_FN_REQUIRED)
   {
      if(my_reduce_md->check_fn != NULL)
      {
         metadata_result_t result = {0};
         TRACE_ERR("Querying reduce protocol %s, type was %d\n",
            my_reduce_md->name,
            queryreq);
         result = my_reduce_md->check_fn(&reduce);
         TRACE_ERR("Bitmask: %#X\n", result.bitmask);
         if(result.bitmask)
         {
            if(verbose)
              fprintf(stderr,"Query failed for %s.\n",
                 my_reduce_md->name);
         }
         else alg_selected = 1;
      }
      else
      {
         /* No check function, but check required */
         /* look at meta data */
         /* assert(0);*/
      }
   }

   if(alg_selected)
   {
      if(unlikely(verbose))
      {
         unsigned long long int threadID;
         MPIU_Thread_id_t tid;
         MPIU_Thread_self(&tid);
         threadID = (unsigned long long int)tid;
         fprintf(stderr,"<%llx> Using protocol %s for reduce on %u\n", 
                 threadID,
                 my_reduce_md->name,
              (unsigned) comm_ptr->context_id);
      }
      TRACE_ERR("%s reduce, context %d, algoname: %s, exflag: %d\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking", 0,
                my_reduce_md->name, exflag);
      MPIDI_Post_coll_t reduce_post;
      MPIDI_Context_post(MPIDI_Context[0], &reduce_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&reduce);
      TRACE_ERR("Reduce %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");
   }
   else
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH reduce algorithm\n");
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
   }

   MPIDI_Update_last_algorithm(comm_ptr,
                               my_reduce_md->name);
   MPID_PROGRESS_WAIT_WHILE(reduce_active);
   TRACE_ERR("Reduce done\n");
   return 0;
}
