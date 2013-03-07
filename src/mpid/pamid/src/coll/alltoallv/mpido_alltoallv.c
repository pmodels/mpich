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
/* #define TRACE_ON */

#include <mpidimpl.h>

static void cb_alltoallv(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *)clientdata;
   TRACE_ERR("alltoallv callback enter, active: %d\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}


int MPIDO_Alltoallv(const void *sendbuf,
                   const int *sendcounts,
                   const int *senddispls,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcounts,
                   const int *recvdispls,
                   MPI_Datatype recvtype,
                   MPID_Comm *comm_ptr,
                   int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Alltoallv\n");
   volatile unsigned active = 1;
   int sndtypelen, rcvtypelen, snd_contig, rcv_contig;
   MPID_Datatype *sdt, *rdt;
   pami_type_t stype, rtype;
   MPI_Aint sdt_true_lb, rdt_true_lb;
   MPIDI_Post_coll_t alltoallv_post;
   int pamidt = 1;
   int tmp;
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_ALLTOALLV_INT];

   if((sendbuf != MPI_IN_PLACE) && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   MPIDI_Datatype_get_info(1, recvtype, rcv_contig, rcvtypelen, rdt, rdt_true_lb);
   if(!rcv_contig) pamidt = 0;

   if((selected_type == MPID_COLL_USE_MPICH) ||
       pamidt == 0)
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH alltoallv algorithm\n");
      return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                            recvbuf, recvcounts, recvdispls, recvtype,
                            comm_ptr, mpierrno);
   }

   pami_xfer_t alltoallv;
   pami_algorithm_t my_alltoallv;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   int queryreq = 0;

   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized alltoallv was selected\n");
      my_alltoallv = mpid->opt_protocol[PAMI_XFER_ALLTOALLV_INT][0];
      my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLTOALLV_INT][0];
      queryreq = mpid->must_query[PAMI_XFER_ALLTOALLV_INT][0];
   }
   else
   { /* is this purely an else? or do i need to check for some other selectedvar... */
      TRACE_ERR("Alltoallv specified by user\n");
      my_alltoallv = mpid->user_selected[PAMI_XFER_ALLTOALLV_INT];
      my_md = &mpid->user_metadata[PAMI_XFER_ALLTOALLV_INT];
      queryreq = selected_type;
   }
   alltoallv.algorithm = my_alltoallv;
   char *pname = my_md->name;


   alltoallv.cb_done = cb_alltoallv;
   alltoallv.cookie = (void *)&active;
   /* We won't bother with alltoallv since MPI is always going to be ints. */
   if(sendbuf == MPI_IN_PLACE)
   {
     if(unlikely(verbose))
         fprintf(stderr,"alltoallv MPI_IN_PLACE buffering\n");
      alltoallv.cmd.xfer_alltoallv_int.stype = rtype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) recvdispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) recvcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = PAMI_IN_PLACE;
   }
   else
   {
      MPIDI_Datatype_get_info(1, sendtype, snd_contig, sndtypelen, sdt, sdt_true_lb);
      alltoallv.cmd.xfer_alltoallv_int.stype = stype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) senddispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) sendcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = (char *)sendbuf+sdt_true_lb;
   }
   alltoallv.cmd.xfer_alltoallv_int.rcvbuf = (char *)recvbuf+rdt_true_lb;
      
   alltoallv.cmd.xfer_alltoallv_int.rdispls = (int *) recvdispls;
   alltoallv.cmd.xfer_alltoallv_int.rtypecounts = (int *) recvcounts;
   alltoallv.cmd.xfer_alltoallv_int.rtype = rtype;

   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || 
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying alltoallv protocol %s, type was %d\n", pname, queryreq);
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
            MPIDI_Datatype_get_info(??, sendtype, data_contig, data_size, data_ptr, data_true_lb); 
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
         result = my_md->check_fn(&alltoallv);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
      if(result.bitmask)
      {
        if(unlikely(verbose))
          fprintf(stderr,"Query failed for %s. Using MPICH alltoallv\n", pname);
        MPIDI_Update_last_algorithm(comm_ptr, "ALLTOALLV_MPICH");
        return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                              recvbuf, recvcounts, recvdispls, recvtype,
                              comm_ptr, mpierrno);
      }
      if(my_md->check_correct.values.asyncflowctl && !(--(comm_ptr->mpid.num_requests))) 
      { 
         comm_ptr->mpid.num_requests = MPIDI_Process.optimized.num_requests;
         int tmpmpierrno;   
         if(unlikely(verbose))
            fprintf(stderr,"Query barrier required for %s\n", pname);
         MPIDO_Barrier(comm_ptr, &tmpmpierrno);
      }
   }

   if(unlikely(verbose))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for alltoallv on %u\n", 
              threadID,
              my_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Context_post(MPIDI_Context[0], &alltoallv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&alltoallv);

   TRACE_ERR("%d waiting on active %d\n", rank, active);
   MPID_PROGRESS_WAIT_WHILE(active);


   TRACE_ERR("Leaving alltoallv\n");


   return 0;
}


