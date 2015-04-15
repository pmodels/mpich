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
 * \file src/coll/scatter/mpido_scatter.c
 * \brief ???
 */

/* #define TRACE_ON */

#include <mpidimpl.h>

static void cb_scatter(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_scatter enter, active: %u\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}

/* works for simple data types, assumes fast bcast is available */
int MPIDO_Scatter_bcast(void * sendbuf,
			int sendcount,
			MPI_Datatype sendtype,
			void *recvbuf,
			int recvcount,
			MPI_Datatype recvtype,
			int root,
			MPID_Comm *comm_ptr,
                        int *mpierrno)
{
  /* Pretty simple - bcast a temp buffer and copy our little chunk out */
  int nbytes, rc;
  const int rank = comm_ptr->rank;
  const int size = comm_ptr->local_size;
  char *tempbuf = NULL;

  if(rank == root)
  {
    MPIDI_Datatype_get_data_size(sendcount,
                            sendtype,
                            nbytes);
    tempbuf = sendbuf;
  }
  else
  {
    MPIDI_Datatype_get_data_size(recvcount,
                            recvtype,
                            nbytes);

    tempbuf = MPIU_Malloc(nbytes * size);
    if(!tempbuf)
    {
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  __FUNCTION__,
                                  __LINE__,
                                  MPI_ERR_OTHER,
                                  "**nomem", 0);
    }
  }

  /* Switch to comm->coll_fns->fn() */
  rc = MPIDO_Bcast(tempbuf, nbytes*size, MPI_CHAR, root, comm_ptr, mpierrno);

  if(rank == root && recvbuf == MPI_IN_PLACE)
    return rc;
  else
    memcpy(recvbuf, tempbuf+(rank*nbytes), nbytes);

  if (rank!=root)
    MPIU_Free(tempbuf);

  return rc;
}


int MPIDO_Scatter(const void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  int root,
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
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb ATTRIBUTE((unused));
  int contig, nbytes ATTRIBUTE((unused));
  const int rank = comm_ptr->rank;
  int success = 1;
  pami_type_t stype, rtype;
  int tmp;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_SCATTER];
   char use_pami = !(selected_type == MPID_COLL_USE_MPICH);

  /* if (rank == root)
     We can't decide on just the root to use MPICH. Really need a pre-allreduce.
     For now check sendtype on non-roots too and hope it matches? I think that's what
     scatterv does... */
  {
    if(MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS)
      use_pami = 0;
  }
  if(recvbuf != MPI_IN_PLACE && (MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS))
    use_pami = 0;

  if(!use_pami)
  {
    if(unlikely(verbose))
      fprintf(stderr,"Using MPICH scatter algorithm\n");
    MPIDI_Update_last_algorithm(comm_ptr, "SCATTER_MPICH");
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint sdt_extent,rdt_extent;
       MPID_Datatype_get_extent_macro(sendtype, sdt_extent);
       MPID_Datatype_get_extent_macro(recvtype, rdt_extent);
       char *scbuf = NULL;
       char *rcbuf = NULL;
       int is_send_dev_buf = (rank == root) ? MPIDI_cuda_is_device_buf(sendbuf) : 0;
       int is_recv_dev_buf = MPIDI_cuda_is_device_buf(recvbuf);
       if(is_send_dev_buf)
       {
         scbuf = MPIU_Malloc(sdt_extent * sendcount);
         cudaError_t cudaerr = CudaMemcpy(scbuf, sendbuf, sdt_extent * sendcount, cudaMemcpyDeviceToHost);
         if (cudaSuccess != cudaerr)
           fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
       }
       else
         scbuf = sendbuf;
       if(is_recv_dev_buf)
       {
         rcbuf = MPIU_Malloc(rdt_extent * recvcount);
         CudaMemcpy(rcbuf, recvbuf, rdt_extent * recvcount, cudaMemcpyDeviceToHost);
       }
       else
         rcbuf = recvbuf;
       int cuda_res =  MPIR_Scatter(scbuf, sendcount, sendtype, rcbuf, recvcount, recvtype, root, comm_ptr, mpierrno);
       if(is_send_dev_buf)MPIU_Free(scbuf);
       if(is_recv_dev_buf)
         {
           cudaError_t cudaerr = CudaMemcpy(recvbuf, rcbuf, rdt_extent * recvcount, cudaMemcpyHostToDevice);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
           MPIU_Free(rcbuf);
         }
       return cuda_res;
    }
    else
