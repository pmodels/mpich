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
 * \file src/coll/gather/mpido_gatherv.c
 * \brief ???
 */

/* #define TRACE_ON */
#include <mpidimpl.h>

static void cb_gatherv(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_gatherv enter, active: %u\n", (*active));
   (*active)--;
}

int MPIDO_Gatherv(const void *sendbuf, 
                  int sendcount, 
                  MPI_Datatype sendtype,
                  void *recvbuf, 
                  const int *recvcounts, 
                  const int *displs, 
                  MPI_Datatype recvtype,
                  int root, 
                  MPID_Comm * comm_ptr, 
                  int *mpierrno)

{
   TRACE_ERR("Entering MPIDO_Gatherv\n");
   int rc;
   int contig, rsize=0, ssize=0;
   int pamidt = 1;
   MPID_Datatype *dt_ptr = NULL;
   MPI_Aint send_true_lb, recv_true_lb;
   char *sbuf, *rbuf;
   pami_type_t stype, rtype;
   int tmp;
   volatile unsigned gatherv_active = 1;
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_GATHERV_INT];

   /* Check for native PAMI types and MPI_IN_PLACE on sendbuf */
   /* MPI_IN_PLACE is a nonlocal decision. We will need a preallreduce if we ever have
    * multiple "good" gathervs that work on different counts for example */
   if((sendbuf != MPI_IN_PLACE) && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   if(pamidt == 0 || selected_type == MPID_COLL_USE_MPICH)
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH gatherv algorithm\n");
      TRACE_ERR("GATHERV using MPICH\n");
      MPIDI_Update_last_algorithm(comm_ptr, "GATHERV_MPICH");
      return MPIR_Gatherv(sendbuf, sendcount, sendtype,
               recvbuf, recvcounts, displs, recvtype,
               root, comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(1, recvtype, contig, rsize, dt_ptr, recv_true_lb);
   rbuf = (char *)recvbuf + recv_true_lb;
   sbuf = (void *) sendbuf;

   pami_xfer_t gatherv;

   gatherv.cb_done = cb_gatherv;
   gatherv.cookie = (void *)&gatherv_active;
   gatherv.cmd.xfer_gatherv_int.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);
   gatherv.cmd.xfer_gatherv_int.rcvbuf = rbuf;
   gatherv.cmd.xfer_gatherv_int.rtype = rtype;
   gatherv.cmd.xfer_gatherv_int.rtypecounts = (int *) recvcounts;
   gatherv.cmd.xfer_gatherv_int.rdispls = (int *) displs;

   gatherv.cmd.xfer_gatherv_int.sndbuf = NULL;
   gatherv.cmd.xfer_gatherv_int.stype = stype;
   gatherv.cmd.xfer_gatherv_int.stypecount = sendcount;

   if(rank == root)
   {
      if(sendbuf == MPI_IN_PLACE) 
      {
         if(unlikely(verbose))
            fprintf(stderr,"gatherv MPI_IN_PLACE buffering\n");
         sbuf = PAMI_IN_PLACE;
         gatherv.cmd.xfer_gatherv_int.stype = rtype;
         gatherv.cmd.xfer_gatherv_int.stypecount = recvcounts[rank];
      }
      else
      {
         MPIDI_Datatype_get_info(1, sendtype, contig, ssize, dt_ptr, send_true_lb);
         sbuf = (char *)sbuf + send_true_lb;
      }
   }
   gatherv.cmd.xfer_gatherv_int.sndbuf = sbuf;

   pami_algorithm_t my_gatherv;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   int queryreq = 0;

   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized gatherv %s was selected\n",
         mpid->opt_protocol_md[PAMI_XFER_GATHERV_INT][0].name);
      my_gatherv = mpid->opt_protocol[PAMI_XFER_GATHERV_INT][0];
      my_md = &mpid->opt_protocol_md[PAMI_XFER_GATHERV_INT][0];
      queryreq = mpid->must_query[PAMI_XFER_GATHERV_INT][0];
   }
   else
   {
      TRACE_ERR("Optimized gatherv %s was set by user\n",
         mpid->user_metadata[PAMI_XFER_GATHERV_INT].name);
         my_gatherv = mpid->user_selected[PAMI_XFER_GATHERV_INT];
         my_md = &mpid->user_metadata[PAMI_XFER_GATHERV_INT];
         queryreq = selected_type;
   }

   gatherv.algorithm = my_gatherv;


   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || 
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying gatherv protocol %s, type was %d\n", 
         my_md->name, queryreq);
      if(my_md->check_fn == NULL)
      {
         /* process metadata bits */
         if((!my_md->check_correct.values.inplace) && (sendbuf == MPI_IN_PLACE))
            result.check.unspecified = 1;
/* Can't check ranges like this.  Non-local.  Comment out for now.
         if(my_md->check_correct.values.rangeminmax)
         {
            MPI_Aint data_true_lb;
            MPID_Datatype *data_ptr;
            int data_size, data_contig;
            MPIDI_Datatype_get_info(sendcount, sendtype, data_contig, data_size, data_ptr, data_true_lb); 
            if((my_md->range_lo <= data_size) &&
               (my_md->range_hi >= data_size))
               ; 
            else
            {
               result.check.range = 1;
               if(unlikely(verbose))
               {   
                  fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                          data_size,
                          my_md->range_lo,
                          my_md->range_hi,
                          my_md->name);
               }
            }
         }
 */
      }
      else /* calling the check fn is sufficient */
         result = my_md->check_fn(&gatherv);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
      if(result.bitmask)
      {
         if(unlikely(verbose))
            fprintf(stderr,"Query failed for %s. Using MPICH gatherv.\n", my_md->name);
         MPIDI_Update_last_algorithm(comm_ptr, "GATHERV_MPICH");
         return MPIR_Gatherv(sendbuf, sendcount, sendtype,
                             recvbuf, recvcounts, displs, recvtype,
                             root, comm_ptr, mpierrno);
      }
      if(my_md->check_correct.values.asyncflowctl && !(--(comm_ptr->mpid.num_requests))) 
      { 
         comm_ptr->mpid.num_requests = MPIDI_Process.optimized.num_requests;
         int tmpmpierrno;   
         if(unlikely(verbose))
            fprintf(stderr,"Query barrier required for %s\n", my_md->name);
         MPIDO_Barrier(comm_ptr, &tmpmpierrno);
      }
   }
   
   MPIDI_Update_last_algorithm(comm_ptr, my_md->name);

   if(unlikely(verbose))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for gatherv on %u\n", 
              threadID,
              my_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Post_coll_t gatherv_post;
   MPIDI_Context_post(MPIDI_Context[0], &gatherv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&gatherv);
   
   TRACE_ERR("Waiting on active %d\n", gatherv_active);
   MPID_PROGRESS_WAIT_WHILE(gatherv_active);

   TRACE_ERR("Leaving MPIDO_Gatherv\n");
   return 0;
}


