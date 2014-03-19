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
#ifndef HAVE_PAMI_IN_PLACE
  if (sendbuf == MPI_IN_PLACE)
  {
    MPID_Abort (NULL, 0, 1, "'MPI_IN_PLACE' requries support for `PAMI_IN_PLACE`");
    return -1;
  }
#endif
   TRACE_ERR("Entering MPIDO_Alltoallv\n");
   volatile unsigned active = 1;
   void *snd_noncontig_buff = NULL, *rcv_noncontig_buff = NULL;
   void *sbuf = NULL, *rbuf = NULL;
   int recvok=PAMI_SUCCESS, sendok=PAMI_SUCCESS;
   int sndtypelen, rcvtypelen, snd_contig=0, rcv_contig=0;
   MPID_Datatype *sdt, *rdt;
   pami_type_t stype, rtype;
   MPI_Aint sdt_true_lb, rdt_true_lb;
   MPIDI_Post_coll_t alltoallv_post;
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
   const int size = comm_ptr->local_size;
   int sendcontinuous , recvcontinuous=0;
   size_t recv_size=0, send_size=0;
   size_t totalrecvcount=0;
   int *lrecvdispls = NULL; /* possible local displs calculated for noncontinous */
   int *lsenddispls = NULL;/* possible local displs calculated for noncontinous */
   int *lrecvcounts  = NULL;/* possible local counts calculated for noncontinous */
   int *lsendcounts  = NULL;/* possible local counts calculated for noncontinous */
   const int *precvdispls = recvdispls; /* pointer to displs to use as pami parmi */
   const int *psenddispls = senddispls; /* pointer to displs to use as pami parmi */
   const int *precvcounts = recvcounts; /* pointer to counts to use as pami parmi */
   const int *psendcounts = sendcounts; /* pointer to counts to use as pami parmi */
   int inplace = sendbuf == MPI_IN_PLACE? 1 : 0;
   if(selected_type == MPID_COLL_USE_MPICH)
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH alltoallv algorithm\n");
      return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                            recvbuf, recvcounts, recvdispls, recvtype,
                            comm_ptr, mpierrno);
   }

   if(!inplace)
   {
     sendok = MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp);
     MPIDI_Datatype_get_info(1, sendtype, snd_contig, sndtypelen, sdt, sdt_true_lb);
     sbuf = (char *)sendbuf + sdt_true_lb;
     if(!snd_contig || (sendok != PAMI_SUCCESS))
     {
        stype = PAMI_TYPE_UNSIGNED_CHAR;
        size_t totalsendcount = sendcounts[0];
        sendcontinuous = senddispls[0] == 0? 1 : 0 ;
        int i;
        psenddispls = lsenddispls = MPIU_Malloc(size*sizeof(int));
        psendcounts = lsendcounts = MPIU_Malloc(size*sizeof(int));
        lsenddispls[0]= 0;
        lsendcounts[0]= sndtypelen * sendcounts[0];
        for(i=1; i<size; ++i)
        {
           lsenddispls[i]= sndtypelen * totalsendcount;
           totalsendcount += sendcounts[i];
           if(senddispls[i] != (senddispls[i-1] + (sendcounts[i-1]*sndtypelen)))
              sendcontinuous = 0;
           lsendcounts[i]= sndtypelen * sendcounts[i];
        }
        send_size = sndtypelen * totalsendcount;
        TRACE_ERR("Pack receive sndv_contig %zu, sendok %zd, totalsendcount %zu, sendcontinuous %zu, sndtypelen %zu,  send_size %zu\n",
                (size_t)snd_contig, (size_t)sendok, (size_t)totalsendcount, (size_t)sendcontinuous, (size_t) sndtypelen, (size_t)send_size);
        snd_noncontig_buff = MPIU_Malloc(send_size);
        sbuf = snd_noncontig_buff;
        if(snd_noncontig_buff == NULL)
        {
           MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
              "Fatal:  Cannot allocate pack buffer");
        }
        if(sendcontinuous)
        {
           MPIR_Localcopy(sendbuf, totalsendcount, sendtype, 
                          snd_noncontig_buff, send_size,MPI_CHAR);
        }
        else
        {
           size_t extent; 
           MPID_Datatype_get_extent_macro(sendtype,extent);
           for(i=0; i<size; ++i)
           {
              char* scbuf = (char*)sendbuf + senddispls[i]*extent;
              char* rcbuf = (char*)snd_noncontig_buff + psenddispls[i];
              MPIR_Localcopy(scbuf, sendcounts[i], sendtype, 
                             rcbuf, psendcounts[i], MPI_CHAR);
              TRACE_ERR("Pack send src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                        (size_t)extent, (size_t)i,(size_t)senddispls[i],(size_t)i,(size_t)sendcounts[i],(size_t)senddispls[i], *(int*)scbuf);
              TRACE_ERR("Pack send dest             displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                        (size_t)i,(size_t)psenddispls[i],(size_t)i,(size_t)psendcounts[i],(size_t)psenddispls[i], *(int*)rcbuf);
           }
        }
     } 
   }
   MPIDI_Datatype_get_info(1, recvtype, rcv_contig, rcvtypelen, rdt, rdt_true_lb);
   recvok = MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp);
   rbuf = (char *)recvbuf + rdt_true_lb;
   if(!rcv_contig || (recvok != PAMI_SUCCESS))
   {
      rtype = PAMI_TYPE_UNSIGNED_CHAR;
      totalrecvcount = recvcounts[0];
      recvcontinuous = recvdispls[0] == 0? 1 : 0 ;
      int i;
      precvdispls = lrecvdispls = MPIU_Malloc(size*sizeof(int));
      precvcounts = lrecvcounts = MPIU_Malloc(size*sizeof(int));
      lrecvdispls[0]= 0;
      lrecvcounts[0]= rcvtypelen * recvcounts[0];
      for(i=1; i<size; ++i)
      {
         lrecvdispls[i]= rcvtypelen * totalrecvcount;
         totalrecvcount += recvcounts[i];
         if(recvdispls[i] != (recvdispls[i-1] + (recvcounts[i-1]*rcvtypelen)))
            recvcontinuous = 0;
         lrecvcounts[i]= rcvtypelen * recvcounts[i];
      }
      recv_size = rcvtypelen * totalrecvcount;
      TRACE_ERR("Pack receive rcv_contig %zu, recvok %zd, totalrecvcount %zu, recvcontinuous %zu, rcvtypelen %zu, recv_size %zu\n",
                (size_t)rcv_contig, (size_t)recvok, (size_t)totalrecvcount, (size_t)recvcontinuous,(size_t)rcvtypelen, (size_t)recv_size);
      rcv_noncontig_buff = MPIU_Malloc(recv_size);
      rbuf = rcv_noncontig_buff;
      if(rcv_noncontig_buff == NULL)
      {
         MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
            "Fatal:  Cannot allocate pack buffer");
      }
      /* need to copy it now if it's used for the send buffer and then do not do in place */
      if(inplace)
      {
        inplace = 0;
        stype = PAMI_TYPE_UNSIGNED_CHAR;
        size_t totalsendcount = recvcounts[0];
        sendcontinuous = recvdispls[0] == 0? 1 : 0 ;
        int i;
        psenddispls = lsenddispls = MPIU_Malloc(size*sizeof(int));
        psendcounts = lsendcounts = MPIU_Malloc(size*sizeof(int));
        lsenddispls[0]= 0;
        lsendcounts[0]= rcvtypelen * recvcounts[0];
        for(i=1; i<size; ++i)
        {
           lsenddispls[i]= rcvtypelen * totalsendcount;
           totalsendcount += recvcounts[i];
           if(recvdispls[i] != (recvdispls[i-1] + (recvcounts[i-1]*rcvtypelen)))
              sendcontinuous = 0;
           lsendcounts[i]= rcvtypelen * recvcounts[i];
        }
        send_size = rcvtypelen * totalsendcount;
        TRACE_ERR("Pack MPI_IN_PLACE receive sndv_contig %zu, sendok %zd, totalsendcount %zu, sendcontinuous %zu, rcvtypelen %zu,  send_size %zu\n",
                (size_t)snd_contig, (size_t)sendok, (size_t)totalsendcount, (size_t)sendcontinuous, (size_t) rcvtypelen, (size_t)send_size);
        snd_noncontig_buff = MPIU_Malloc(send_size);
        sbuf = snd_noncontig_buff;
        if(snd_noncontig_buff == NULL)
        {
           MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
              "Fatal:  Cannot allocate pack buffer");
        }
        if(sendcontinuous)
        {
           MPIR_Localcopy(recvbuf, totalsendcount, recvtype, 
                          snd_noncontig_buff, send_size,MPI_CHAR);
        }
        else
        {
           size_t extent; 
           MPID_Datatype_get_extent_macro(recvtype,extent);
           for(i=0; i<size; ++i)
           {
              char* scbuf = (char*)recvbuf + recvdispls[i]*extent;
              char* rcbuf = (char*)snd_noncontig_buff + psenddispls[i];
              MPIR_Localcopy(scbuf, recvcounts[i], recvtype, 
                             rcbuf, psendcounts[i], MPI_CHAR);
              TRACE_ERR("Pack send src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                        (size_t)extent, (size_t)i,(size_t)recvdispls[i],(size_t)i,(size_t)recvcounts[i],(size_t)recvdispls[i], *(int*)scbuf);
              TRACE_ERR("Pack send dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                        (size_t)extent, (size_t)i,(size_t)psenddispls[i],(size_t)i,(size_t)psendcounts[i],(size_t)psenddispls[i], *(int*)rcbuf);
           }
        }
      }
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
   if(inplace)
   {
     if(unlikely(verbose))
         fprintf(stderr,"alltoallv MPI_IN_PLACE buffering\n");
      alltoallv.cmd.xfer_alltoallv_int.stype = rtype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) precvdispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) precvcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = PAMI_IN_PLACE;
   }
   else
   {
      alltoallv.cmd.xfer_alltoallv_int.stype = stype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) psenddispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) psendcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = sbuf;
   }
   alltoallv.cmd.xfer_alltoallv_int.rcvbuf = rbuf;
      
   alltoallv.cmd.xfer_alltoallv_int.rdispls = (int *) precvdispls;
   alltoallv.cmd.xfer_alltoallv_int.rtypecounts = (int *) precvcounts;
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

   if(!rcv_contig || (recvok != PAMI_SUCCESS))
   {
      if(recvcontinuous)
      {
         MPIR_Localcopy(rcv_noncontig_buff, recv_size,MPI_CHAR,
                        recvbuf, totalrecvcount, recvtype);
      }
      else
      {
         size_t extent; 
         int i;
         MPID_Datatype_get_extent_macro(recvtype,extent);
         for(i=0; i<size; ++i)
         {
            char* scbuf = (char*)rcv_noncontig_buff+ precvdispls[i];
            char* rcbuf = (char*)recvbuf + recvdispls[i]*extent;
            MPIR_Localcopy(scbuf, precvcounts[i], MPI_CHAR, 
                           rcbuf, recvcounts[i], recvtype);
            TRACE_ERR("Pack recv src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                      (size_t)extent, (size_t)i,(size_t)precvdispls[i],(size_t)i,(size_t)precvcounts[i],(size_t)precvdispls[i], *(int*)scbuf);
            TRACE_ERR("Pack recv dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                      (size_t)extent, (size_t)i,(size_t)recvdispls[i],(size_t)i,(size_t)recvcounts[i],(size_t)recvdispls[i], *(int*)rcbuf);
         }
      }
      MPIU_Free(rcv_noncontig_buff);
   }
   if(!snd_contig || (sendok != PAMI_SUCCESS))  MPIU_Free(snd_noncontig_buff);
   if(lrecvdispls) MPIU_Free(lrecvdispls);
   if(lsenddispls) MPIU_Free(lsenddispls);
   if(lrecvcounts) MPIU_Free(lrecvcounts);
   if(lsendcounts) MPIU_Free(lsendcounts);

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
#ifndef HAVE_PAMI_IN_PLACE
  if (sendbuf == MPI_IN_PLACE)
  {
    MPID_Abort (NULL, 0, 1, "'MPI_IN_PLACE' requries support for `PAMI_IN_PLACE`");
    return -1;
  }
#endif
   TRACE_ERR("Entering MPIDO_Alltoallv_optimized\n");
   volatile unsigned active = 1;
  int sndtypelen, rcvtypelen, snd_contig = 1, rcv_contig = 1;
   MPID_Datatype *sdt, *rdt;
   pami_type_t stype = NULL, rtype;
   MPI_Aint sdt_true_lb = 0, rdt_true_lb;
   MPIDI_Post_coll_t alltoallv_post;
  void *snd_noncontig_buff = NULL, *rcv_noncontig_buff = NULL;
  void *sbuf = NULL, *rbuf = NULL;
  int recvok=PAMI_SUCCESS, sendok=PAMI_SUCCESS;
   int tmp;
   const int rank = comm_ptr->rank;
  const int size = comm_ptr->local_size;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

  int sendcontinuous , recvcontinuous=0;
  size_t recv_size=0, send_size=0;
  size_t totalrecvcount=0;
  int *lrecvdispls = NULL; /* possible local displs calculated for noncontinous */
  int *lsenddispls = NULL;/* possible local displs calculated for noncontinous */
  int *lrecvcounts  = NULL;/* possible local counts calculated for noncontinous */
  int *lsendcounts  = NULL;/* possible local counts calculated for noncontinous */
  const int *precvdispls = recvdispls; /* pointer to displs to use as pami parmi */
  const int *psenddispls = senddispls; /* pointer to displs to use as pami parmi */
  const int *precvcounts = recvcounts; /* pointer to counts to use as pami parmi */
  const int *psendcounts = sendcounts; /* pointer to counts to use as pami parmi */
  int inplace = sendbuf == MPI_IN_PLACE? 1 : 0;




   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   /* We don't pack and unpack in alltoallv as we do in alltoall because alltoallv has
      more overhead of book keeping for sendcounts/displs and recvcounts/displs. Since,
      alltoallv is non-rooted and all tasks has type info, decision to punt to mpich
      will be the same on all tasks */


  /* Check if collsel has MPICH algorithm as the best performing one, if so, call MPICH now w/o doing any conversions */
  MPIDI_Datatype_get_info(1, recvtype, rcv_contig, rcvtypelen, rdt, rdt_true_lb);
  if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
  {
    advisor_algorithm_t advisor_algorithms[1];
    int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_ALLTOALLV_INT, rcvtypelen * recvcounts[0], advisor_algorithms, 1);
    if(num_algorithms)
    {
      if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
      {
        return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                              recvbuf, recvcounts, recvdispls, recvtype,
                              comm_ptr, mpierrno);
       }
       else if(advisor_algorithms[0].metadata && advisor_algorithms[0].metadata->check_correct.values.asyncflowctl && !(--(comm_ptr->mpid.num_requests)))
       {
         comm_ptr->mpid.num_requests = MPIDI_Process.optimized.num_requests;
         int tmpmpierrno;
         if(unlikely(verbose))
           fprintf(stderr,"Query barrier required for %s\n", advisor_algorithms[0].metadata->name);
         MPIDO_Barrier(comm_ptr, &tmpmpierrno);
       }
     }
   }

  /* Now do checks on send data and datatypes (contig and contin) and do necessary conversions */
  if(!inplace)
   {
    sendok = MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp);
     MPIDI_Datatype_get_info(1, sendtype, snd_contig, sndtypelen, sdt, sdt_true_lb);
    sbuf = (char *)sendbuf + sdt_true_lb;
    if(!snd_contig || (sendok != PAMI_SUCCESS))
    {
      stype = PAMI_TYPE_UNSIGNED_CHAR;
      size_t totalsendcount = sendcounts[0];
      sendcontinuous = senddispls[0] == 0? 1 : 0 ;
      int i;
      psenddispls = lsenddispls = MPIU_Malloc(size*sizeof(int));
      psendcounts = lsendcounts = MPIU_Malloc(size*sizeof(int));
      lsenddispls[0]= 0;
      lsendcounts[0]= sndtypelen * sendcounts[0];
      for(i=1; i<size; ++i)
      {
        lsenddispls[i]= sndtypelen * totalsendcount;
        totalsendcount += sendcounts[i];
        if(senddispls[i] != (senddispls[i-1] + (sendcounts[i-1])))
          sendcontinuous = 0;
        lsendcounts[i]= sndtypelen * sendcounts[i];
   }
      send_size = sndtypelen * totalsendcount;
      TRACE_ERR("Pack receive sndv_contig %zu, sendok %zd, totalsendcount %zu, sendcontinuous %zu, sndtypelen %zu,  send_size %zu\n",
                (size_t)snd_contig, (size_t)sendok, (size_t)totalsendcount, (size_t)sendcontinuous, (size_t) sndtypelen, (size_t)send_size);
      snd_noncontig_buff = MPIU_Malloc(send_size);
      sbuf = snd_noncontig_buff;
      if(snd_noncontig_buff == NULL)
   {
        MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                   "Fatal:  Cannot allocate pack buffer");
      }
      if(sendcontinuous)
      {
        MPIR_Localcopy(sendbuf, totalsendcount, sendtype, 
                       snd_noncontig_buff, send_size,MPI_CHAR);
      }
      else
      {
        size_t extent; 
        MPID_Datatype_get_extent_macro(sendtype,extent);
        for(i=0; i<size; ++i)
        {
          char* scbuf = (char*)sendbuf + senddispls[i]*extent;
          char* rcbuf = (char*)snd_noncontig_buff + psenddispls[i];
          MPIR_Localcopy(scbuf, sendcounts[i], sendtype, 
                         rcbuf, psendcounts[i], MPI_CHAR);
          TRACE_ERR("Pack send src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                    (size_t)extent, (size_t)i,(size_t)senddispls[i],(size_t)i,(size_t)sendcounts[i],(size_t)senddispls[i], *(int*)scbuf);
          TRACE_ERR("Pack send dest             displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                    (size_t)i,(size_t)psenddispls[i],(size_t)i,(size_t)psendcounts[i],(size_t)psenddispls[i], *(int*)rcbuf);
        }
      }
    }
  }

  recvok = MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp);
  rbuf = (char *)recvbuf + rdt_true_lb;
  if(!rcv_contig || (recvok != PAMI_SUCCESS))
  {
    rtype = PAMI_TYPE_UNSIGNED_CHAR;
    totalrecvcount = recvcounts[0];
    recvcontinuous = recvdispls[0] == 0? 1 : 0 ;
    int i;
    precvdispls = lrecvdispls = MPIU_Malloc(size*sizeof(int));
    precvcounts = lrecvcounts = MPIU_Malloc(size*sizeof(int));
    lrecvdispls[0]= 0;
    lrecvcounts[0]= rcvtypelen * recvcounts[0];
    for(i=1; i<size; ++i)
    {
      lrecvdispls[i]= rcvtypelen * totalrecvcount;
      totalrecvcount += recvcounts[i];
      if(recvdispls[i] != (recvdispls[i-1] + (recvcounts[i-1])))
        recvcontinuous = 0;
      lrecvcounts[i]= rcvtypelen * recvcounts[i];
   }   
    recv_size = rcvtypelen * totalrecvcount;
    TRACE_ERR("Pack receive rcv_contig %zu, recvok %zd, totalrecvcount %zu, recvcontinuous %zu, rcvtypelen %zu, recv_size %zu\n",
              (size_t)rcv_contig, (size_t)recvok, (size_t)totalrecvcount, (size_t)recvcontinuous,(size_t)rcvtypelen, (size_t)recv_size);
    rcv_noncontig_buff = MPIU_Malloc(recv_size);
    rbuf = rcv_noncontig_buff;
    if(rcv_noncontig_buff == NULL)
    {
      MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                 "Fatal:  Cannot allocate pack buffer");
    }
    /* need to copy it now if it's used for the send buffer and then do not do in place */
    if(inplace)
    {
      inplace = 0;
      stype = PAMI_TYPE_UNSIGNED_CHAR;
      size_t totalsendcount = recvcounts[0];
      sendcontinuous = recvdispls[0] == 0? 1 : 0 ;
      int i;
      psenddispls = lsenddispls = MPIU_Malloc(size*sizeof(int));
      psendcounts = lsendcounts = MPIU_Malloc(size*sizeof(int));
      lsenddispls[0]= 0;
      lsendcounts[0]= rcvtypelen * recvcounts[0];
      for(i=1; i<size; ++i)
      {
        lsenddispls[i]= rcvtypelen * totalsendcount;
        totalsendcount += recvcounts[i];
        if(recvdispls[i] != (recvdispls[i-1] + (recvcounts[i-1])))
          sendcontinuous = 0;
        lsendcounts[i]= rcvtypelen * recvcounts[i];
      }
      send_size = rcvtypelen * totalsendcount;
      TRACE_ERR("Pack MPI_IN_PLACE receive sndv_contig %zu, sendok %zd, totalsendcount %zu, sendcontinuous %zu, rcvtypelen %zu,  send_size %zu\n",
                (size_t)snd_contig, (size_t)sendok, (size_t)totalsendcount, (size_t)sendcontinuous, (size_t) rcvtypelen, (size_t)send_size);
      snd_noncontig_buff = MPIU_Malloc(send_size);
      sbuf = snd_noncontig_buff;
      if(snd_noncontig_buff == NULL)
      {
        MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                   "Fatal:  Cannot allocate pack buffer");
      }
      if(sendcontinuous)
      {
        MPIR_Localcopy(recvbuf, totalsendcount, recvtype, 
                       snd_noncontig_buff, send_size,MPI_CHAR);
      }
      else
      {
        size_t extent; 
        MPID_Datatype_get_extent_macro(recvtype,extent);
        for(i=0; i<size; ++i)
        {
          char* scbuf = (char*)recvbuf + recvdispls[i]*extent;
          char* rcbuf = (char*)snd_noncontig_buff + psenddispls[i];
          MPIR_Localcopy(scbuf, recvcounts[i], recvtype, 
                         rcbuf, psendcounts[i], MPI_CHAR);
          TRACE_ERR("Pack send src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                    (size_t)extent, (size_t)i,(size_t)recvdispls[i],(size_t)i,(size_t)recvcounts[i],(size_t)recvdispls[i], *(int*)scbuf);
          TRACE_ERR("Pack send dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                    (size_t)extent, (size_t)i,(size_t)psenddispls[i],(size_t)i,(size_t)psendcounts[i],(size_t)psenddispls[i], *(int*)rcbuf);
        }
      }
    }
  }



   pami_xfer_t alltoallv;
   alltoallv.algorithm = mpid->coll_algorithm[PAMI_XFER_ALLTOALLV_INT][0][0];

   alltoallv.cb_done = cb_alltoallv;
   alltoallv.cookie = (void *)&active;
  if(inplace)
   {
    alltoallv.cmd.xfer_alltoallv_int.stype = rtype;
    alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) precvdispls;
    alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) precvcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = PAMI_IN_PLACE;
   }
  else
  {
    alltoallv.cmd.xfer_alltoallv_int.stype = stype;
    alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) psenddispls;
    alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) psendcounts;
    alltoallv.cmd.xfer_alltoallv_int.sndbuf = sbuf;
  }
  alltoallv.cmd.xfer_alltoallv_int.rcvbuf = rbuf;

  alltoallv.cmd.xfer_alltoallv_int.rdispls = (int *) precvdispls;
  alltoallv.cmd.xfer_alltoallv_int.rtypecounts = (int *) precvcounts;
   alltoallv.cmd.xfer_alltoallv_int.rtype = rtype;


   MPIDI_Context_post(MPIDI_Context[0], &alltoallv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&alltoallv);

   TRACE_ERR("%d waiting on active %d\n", rank, active);
   MPID_PROGRESS_WAIT_WHILE(active);

  if(!rcv_contig || (recvok != PAMI_SUCCESS))
  {
    if(recvcontinuous)
    {
      MPIR_Localcopy(rcv_noncontig_buff, recv_size,MPI_CHAR,
                     recvbuf, totalrecvcount, recvtype);
    }
    else
    {
      size_t extent; 
      int i;
      MPID_Datatype_get_extent_macro(recvtype,extent);
      for(i=0; i<size; ++i)
      {
        char* scbuf = (char*)rcv_noncontig_buff+ precvdispls[i];
        char* rcbuf = (char*)recvbuf + recvdispls[i]*extent;
        MPIR_Localcopy(scbuf, precvcounts[i], MPI_CHAR, 
                       rcbuf, recvcounts[i], recvtype);
        TRACE_ERR("Pack recv src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)precvdispls[i],(size_t)i,(size_t)precvcounts[i],(size_t)precvdispls[i], *(int*)scbuf);
        TRACE_ERR("Pack recv dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)recvdispls[i],(size_t)i,(size_t)recvcounts[i],(size_t)recvdispls[i], *(int*)rcbuf);
      }
    }
    MPIU_Free(rcv_noncontig_buff);
  }
  if(!snd_contig || (sendok != PAMI_SUCCESS))  MPIU_Free(snd_noncontig_buff);
  if(lrecvdispls) MPIU_Free(lrecvdispls);
  if(lsenddispls) MPIU_Free(lsenddispls);
  if(lrecvcounts) MPIU_Free(lrecvcounts);
  if(lsendcounts) MPIU_Free(lsendcounts);


   TRACE_ERR("Leaving alltoallv\n");


   return MPI_SUCCESS;
}

int
MPIDO_CSWrapper_alltoallv(pami_xfer_t *alltoallv,
                          void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype sendtype, recvtype;
   void *sbuf;
   MPIDI_coll_check_in_place(alltoallv->cmd.xfer_alltoallv_int.sndbuf, &sbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  alltoallv->cmd.xfer_alltoallv_int.stype,
                                   &sendtype,
                                    NULL,
                                    NULL);
   if(rc == -1) return rc;

   rc = MPIDI_Dtpami_to_dtmpi(  alltoallv->cmd.xfer_alltoallv_int.rtype,
                               &recvtype,
                                NULL,
                                NULL);
   if(rc == -1) return rc;

   rc  =  MPIR_Alltoallv(sbuf,
                         alltoallv->cmd.xfer_alltoallv_int.stypecounts, 
                         alltoallv->cmd.xfer_alltoallv_int.sdispls, sendtype,
                         alltoallv->cmd.xfer_alltoallv_int.rcvbuf,
                         alltoallv->cmd.xfer_alltoallv_int.rtypecounts,
                         alltoallv->cmd.xfer_alltoallv_int.rdispls, recvtype,
                          comm_ptr, &mpierrno);
   if(alltoallv->cb_done && rc == 0)
     alltoallv->cb_done(NULL, alltoallv->cookie, PAMI_SUCCESS);
   return rc;

}