#endif
    return MPIR_Scatter(sendbuf, sendcount, sendtype,
                        recvbuf, recvcount, recvtype,
                        root, comm_ptr, mpierrno);
  }

  if (rank == root)
  {
    if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)/* Should this be send or recv? */
    {
      MPIDI_Datatype_get_info(sendcount, sendtype, contig,
                              nbytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;

    if (success)
    {
      if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
      {
        MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                                nbytes, data_ptr, true_lb);
        if (!contig) success = 0;
      }
      else success = 0;
    }
  }

  else
  {
    if (sendtype != MPI_DATATYPE_NULL && sendcount >= 0)/* Should this be send or recv? */
    {
      MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                              nbytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;
  }


   pami_xfer_t scatter;
   MPIDI_Post_coll_t scatter_post;
   pami_algorithm_t my_scatter;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   volatile unsigned scatter_active = 1;
   int queryreq = 0;

   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized scatter %s was selected\n",
         mpid->opt_protocol_md[PAMI_XFER_SCATTER][0].name);
      my_scatter = mpid->opt_protocol[PAMI_XFER_SCATTER][0];
      my_md = &mpid->opt_protocol_md[PAMI_XFER_SCATTER][0];
      queryreq = mpid->must_query[PAMI_XFER_SCATTER][0];
   }
   else
   {
      TRACE_ERR("Optimized scatter %s was set by user\n",
         mpid->user_metadata[PAMI_XFER_SCATTER].name);
      my_scatter = mpid->user_selected[PAMI_XFER_SCATTER];
      my_md = &mpid->user_metadata[PAMI_XFER_SCATTER];
      queryreq = selected_type;
   }
 
   scatter.algorithm = my_scatter;
   scatter.cb_done = cb_scatter;
   scatter.cookie = (void *)&scatter_active;
   scatter.cmd.xfer_scatter.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
   scatter.cmd.xfer_scatter.sndbuf = (void *)sendbuf;
   scatter.cmd.xfer_scatter.stype = stype;
   scatter.cmd.xfer_scatter.stypecount = sendcount;
   if(recvbuf == MPI_IN_PLACE) 
   {
     if(unlikely(verbose))
       fprintf(stderr,"scatter MPI_IN_PLACE buffering\n");
     MPIDI_Datatype_get_info(sendcount, sendtype, contig,
                             nbytes, data_ptr, true_lb);
     scatter.cmd.xfer_scatter.rcvbuf = PAMI_IN_PLACE;
     scatter.cmd.xfer_scatter.rtype = stype;
     scatter.cmd.xfer_scatter.rtypecount = sendcount;
   }
   else
   {
     scatter.cmd.xfer_scatter.rcvbuf = (void *)recvbuf;
     scatter.cmd.xfer_scatter.rtype = rtype;
     scatter.cmd.xfer_scatter.rtypecount = recvcount;
   }

   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY ||
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying scatter protoocl %s, type was %d\n",
         my_md->name, queryreq);
      if(my_md->check_fn == NULL)
      {
        /* process metadata bits */
        if((!my_md->check_correct.values.inplace) && (recvbuf == MPI_IN_PLACE))
           result.check.unspecified = 1;
         if(my_md->check_correct.values.rangeminmax)
         {
           int data_size;
           if(rank == root)
             MPIDI_Datatype_get_data_size(sendcount, sendtype, data_size); 
           else
             MPIDI_Datatype_get_data_size(recvcount, recvtype, data_size); 
           if((my_md->range_lo <= data_size) &&
              (my_md->range_hi >= data_size))
              ; /* ok, algorithm selected */
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
      }
      else /* calling the check fn is sufficient */
        result = my_md->check_fn(&scatter);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
      if(result.bitmask)
      {
        if(unlikely(verbose))
          fprintf(stderr,"query failed for %s\n", my_md->name);
        MPIDI_Update_last_algorithm(comm_ptr, "SCATTER_MPICH");
        return MPIR_Scatter(sendbuf, sendcount, sendtype,
                            recvbuf, recvcount, recvtype,
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

   if(unlikely(verbose))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for scatter on %u\n", 
              threadID,
              my_md->name,
              (unsigned) comm_ptr->context_id);
   }
   MPIDI_Context_post(MPIDI_Context[0], &scatter_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&scatter);
   TRACE_ERR("Waiting on active %d\n", scatter_active);
   MPID_PROGRESS_WAIT_WHILE(scatter_active);


   TRACE_ERR("Leaving MPIDO_Scatter\n");

   return 0;
}


