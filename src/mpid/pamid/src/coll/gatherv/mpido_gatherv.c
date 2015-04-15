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
#ifndef HAVE_PAMI_IN_PLACE
  if (sendbuf == MPI_IN_PLACE)
  {
    MPID_Abort (NULL, 0, 1, "'MPI_IN_PLACE' requries support for `PAMI_IN_PLACE`");
    return -1;
  }
#endif
   TRACE_ERR("Entering MPIDO_Gatherv\n");
   int i;
   int contig ATTRIBUTE((unused)), rsize ATTRIBUTE((unused)), ssize ATTRIBUTE((unused));
   int pamidt = 1;
   MPID_Datatype *dt_ptr = NULL;
   MPI_Aint send_true_lb, recv_true_lb;
   char *sbuf, *rbuf;
   pami_type_t stype, rtype;
   int tmp;
   volatile unsigned gatherv_active = 1;
   const int rank = comm_ptr->rank;
   const int size = comm_ptr->local_size;
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
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint sdt_extent,rdt_extent;
       MPID_Datatype_get_extent_macro(sendtype, sdt_extent);
       MPID_Datatype_get_extent_macro(recvtype, rdt_extent);
       char *scbuf = NULL;
       char *rcbuf = NULL;
       int is_send_dev_buf = MPIDI_cuda_is_device_buf(sendbuf);
       int is_recv_dev_buf = (rank == root) ? MPIDI_cuda_is_device_buf(recvbuf) : 0;
       if(is_send_dev_buf)
       {
         scbuf = MPIU_Malloc(sdt_extent * sendcount);
         cudaError_t cudaerr = CudaMemcpy(scbuf, sendbuf, sdt_extent * sendcount, cudaMemcpyDeviceToHost);
         if (cudaSuccess != cudaerr)
           fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
       }
       else
         scbuf = sendbuf;
       size_t rtotal_buf;
       if(is_recv_dev_buf)
       {
         //Since displs can be non-continous, we need to calculate max buffer size 
         int highest_displs = displs[size - 1];
         int highest_recvcount = recvcounts[size - 1];
         for(i = 0; i < size; i++)
         {
           if(displs[i]+recvcounts[i] > highest_displs+highest_recvcount)
           {
             highest_displs = displs[i];
             highest_recvcount = recvcounts[i];
           }
         }
         rtotal_buf = (highest_displs+highest_recvcount)*rdt_extent;
         rcbuf = MPIU_Malloc(rtotal_buf);
         if(sendbuf == MPI_IN_PLACE)
         {
           cudaError_t cudaerr = CudaMemcpy(rcbuf, recvbuf, rtotal_buf, cudaMemcpyDeviceToHost);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
         }
         else
           memset(rcbuf, 0, rtotal_buf);
       }
       else
         rcbuf = recvbuf;
       int cuda_res =  MPIR_Gatherv(scbuf, sendcount, sendtype, rcbuf, recvcounts, displs, recvtype, root, comm_ptr, mpierrno);
       if(is_send_dev_buf)MPIU_Free(scbuf);
       if(is_recv_dev_buf)
         {
           cudaError_t cudaerr = CudaMemcpy(recvbuf, rcbuf, rtotal_buf, cudaMemcpyHostToDevice);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
           MPIU_Free(rcbuf);
         }
       return cuda_res;
    }
    else
#endif
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
   gatherv.cmd.xfer_gatherv_int.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
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
#ifndef HAVE_PAMI_IN_PLACE
  if (sendbuf == MPI_IN_PLACE)
  {
    MPID_Abort (NULL, 0, 1, "'MPI_IN_PLACE' requries support for `PAMI_IN_PLACE`");
    return -1;
  }