int MPIDO_Gatherv_simple(const void *sendbuf, 
                  int sendcount, 
                  MPI_Datatype sendtype,
                  void *recvbuf, 
                  const int *recvcounts, 
                  const int *displs, 
                  MPI_Datatype recvtype,
                  int root, 
                  MPID_Comm * comm_ptr, 
                  int *mpierrno)

{
   TRACE_ERR("Entering MPIDO_Gatherv_optimized\n");
   int rc;
   int contig, rsize=0, ssize=0;
   int pamidt = 1;
   MPID_Datatype *dt_ptr = NULL;
   MPI_Aint send_true_lb, recv_true_lb;
   char *sbuf, *rbuf;
   pami_type_t stype = NULL, rtype;
   int tmp;
   volatile unsigned gatherv_active = 1;
   const int rank = comm_ptr->rank;

   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);


   /* Check for native PAMI types and MPI_IN_PLACE on sendbuf */
   /* MPI_IN_PLACE is a nonlocal decision. We will need a preallreduce if we ever have
    * multiple "good" gathervs that work on different counts for example */
   if(sendbuf != MPI_IN_PLACE && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   MPIDI_Datatype_get_info(1, recvtype, contig, rsize, dt_ptr, recv_true_lb);
   if(!contig) pamidt = 0;

   rbuf = (char *)recvbuf + recv_true_lb;
   sbuf = (void *) sendbuf;
   pami_xfer_t gatherv;
   if(rank == root)
   {
      if(sendbuf == MPI_IN_PLACE) 
      {
         sbuf = PAMI_IN_PLACE;
      }
      else
      {
         MPIDI_Datatype_get_info(1, sendtype, contig, ssize, dt_ptr, send_true_lb);
		 if(!contig) pamidt = 0;
         sbuf = (char *)sbuf + send_true_lb;
      }
      gatherv.cmd.xfer_gatherv_int.stype = stype;/* stype is ignored when sndbuf == PAMI_IN_PLACE */
      gatherv.cmd.xfer_gatherv_int.stypecount = sendcount;

   }
   else
   {
      gatherv.cmd.xfer_gatherv_int.stype = stype;
      gatherv.cmd.xfer_gatherv_int.stypecount = sendcount;     
   }

   if(pamidt == 0)
   {
      TRACE_ERR("GATHERV using MPICH\n");
      MPIDI_Update_last_algorithm(comm_ptr, "GATHERV_MPICH");
      return MPIR_Gatherv(sendbuf, sendcount, sendtype,
               recvbuf, recvcounts, displs, recvtype,
               root, comm_ptr, mpierrno);
   }





   gatherv.cb_done = cb_gatherv;
   gatherv.cookie = (void *)&gatherv_active;
   gatherv.cmd.xfer_gatherv_int.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);
   gatherv.cmd.xfer_gatherv_int.rcvbuf = rbuf;
   gatherv.cmd.xfer_gatherv_int.rtype = rtype;
   gatherv.cmd.xfer_gatherv_int.rtypecounts = (int *) recvcounts;
   gatherv.cmd.xfer_gatherv_int.rdispls = (int *) displs;

   gatherv.cmd.xfer_gatherv_int.sndbuf = sbuf;

   const pami_metadata_t *my_gatherv_md;

   gatherv.algorithm = mpid->coll_algorithm[PAMI_XFER_GATHERV_INT][0][0];
   my_gatherv_md = &mpid->coll_metadata[PAMI_XFER_GATHERV_INT][0][0];

   MPIDI_Update_last_algorithm(comm_ptr, my_gatherv_md->name);

   MPIDI_Post_coll_t gatherv_post;
   TRACE_ERR("%s gatherv\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
   MPIDI_Context_post(MPIDI_Context[0], &gatherv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&gatherv);
   TRACE_ERR("Gatherv %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");
   
   TRACE_ERR("Waiting on active %d\n", gatherv_active);
   MPID_PROGRESS_WAIT_WHILE(gatherv_active);

   TRACE_ERR("Leaving MPIDO_Gatherv_optimized\n");
   return MPI_SUCCESS;
}