int MPIDO_Scatter_simple(const void *sendbuf,
                  int sendcount,
                  MPI_Datatype sendtype,
                  void *recvbuf,
                  int recvcount,
                  MPI_Datatype recvtype,
                  int root,
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
  MPID_Datatype * data_ptr;
  const int rank = comm_ptr->rank;
  int success = 1, snd_contig = 1, rcv_contig = 1;
  void *snd_noncontig_buff = NULL, *rcv_noncontig_buff = NULL;
  void *sbuf = NULL, *rbuf = NULL;
  size_t send_size = 0, recv_size = 0, data_size = 0;
  MPI_Aint snd_true_lb = 0, rcv_true_lb = 0; 
  MPID_Segment segment;
  const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
  const int size = comm_ptr->local_size;
  advisor_algorithm_t advisor_algorithms[1];

  if (rank == root)
  {
      MPIDI_Datatype_get_info(sendcount, sendtype, snd_contig,
                              send_size, data_ptr, snd_true_lb);

    if (recvbuf != MPI_IN_PLACE && recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
    {  
      MPIDI_Datatype_get_info(recvcount, recvtype, rcv_contig,
                              recv_size, data_ptr, rcv_true_lb);
    }
    data_size = send_size;
  }
  else if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
  {
    MPIDI_Datatype_get_info(recvcount, recvtype, rcv_contig,
                            recv_size, data_ptr, rcv_true_lb);
    data_size = recv_size;
  }

  advisor_algorithms[0].metadata = NULL;/* We check for NULL further down */
  if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
  {
    int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query,
                           PAMI_XFER_SCATTER, data_size, advisor_algorithms, 1);
    if(num_algorithms)
    {
      if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
      {
        return MPIR_Scatter(sendbuf, sendcount, sendtype,
                          recvbuf, recvcount, recvtype,
                          root, comm_ptr, mpierrno);
      }
    }
  }
  sbuf = (char *)sendbuf + snd_true_lb;
  rbuf = (char *)recvbuf + rcv_true_lb;
  if (rank == root)
  {    
    if (send_size)
    {
      if (!snd_contig)
      {
        snd_noncontig_buff = MPIU_Malloc(send_size * size);
        sbuf = snd_noncontig_buff;
        if(snd_noncontig_buff == NULL)
        {
           MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
              "Fatal:  Cannot allocate pack buffer");
        }
        DLOOP_Offset last = send_size * size;
        MPID_Segment_init(sendbuf, sendcount * size, sendtype, &segment, 0);
        MPID_Segment_pack(&segment, 0, &last, snd_noncontig_buff);
      }
    }
    else
      success = 0;

    if (success && recvbuf != MPI_IN_PLACE)
    {
      if (recv_size)
      {
        if (!rcv_contig)
        {
          rcv_noncontig_buff = MPIU_Malloc(recv_size);
          rbuf = rcv_noncontig_buff;
          if(rcv_noncontig_buff == NULL)
          {
             MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                "Fatal:  Cannot allocate pack buffer");
          }
        }
      }
      else success = 0;
    }
  }
  else
  {
    if (recv_size)/* Should this be send or recv? */
    {
      if (!rcv_contig)
      {
        rcv_noncontig_buff = MPIU_Malloc(recv_size);
        rbuf = rcv_noncontig_buff;
        if(rcv_noncontig_buff == NULL)
        {
           MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
              "Fatal:  Cannot allocate pack buffer");
        }
      }
    }
    else
      success = 0;
  }
  
  if(!success)
  {
    MPIDI_Update_last_algorithm(comm_ptr, "SCATTER_MPICH");
    return MPIR_Scatter(sendbuf, sendcount, sendtype,
                        recvbuf, recvcount, recvtype,
                        root, comm_ptr, mpierrno);
  }

  if(advisor_algorithms[0].metadata &&
     advisor_algorithms[0].metadata->check_correct.values.asyncflowctl &&
     !(--(comm_ptr->mpid.num_requests)))
  {
    comm_ptr->mpid.num_requests = MPIDI_Process.optimized.num_requests;
    int tmpmpierrno;
    MPIDO_Barrier(comm_ptr, &tmpmpierrno);
  }

   pami_xfer_t scatter;
   MPIDI_Post_coll_t scatter_post;
   volatile unsigned scatter_active = 1;

 
   scatter.algorithm = mpid->coll_algorithm[PAMI_XFER_SCATTER][0][0];

   scatter.cb_done = cb_scatter;
   scatter.cookie = (void *)&scatter_active;
   scatter.cmd.xfer_scatter.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
   scatter.cmd.xfer_scatter.sndbuf = (void *)sbuf;
   scatter.cmd.xfer_scatter.stype = PAMI_TYPE_BYTE;
   scatter.cmd.xfer_scatter.stypecount = send_size;
   scatter.cmd.xfer_scatter.rcvbuf = (void *)rbuf;
   scatter.cmd.xfer_scatter.rtype = PAMI_TYPE_BYTE;/* rtype is ignored when rcvbuf == PAMI_IN_PLACE */
   scatter.cmd.xfer_scatter.rtypecount = recv_size;

   if(recvbuf == MPI_IN_PLACE) 
   {
     scatter.cmd.xfer_scatter.rcvbuf = PAMI_IN_PLACE;
   }
       
   TRACE_ERR("%s scatter\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
   MPIDI_Context_post(MPIDI_Context[0], &scatter_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&scatter);
   TRACE_ERR("Waiting on active %d\n", scatter_active);
   MPID_PROGRESS_WAIT_WHILE(scatter_active);

   if(!rcv_contig && rcv_noncontig_buff)
   {
      MPIR_Localcopy(rcv_noncontig_buff, recv_size, MPI_CHAR,
                        recvbuf,         recvcount,     recvtype);
      MPIU_Free(rcv_noncontig_buff);
   }
   if(!snd_contig)  MPIU_Free(snd_noncontig_buff);


   TRACE_ERR("Leaving MPIDO_Scatter_optimized\n");

   return MPI_SUCCESS;
}