#endif
   TRACE_ERR("Entering MPIDO_Gatherv_optimized\n");
   int snd_contig = 1, rcv_contig = 1;
   void *snd_noncontig_buff = NULL, *rcv_noncontig_buff = NULL;
   void *sbuf = NULL, *rbuf = NULL;
   int  *rcounts = NULL;
   int  *rdispls = NULL;
   int send_size = 0;
   int recv_size = 0;
   int rcvlen    = 0;
  int totalrecvcount  = 0;
   pami_type_t rtype = PAMI_TYPE_NULL;
   MPID_Segment segment;
   MPID_Datatype *data_ptr = NULL;
   int send_true_lb, recv_true_lb = 0;
   int i, tmp;
   volatile unsigned gatherv_active = 1;
   const int rank = comm_ptr->rank;
   const int size = comm_ptr->local_size;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
  int recvok=PAMI_SUCCESS, recvcontinuous=0;

   if(sendbuf != MPI_IN_PLACE)
   {
     MPIDI_Datatype_get_info(sendcount, sendtype, snd_contig,
                            send_size, data_ptr, send_true_lb);
    if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
    {
      advisor_algorithm_t advisor_algorithms[1];
      int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_GATHERV_INT, 64, advisor_algorithms, 1);
      if(num_algorithms)
      {
        if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
        {
          return MPIR_Gatherv(sendbuf, sendcount, sendtype,
                              recvbuf, recvcounts, displs, recvtype,
                              root, comm_ptr, mpierrno);
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

    sbuf = (char *)sendbuf + send_true_lb;
    if(!snd_contig)
    {
      snd_noncontig_buff = MPIU_Malloc(send_size);
      sbuf = snd_noncontig_buff;
      if(snd_noncontig_buff == NULL)
      {
        MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                   "Fatal:  Cannot allocate pack buffer");
      }
      DLOOP_Offset last = send_size;
      MPID_Segment_init(sendbuf, sendcount, sendtype, &segment, 0);
      MPID_Segment_pack(&segment, 0, &last, snd_noncontig_buff);
    }
  }
  else
  {
    MPIDI_Datatype_get_info(1, recvtype, rcv_contig,
                            rcvlen, data_ptr, recv_true_lb);
    if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
    {
      advisor_algorithm_t advisor_algorithms[1];
      int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_GATHERV_INT, 64, advisor_algorithms, 1);
      if(num_algorithms)
      {
        if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
        {
          return MPIR_Gatherv(sendbuf, sendcount, sendtype,
                              recvbuf, recvcounts, displs, recvtype,
                              root, comm_ptr, mpierrno);
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
  }

   pami_xfer_t gatherv;
   rbuf = (char *)recvbuf + recv_true_lb;
   rcounts = (int*)recvcounts;
   rdispls = (int*)displs;
   if(rank == root)
   {
    if((recvok = MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp)) != MPI_SUCCESS)
      {
        MPIDI_Datatype_get_info(1, recvtype, rcv_contig,
                                rcvlen, data_ptr, recv_true_lb);
      totalrecvcount = recvcounts[0];
      recvcontinuous = displs[0] == 0? 1 : 0 ;
          rcounts = (int*)MPIU_Malloc(size);
          rdispls = (int*)MPIU_Malloc(size);
      rdispls[0] = 0;
      rcounts[0] = rcvlen * recvcounts[0];
      for(i = 1; i < size; i++)
      {
        rdispls[i]= rcvlen * totalrecvcount;
        totalrecvcount += recvcounts[i];
        if(displs[i] != (displs[i-1] + recvcounts[i-1]))
          recvcontinuous = 0;
            rcounts[i] = rcvlen * recvcounts[i];
          }
      recv_size = rcvlen * totalrecvcount;

          rcv_noncontig_buff = MPIU_Malloc(recv_size);
          rbuf = rcv_noncontig_buff;
          rtype = PAMI_TYPE_BYTE;
          if(rcv_noncontig_buff == NULL)
          {
             MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                "Fatal:  Cannot allocate pack buffer");
          }
      if(sendbuf == MPI_IN_PLACE)
      {
        size_t extent;
        MPID_Datatype_get_extent_macro(recvtype,extent);
        MPIR_Localcopy(recvbuf + displs[rank]*extent, recvcounts[rank], recvtype,
                     rcv_noncontig_buff + rdispls[rank], rcounts[rank],MPI_CHAR);
      }
    }
    if(sendbuf == MPI_IN_PLACE)
    {
      gatherv.cmd.xfer_gatherv_int.sndbuf = PAMI_IN_PLACE;
    }
    else
    {
      gatherv.cmd.xfer_gatherv_int.sndbuf = sbuf;
    }
    gatherv.cmd.xfer_gatherv_int.stype = PAMI_TYPE_BYTE;/* stype is ignored when sndbuf == PAMI_IN_PLACE */
    gatherv.cmd.xfer_gatherv_int.stypecount = send_size;

  }
  else
  {
    gatherv.cmd.xfer_gatherv_int.sndbuf = sbuf;
    gatherv.cmd.xfer_gatherv_int.stype = PAMI_TYPE_BYTE;
    gatherv.cmd.xfer_gatherv_int.stypecount = send_size;     
  }


  gatherv.cb_done = cb_gatherv;
  gatherv.cookie = (void *)&gatherv_active;
  gatherv.cmd.xfer_gatherv_int.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
  gatherv.cmd.xfer_gatherv_int.rcvbuf = rbuf;
  gatherv.cmd.xfer_gatherv_int.rtype = rtype;
  gatherv.cmd.xfer_gatherv_int.rtypecounts = (int *) rcounts;
  gatherv.cmd.xfer_gatherv_int.rdispls = (int *) rdispls;


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

  if(!rcv_contig || recvok != PAMI_SUCCESS)
  {
    if(recvcontinuous)
   {
      MPIR_Localcopy(rcv_noncontig_buff, recv_size, MPI_CHAR,
                     recvbuf,   totalrecvcount,     recvtype);
    }
    else
    {
      size_t extent;
      MPID_Datatype_get_extent_macro(recvtype,extent);
      for(i=0; i<size; ++i)
      {
        char* scbuf = (char*)rcv_noncontig_buff+ rdispls[i];
        char* rcbuf = (char*)recvbuf + displs[i]*extent;
        MPIR_Localcopy(scbuf, rcounts[i], MPI_CHAR,
                       rcbuf, recvcounts[i], recvtype);
        TRACE_ERR("Pack recv src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)precvdispls[i],(size_t)i,(size_t)precvcounts[i],(size_t)precvdispls[i], *(int*)scbuf);
        TRACE_ERR("Pack recv dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)displs[i],(size_t)i,(size_t)recvcounts[i],(size_t)displs[i], *(int*)rcbuf);
      }

    }
      MPIU_Free(rcv_noncontig_buff);
      if(rank == root)
      {
         MPIU_Free(rcounts);
         MPIU_Free(rdispls);
      }
   }
   if(!snd_contig)  MPIU_Free(snd_noncontig_buff);


   TRACE_ERR("Leaving MPIDO_Gatherv_optimized\n");
   return MPI_SUCCESS;
}

int
MPIDO_CSWrapper_gatherv(pami_xfer_t *gatherv,
                        void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype sendtype, recvtype;
   void *sbuf;
   MPIDI_coll_check_in_place(gatherv->cmd.xfer_gatherv_int.sndbuf, &sbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  gatherv->cmd.xfer_gatherv_int.stype,
                                   &sendtype,
                                    NULL,
                                    NULL);
   if(rc == -1) return rc;

   if(gatherv->cmd.xfer_gatherv_int.rtype == PAMI_TYPE_NULL)
     recvtype = MPI_DATATYPE_NULL;
   else
     rc = MPIDI_Dtpami_to_dtmpi(  gatherv->cmd.xfer_gatherv_int.rtype,
                               &recvtype,
                                NULL,
                                NULL);
   if(rc == -1) return rc;

   rc  =  MPIR_Gatherv(sbuf,
                       gatherv->cmd.xfer_gatherv_int.stypecount, sendtype,
                       gatherv->cmd.xfer_gatherv_int.rcvbuf,
                       gatherv->cmd.xfer_gatherv_int.rtypecounts,
                       gatherv->cmd.xfer_gatherv_int.rdispls, recvtype,
                       gatherv->cmd.xfer_gatherv_int.root, comm_ptr, &mpierrno);
   if(gatherv->cb_done && rc == 0)
     gatherv->cb_done(NULL, gatherv->cookie, PAMI_SUCCESS);
   return rc;

}