int MPIDO_Alltoallv_simple(const void *sendbuf,
                   const int *sendcounts,
                   const int *senddispls,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcounts,
                   const int *recvdispls,
                   MPI_Datatype recvtype,
                   MPID_Comm *comm_ptr,
                   int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Alltoallv_optimized\n");
   volatile unsigned active = 1;
   int sndtypelen, rcvtypelen, snd_contig, rcv_contig;
   MPID_Datatype *sdt, *rdt;
   pami_type_t stype = NULL, rtype;
   MPI_Aint sdt_true_lb = 0, rdt_true_lb;
   MPIDI_Post_coll_t alltoallv_post;
   int pamidt = 1;
   int tmp;
   const int rank = comm_ptr->rank;

   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);

   if(sendbuf != MPI_IN_PLACE && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;


   if(sendbuf != MPI_IN_PLACE)
   {
     MPIDI_Datatype_get_info(1, sendtype, snd_contig, sndtypelen, sdt, sdt_true_lb);
     if(!snd_contig) pamidt = 0;
   }
   MPIDI_Datatype_get_info(1, recvtype, rcv_contig, rcvtypelen, rdt, rdt_true_lb);
   if(!rcv_contig) pamidt = 0;


   if(pamidt == 0)
   {
      return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                              recvbuf, recvcounts, recvdispls, recvtype,
                              comm_ptr, mpierrno);

   }   

   pami_xfer_t alltoallv;
   const pami_metadata_t *my_alltoallv_md;
   my_alltoallv_md = &mpid->coll_metadata[PAMI_XFER_ALLTOALLV_INT][0][0];

   alltoallv.algorithm = mpid->coll_algorithm[PAMI_XFER_ALLTOALLV_INT][0][0];
   char *pname = my_alltoallv_md->name;

   alltoallv.cb_done = cb_alltoallv;
   alltoallv.cookie = (void *)&active;
   alltoallv.cmd.xfer_alltoallv_int.stype = stype;/* stype is ignored when sndbuf == PAMI_IN_PLACE */
   alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) senddispls;
   alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) sendcounts;
   alltoallv.cmd.xfer_alltoallv_int.sndbuf = (char *)sendbuf+sdt_true_lb;

   /* We won't bother with alltoallv since MPI is always going to be ints. */
   if(sendbuf == MPI_IN_PLACE)
   {
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = PAMI_IN_PLACE;
   }
   alltoallv.cmd.xfer_alltoallv_int.rcvbuf = (char *)recvbuf+rdt_true_lb;
   alltoallv.cmd.xfer_alltoallv_int.rdispls = (int *) recvdispls;
   alltoallv.cmd.xfer_alltoallv_int.rtypecounts = (int *) recvcounts;
   alltoallv.cmd.xfer_alltoallv_int.rtype = rtype;


   MPIDI_Context_post(MPIDI_Context[0], &alltoallv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&alltoallv);

   TRACE_ERR("%d waiting on active %d\n", rank, active);
   MPID_PROGRESS_WAIT_WHILE(active);


   TRACE_ERR("Leaving alltoallv\n");


   return MPI_SUCCESS;
}