int
MPIDO_CSWrapper_scatter(pami_xfer_t *scatter,
                        void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype sendtype, recvtype;
   void *rbuf;
   MPIDI_coll_check_in_place(scatter->cmd.xfer_scatter.rcvbuf, &rbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  scatter->cmd.xfer_scatter.stype,
                                   &sendtype,
                                    NULL,
                                    NULL);
   if(rc == -1) return rc;

   rc = MPIDI_Dtpami_to_dtmpi(  scatter->cmd.xfer_scatter.rtype,
                               &recvtype,
                                NULL,
                                NULL);
   if(rc == -1) return rc;

   rc  =  MPIR_Scatter(scatter->cmd.xfer_scatter.sndbuf,
                       scatter->cmd.xfer_scatter.stypecount, sendtype,
                       rbuf,
                       scatter->cmd.xfer_scatter.rtypecount, recvtype,
                       scatter->cmd.xfer_scatter.root, comm_ptr, &mpierrno);
   if(scatter->cb_done && rc == 0)
     scatter->cb_done(NULL, scatter->cookie, PAMI_SUCCESS);
   return rc;

}

#if 0 /* old glue-based scatter-via-bcast */

  /* Assume glue protocol sucks for now.... Needs work if not or to enable */



  /* see if we all agree to use bcast scatter */
  MPIDO_Allreduce(MPI_IN_PLACE, &success, 1, MPI_INT, MPI_BAND, comm_ptr, mpierrno);

  if (!success)
  {
    if(unlikely(verbose))
      fprintf(stderr,"Using MPICH scatter algorithm\n");
    return MPIR_Scatter(sendbuf, sendcount, sendtype,
                        recvbuf, recvcount, recvtype,
                        root, comm_ptr, mpierrno);
  }

   sbuf = (char *)sendbuf+true_lb;
   rbuf = (char *)recvbuf+true_lb;

   MPIDI_Update_last_algorithm(comm_ptr, "SCATTER_OPT_BCAST");
   return MPIDO_Scatter_bcast(sbuf, sendcount, sendtype,
                              rbuf, recvcount, recvtype,
                              root, comm_ptr, mpierrno);
#endif
