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
 * \file src/coll/allgatherv/mpido_allgatherv.c
 * \brief ???
 */

/* #define TRACE_ON */
#include <mpidimpl.h>

static void allgatherv_cb_done(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *)clientdata;
   (*active)--;
}

static void allred_cb_done(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *)clientdata;
   (*active)--;
}


/* ****************************************************************** */
/**
 * \brief Use (tree) MPIDO_Allreduce() to do a fast Allgatherv operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - The recv buffer is continuous
 *       - Tree allreduce is availible (for max performance)
 */
/* ****************************************************************** */
#define MAX_ALLGATHERV_ALLREDUCE_BUFFER_SIZE (1024*1024*2)
int MPIDO_Allgatherv_allreduce(const void *sendbuf,
			       int sendcount,
			       MPI_Datatype sendtype,
			       void *recvbuf,
			       const int *recvcounts,
			       int buffer_sum,
			       const int *displs,
			       MPI_Datatype recvtype,
			       MPI_Aint send_true_lb,
			       MPI_Aint recv_true_lb,
			       size_t send_size,
			       size_t recv_size,
			       MPID_Comm * comm_ptr,
                               int *mpierrno)
{
  int start, rc, i;
  int length;
  char *startbuf = NULL;
  char *destbuf = NULL;
  const int rank = comm_ptr->rank;
  TRACE_ERR("Entering MPIDO_Allgatherv_allreduce\n");

  startbuf = (char *) recvbuf + recv_true_lb;
  destbuf = startbuf + displs[rank] * recv_size;

  if (sendbuf != MPI_IN_PLACE)
  {
    char *outputbuf = (char *) sendbuf + send_true_lb;
    memcpy(destbuf, outputbuf, send_size);
  }

  //printf("buffer_sum %d, send_size %d recv_size %d\n", buffer_sum, 
  // (int)send_size, (int)recv_size);	 

  /* TODO: Change to PAMI */
  /*integer/long/double allgathers only*/
  /*Do a convert and then do the allreudce*/
  if ( buffer_sum <= MAX_ALLGATHERV_ALLREDUCE_BUFFER_SIZE &&
       (send_size & 0x3)==0 && (recv_size & 0x3)==0)  
  {
    double *tmprbuf = (double *)MPIU_Malloc(buffer_sum*2);
    if (tmprbuf == NULL)
      goto direct_algo; /*skip int to fp conversion and go to direct
			  algo*/

    double *tmpsbuf = tmprbuf + (displs[rank]*recv_size)/sizeof(int);
    int *sibuf = (int *) destbuf;
    
    memset(tmprbuf, 0, displs[rank]*recv_size*2);
    start  = (displs[rank] + recvcounts[rank]) * recv_size;   
    length = buffer_sum - (displs[rank] + recvcounts[rank]) * recv_size;
    memset(tmprbuf + start/sizeof(int), 0, length*2);

    for(i = 0; i < (send_size/sizeof(int)); ++i) 
      tmpsbuf[i] = (double)sibuf[i];
    
    /* Switch to comm->coll_fns->fn() */
    rc = MPIDO_Allreduce(MPI_IN_PLACE,
			 tmprbuf,
			 buffer_sum/sizeof(int),
			 MPI_DOUBLE,
			 MPI_SUM,
			 comm_ptr,
			 mpierrno);
    
    sibuf = (int *) startbuf;
    for(i = 0; i < (displs[rank]*recv_size/sizeof(int)); ++i) 
      sibuf[i] = (int)tmprbuf[i];
    
    for(i = start/sizeof(int); i < buffer_sum/sizeof(int); ++i) 
      sibuf[i] = (int)tmprbuf[i];

    MPIU_Free(tmprbuf);
    return rc;
  }

 direct_algo:

  start = 0;
  length = displs[rank] * recv_size;
  memset(startbuf + start, 0, length);

  start  = (displs[rank] +
	    recvcounts[rank]) * recv_size;
  length = buffer_sum - (displs[rank] +
			 recvcounts[rank]) * recv_size;
  memset(startbuf + start, 0, length);

  TRACE_ERR("Calling MPIDO_Allreduce from MPIDO_Allgatherv_allreduce\n");
  /* Switch to comm->coll_fns->fn() */
  rc = MPIDO_Allreduce(MPI_IN_PLACE,
		       startbuf,
		       buffer_sum/sizeof(unsigned),
		       MPI_UNSIGNED,
		       MPI_BOR,
		       comm_ptr,
                       mpierrno);

  TRACE_ERR("Leaving MPIDO_Allgatherv_allreduce\n");
  return rc;
}

/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Bcast() to do a fast Allgatherv operation
 *
 * \note This function requires one of these (for max performance):
 *       - Tree broadcast
 *       - Rect broadcast
 *       ? Binomial broadcast
 */
/* ****************************************************************** */
int MPIDO_Allgatherv_bcast(const void *sendbuf,
			   int sendcount,
			   MPI_Datatype sendtype,
			   void *recvbuf,
			   const int *recvcounts,
			   int buffer_sum,
			   const int *displs,
			   MPI_Datatype recvtype,
			   MPI_Aint send_true_lb,
			   MPI_Aint recv_true_lb,
			   size_t send_size,
			   size_t recv_size,
			   MPID_Comm * comm_ptr,
                           int *mpierrno)
{
   const int rank = comm_ptr->rank;
   TRACE_ERR("Entering MPIDO_Allgatherv_bcast\n");
  int i, rc=MPI_ERR_INTERN;
  MPI_Aint extent;
  MPID_Datatype_get_extent_macro(recvtype, extent);

  if (sendbuf != MPI_IN_PLACE)
  {
    void *destbuffer = recvbuf + displs[rank] * extent;
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   destbuffer,
                   recvcounts[rank],
                   recvtype);
  }

   TRACE_ERR("Calling MPIDO_Bcasts in MPIDO_Allgatherv_bcast\n");
  for (i = 0; i < comm_ptr->local_size; i++)
  {
    void *destbuffer = recvbuf + displs[i] * extent;
    /* Switch to comm->coll_fns->fn() */
    rc = MPIDO_Bcast(destbuffer,
                     recvcounts[i],
                     recvtype,
                     i,
                     comm_ptr,
                     mpierrno);
  }
  TRACE_ERR("Leaving MPIDO_Allgatherv_bcast\n");

  return rc;
}

/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Alltoall() to do a fast Allgatherv operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - DMA alltoallv is availible (for max performance)
 */
/* ****************************************************************** */

int MPIDO_Allgatherv_alltoall(const void *sendbuf,
			      int sendcount,
			      MPI_Datatype sendtype,
			      void *recvbuf,
			      int *recvcounts,
			      int buffer_sum,
			      const int *displs,
			      MPI_Datatype recvtype,
			      MPI_Aint send_true_lb,
			      MPI_Aint recv_true_lb,
			      size_t send_size,
			      size_t recv_size,
			      MPID_Comm * comm_ptr,
                              int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Allgatherv_alltoallv\n");
  size_t total_send_size;
  char *startbuf;
  char *destbuf;
  int i, rc;
  int my_recvcounts = -1;
  void *a2a_sendbuf = NULL;
  const int size = comm_ptr->local_size;
  int a2a_sendcounts[size];
  int a2a_senddispls[size];
   const int rank = comm_ptr->rank;

  total_send_size = recvcounts[rank] * recv_size;
  for (i = 0; i < size; ++i)
  {
    a2a_sendcounts[i] = total_send_size;
    a2a_senddispls[i] = 0;
  }
  if (sendbuf != MPI_IN_PLACE)
  {
    a2a_sendbuf = (char *)sendbuf + send_true_lb;
  }
  else
  {
    startbuf = (char *) recvbuf + recv_true_lb;
    destbuf = startbuf + displs[rank] * recv_size;
    a2a_sendbuf = destbuf;
    a2a_sendcounts[rank] = 0;
    my_recvcounts = recvcounts[rank];
    recvcounts[rank] = 0;
  }

   TRACE_ERR("Calling alltoallv in MPIDO_Allgatherv_alltoallv\n");
   /* Switch to comm->coll_fns->fn() */
  rc = MPIDO_Alltoallv(a2a_sendbuf,
		       a2a_sendcounts,
		       a2a_senddispls,
		       MPI_CHAR,
		       recvbuf,
		       recvcounts,
		       displs,
		       recvtype,
		       comm_ptr,
		       mpierrno);
  if (sendbuf == MPI_IN_PLACE)
    recvcounts[rank] = my_recvcounts;

   TRACE_ERR("Leaving MPIDO_Allgatherv_alltoallv\n");
  return rc;
}


int
MPIDO_Allgatherv(const void *sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 const int *recvcounts,
		 const int *displs,
		 MPI_Datatype recvtype,
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
   TRACE_ERR("Entering MPIDO_Allgatherv\n");
  /* function pointer to be used to point to approperiate algorithm */

  /* Check the nature of the buffers */
  MPID_Datatype *dt_null = NULL;
  MPI_Aint send_true_lb  = 0;
  MPI_Aint recv_true_lb  = 0;
  size_t   send_size     = 0;
  size_t   recv_size     = 0;
  int config[6];
  int scount=sendcount;

  int i, rc, buffer_sum = 0;
  const int size = comm_ptr->local_size;
  char use_tree_reduce, use_alltoall, use_bcast, use_pami, use_opt;
  char *sbuf, *rbuf;
  const int rank = comm_ptr->rank;
  const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
  int queryreq = 0;

#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
   const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const int selected_type = mpid->user_selected_type[PAMI_XFER_ALLGATHERV_INT];

  pami_xfer_t allred;
  volatile unsigned allred_active = 1;
  volatile unsigned allgatherv_active = 1;
  pami_type_t stype, rtype;
  int tmp;
  const pami_metadata_t *my_md = (pami_metadata_t *)NULL;

  for(i=0;i<6;i++) config[i] = 1;

  allred.cb_done = allred_cb_done;
  allred.cookie = (void *)&allred_active;
  allred.algorithm = mpid->coll_algorithm[PAMI_XFER_ALLREDUCE][0][0];
  allred.cmd.xfer_allreduce.sndbuf = (void *)config;
  allred.cmd.xfer_allreduce.stype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.rcvbuf = (void *)config;
  allred.cmd.xfer_allreduce.rtype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.stypecount = 6;
  allred.cmd.xfer_allreduce.rtypecount = 6;
  allred.cmd.xfer_allreduce.op = PAMI_DATA_BAND;

  use_alltoall = mpid->allgathervs[2];
  use_tree_reduce = mpid->allgathervs[0];
  use_bcast = mpid->allgathervs[1];
  use_pami = selected_type != MPID_COLL_USE_MPICH;
	 
   if((sendbuf != MPI_IN_PLACE) && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
     use_pami = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
     use_pami = 0;

   use_opt = use_alltoall || use_tree_reduce || use_bcast || use_pami;

   if(!use_opt) /* back to MPICH */
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using MPICH allgatherv type %u.\n",
             selected_type);
     TRACE_ERR("Using MPICH Allgatherv\n");
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_MPICH");
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint sdt_extent,rdt_extent;
       MPID_Datatype_get_extent_macro(sendtype, sdt_extent);
       MPID_Datatype_get_extent_macro(recvtype, rdt_extent);
       char *scbuf = NULL;
       char *rcbuf = NULL;
       int is_send_dev_buf = MPIDI_cuda_is_device_buf(sendbuf);
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
       int cuda_res =  MPIR_Allgatherv(scbuf, sendcount, sendtype, rcbuf, recvcounts, displs, recvtype, comm_ptr, mpierrno);
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
     return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
			   recvbuf, recvcounts, displs, recvtype,
                          comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(1,
			  recvtype,
			  config[MPID_RECV_CONTIG],
			  recv_size,
			  dt_null,
			  recv_true_lb);

   if(sendbuf == MPI_IN_PLACE)
   {
     sbuf = PAMI_IN_PLACE;
     if(unlikely(verbose))
       fprintf(stderr,"allgatherv MPI_IN_PLACE buffering\n");
     stype = rtype;
     scount = recvcounts[rank];
     send_size = recv_size * scount; 
   }
   else
   {
      MPIDI_Datatype_get_info(sendcount,
                              sendtype,
                              config[MPID_SEND_CONTIG],
                              send_size,
                              dt_null,
                              send_true_lb);
       sbuf = (char *)sendbuf+send_true_lb;
   }

   rbuf = (char *)recvbuf+recv_true_lb;

   if(use_alltoall || use_bcast || use_tree_reduce)
   {
      if (displs[0])
       config[MPID_RECV_CONTINUOUS] = 0;

      for (i = 1; i < size; i++)
      {
        buffer_sum += recvcounts[i - 1];
        if (buffer_sum != displs[i])
        {
          config[MPID_RECV_CONTINUOUS] = 0;
          break;
        }
      }

      buffer_sum += recvcounts[size - 1];

      buffer_sum *= recv_size;

      /* disable with "safe allgatherv" env var */
      if(mpid->preallreduces[MPID_ALLGATHERV_PREALLREDUCE])
      {
         MPIDI_Post_coll_t allred_post;
         MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                            MPIDI_Pami_post_wrapper, (void *)&allred);

         MPID_PROGRESS_WAIT_WHILE(allred_active);
      }

      use_tree_reduce = mpid->allgathervs[0] &&
         config[MPID_RECV_CONTIG] && config[MPID_SEND_CONTIG] &&
         config[MPID_RECV_CONTINUOUS] && buffer_sum % sizeof(unsigned) == 0;

      use_alltoall = mpid->allgathervs[2] &&
         config[MPID_RECV_CONTIG] && config[MPID_SEND_CONTIG];

      use_bcast = mpid->allgathervs[1];
   }

   if(use_pami)
   {
      pami_xfer_t allgatherv;
      allgatherv.cb_done = allgatherv_cb_done;
      allgatherv.cookie = (void *)&allgatherv_active;
      if(selected_type == MPID_COLL_OPTIMIZED)
      {
        if((mpid->cutoff_size[PAMI_XFER_ALLGATHERV_INT][0] == 0) || 
           (mpid->cutoff_size[PAMI_XFER_ALLGATHERV_INT][0] > 0 && mpid->cutoff_size[PAMI_XFER_ALLGATHERV_INT][0] >= send_size))
        {		
          allgatherv.algorithm = mpid->opt_protocol[PAMI_XFER_ALLGATHERV_INT][0];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLGATHERV_INT][0];
          queryreq     = mpid->must_query[PAMI_XFER_ALLGATHERV_INT][0];
        }
        else
          return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                       recvbuf, recvcounts, displs, recvtype,
                       comm_ptr, mpierrno);
      }
      else
      {  
        allgatherv.algorithm = mpid->user_selected[PAMI_XFER_ALLGATHERV_INT];
        my_md = &mpid->user_metadata[PAMI_XFER_ALLGATHERV_INT];
        queryreq     = selected_type;
      }
      
      allgatherv.cmd.xfer_allgatherv_int.sndbuf = sbuf;
      allgatherv.cmd.xfer_allgatherv_int.rcvbuf = rbuf;

      allgatherv.cmd.xfer_allgatherv_int.stype = stype;
      allgatherv.cmd.xfer_allgatherv_int.rtype = rtype;
      allgatherv.cmd.xfer_allgatherv_int.stypecount = scount;
      allgatherv.cmd.xfer_allgatherv_int.rtypecounts = (int *) recvcounts;
      allgatherv.cmd.xfer_allgatherv_int.rdispls = (int *) displs;

      if(unlikely (queryreq == MPID_COLL_ALWAYS_QUERY ||
                   queryreq == MPID_COLL_CHECK_FN_REQUIRED))
      {
         metadata_result_t result = {0};
         TRACE_ERR("Querying allgatherv_int protocol %s, type was %d\n", my_md->name,
            selected_type);
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
           result = my_md->check_fn(&allgatherv);
         TRACE_ERR("Allgatherv bitmask: %#X\n", result.bitmask);
         result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
         if(result.bitmask)
         {
           if(unlikely(verbose))
             fprintf(stderr,"Query failed for %s. Using MPICH allgatherv.\n", my_md->name);
           MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_MPICH");
           return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                                  recvbuf, recvcounts, displs, recvtype,
                                  comm_ptr, mpierrno);
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
         fprintf(stderr,"<%llx> Using protocol %s for allgatherv on %u\n", 
                 threadID,
                 my_md->name,
              (unsigned) comm_ptr->context_id);
      }
      MPIDI_Post_coll_t allgatherv_post;
      MPIDI_Context_post(MPIDI_Context[0], &allgatherv_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&allgatherv);

      MPIDI_Update_last_algorithm(comm_ptr, my_md->name);

      TRACE_ERR("Rank %d waiting on active %d\n", rank, allgatherv_active);
      MPID_PROGRESS_WAIT_WHILE(allgatherv_active);

      return PAMI_SUCCESS;
   }

   /* TODO These need ordered in speed-order */
   if(use_tree_reduce)
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using tree reduce allgatherv type %u.\n",
               selected_type);
     rc = MPIDO_Allgatherv_allreduce(sendbuf, sendcount, sendtype,
             recvbuf, recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_ALLREDUCE");
     return rc;
   }

   if(use_bcast)
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using bcast allgatherv type %u.\n",
               selected_type);
     rc = MPIDO_Allgatherv_bcast(sendbuf, sendcount, sendtype,
             recvbuf, recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_BCAST");
     return rc;
   }

   if(use_alltoall)
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using alltoall allgatherv type %u.\n",
               selected_type);
     rc = MPIDO_Allgatherv_alltoall(sendbuf, sendcount, sendtype,
             recvbuf, (int *)recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_ALLTOALL");
     return rc;
   }

   if(unlikely(verbose))
      fprintf(stderr,"Using MPICH allgatherv type %u.\n",
            selected_type);
   TRACE_ERR("Using MPICH for Allgatherv\n");
   MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_MPICH");
   return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                       recvbuf, recvcounts, displs, recvtype,
                       comm_ptr, mpierrno);
}

int
MPIDO_Allgatherv_simple(const void *sendbuf,
		 int sendcount,
		 MPI_Datatype sendtype,
		 void *recvbuf,
		 const int *recvcounts,
		 const int *displs,
		 MPI_Datatype recvtype,
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
   TRACE_ERR("Entering MPIDO_Allgatherv_optimized\n");
  /* function pointer to be used to point to approperiate algorithm */
  /* Check the nature of the buffers */
  MPID_Datatype *dt_null = NULL;
  MPI_Aint send_true_lb  = 0;
  MPI_Aint recv_true_lb  = 0;
  size_t   send_size     = 0;
  size_t   recv_size     = 0;
  size_t   rcvtypelen    = 0;
  int snd_data_contig = 0, rcv_data_contig = 0;
  void *snd_noncontig_buff = NULL, *rcv_noncontig_buff = NULL;
  int scount=sendcount;

  char *sbuf, *rbuf;
  pami_type_t stype = NULL, rtype;
  const int rank = comm_ptr->rank;
  const int size = comm_ptr->local_size;
  const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
   const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

  int recvcontinuous=0;
  size_t totalrecvcount=0;
  int *lrecvdispls = NULL; /* possible local displs calculated for noncontinous */
  int *lrecvcounts  = NULL;/* possible local counts calculated for noncontinous */
  const int *precvdispls = displs; /* pointer to displs to use as pami parmi */
  const int *precvcounts = recvcounts; /* pointer to counts to use as pami parmi */
  int inplace = sendbuf == MPI_IN_PLACE? 1 : 0;


  volatile unsigned allgatherv_active = 1;
  int recvok=PAMI_SUCCESS, sendok=PAMI_SUCCESS;
  int tmp;
  const pami_metadata_t *my_md;


   MPIDI_Datatype_get_info(1,
                          recvtype,
                          rcv_data_contig,
                          rcvtypelen,
                          dt_null,
                          recv_true_lb);

  if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
  {
    advisor_algorithm_t advisor_algorithms[1];
    int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_ALLGATHERV_INT, rcvtypelen * recvcounts[0], advisor_algorithms, 1);
     if(num_algorithms)
     {
       if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
       {
         return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                       recvbuf, recvcounts, displs, recvtype,
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


  if(!inplace)
   {
    sendok = MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp);
    MPIDI_Datatype_get_info(sendcount, sendtype, snd_data_contig, send_size, dt_null, send_true_lb);
    sbuf = (char *)sendbuf + send_true_lb;
    if(!snd_data_contig || (sendok != PAMI_SUCCESS))
   {
      stype  = PAMI_TYPE_UNSIGNED_CHAR;
      scount = send_size;
      if(!snd_data_contig)
   {
        snd_noncontig_buff = MPIU_Malloc(send_size);
        sbuf = snd_noncontig_buff;
        if(snd_noncontig_buff == NULL)
   {
          MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                   "Fatal:  Cannot allocate pack buffer");
   }
        MPIR_Localcopy(sendbuf, sendcount, sendtype,
                       snd_noncontig_buff, send_size,MPI_CHAR);
      }
    }
  }
  else
    sbuf = PAMI_IN_PLACE;

  recvok = MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp);
   rbuf = (char *)recvbuf+recv_true_lb;
  if(!rcv_data_contig || (recvok != PAMI_SUCCESS))
  {
    rtype = PAMI_TYPE_UNSIGNED_CHAR;
    totalrecvcount = recvcounts[0];
    recvcontinuous = displs[0] == 0? 1 : 0 ;
    int i;
    precvdispls = lrecvdispls = MPIU_Malloc(size*sizeof(int));
    precvcounts = lrecvcounts = MPIU_Malloc(size*sizeof(int));
    lrecvdispls[0]= 0;
    lrecvcounts[0]= rcvtypelen * recvcounts[0];
    for(i=1; i<size; ++i)
    {
      lrecvdispls[i]= rcvtypelen * totalrecvcount;
      totalrecvcount += recvcounts[i];
      if(displs[i] != (displs[i-1] + recvcounts[i-1]))
        recvcontinuous = 0;
      lrecvcounts[i]= rcvtypelen * recvcounts[i];
    }
    recv_size = rcvtypelen * totalrecvcount;
    TRACE_ERR("Pack receive rcv_contig %zu, recvok %zd, totalrecvcount %zu, recvcontinuous %zu, rcvtypelen %zu, recv_size %zu\n",
              (size_t)rcv_data_contig, (size_t)recvok, (size_t)totalrecvcount, (size_t)recvcontinuous,(size_t)rcvtypelen, (size_t)recv_size);

    rcv_noncontig_buff = MPIU_Malloc(recv_size);
    rbuf = rcv_noncontig_buff;
    if(rcv_noncontig_buff == NULL)
    {
      MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
                 "Fatal:  Cannot allocate pack buffer");
    }
    if(inplace)
    {
      size_t extent;
      MPID_Datatype_get_extent_macro(recvtype,extent);
      MPIR_Localcopy(recvbuf + displs[rank]*extent, recvcounts[rank], recvtype,
                     rcv_noncontig_buff + precvdispls[rank], precvcounts[rank],MPI_CHAR);
      scount = precvcounts[rank];
      stype   = PAMI_TYPE_UNSIGNED_CHAR;
      sbuf    = PAMI_IN_PLACE;
    }
   }


   pami_xfer_t allgatherv;
   allgatherv.cb_done = allgatherv_cb_done;
   allgatherv.cookie = (void *)&allgatherv_active;
   allgatherv.cmd.xfer_allgatherv_int.sndbuf = sbuf;
   allgatherv.cmd.xfer_allgatherv_int.rcvbuf = rbuf;
   allgatherv.cmd.xfer_allgatherv_int.stype = stype;/* stype is ignored when sndbuf == PAMI_IN_PLACE */
   allgatherv.cmd.xfer_allgatherv_int.rtype = rtype;
   allgatherv.cmd.xfer_allgatherv_int.stypecount = scount;
  allgatherv.cmd.xfer_allgatherv_int.rtypecounts = (int *) precvcounts;
  allgatherv.cmd.xfer_allgatherv_int.rdispls = (int *) precvdispls;
   allgatherv.algorithm = mpid->coll_algorithm[PAMI_XFER_ALLGATHERV_INT][0][0];
   my_md = &mpid->coll_metadata[PAMI_XFER_ALLGATHERV_INT][0][0];

   TRACE_ERR("Calling allgatherv via %s()\n", MPIDI_Process.context_post.active>0?"PAMI_Collective":"PAMI_Context_post");
   MPIDI_Post_coll_t allgatherv_post;
   MPIDI_Context_post(MPIDI_Context[0], &allgatherv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&allgatherv);

   MPIDI_Update_last_algorithm(comm_ptr, my_md->name);

   TRACE_ERR("Rank %d waiting on active %d\n", rank, allgatherv_active);
   MPID_PROGRESS_WAIT_WHILE(allgatherv_active);

  if(!rcv_data_contig || (recvok != PAMI_SUCCESS))
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
        char* rcbuf = (char*)recvbuf + displs[i]*extent;
        MPIR_Localcopy(scbuf, precvcounts[i], MPI_CHAR,
                       rcbuf, recvcounts[i], recvtype);
        TRACE_ERR("Pack recv src  extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)precvdispls[i],(size_t)i,(size_t)precvcounts[i],(size_t)precvdispls[i], *(int*)scbuf);
        TRACE_ERR("Pack recv dest extent %zu, displ[%zu]=%zu, count[%zu]=%zu buf[%zu]=%u\n",
                  (size_t)extent, (size_t)i,(size_t)displs[i],(size_t)i,(size_t)recvcounts[i],(size_t)displs[i], *(int*)rcbuf);
      }
    }
    MPIU_Free(rcv_noncontig_buff);
  }
  if(!snd_data_contig)  MPIU_Free(snd_noncontig_buff);
  if(lrecvdispls) MPIU_Free(lrecvdispls);
  if(lrecvcounts) MPIU_Free(lrecvcounts);

   return MPI_SUCCESS;
}


int
MPIDO_CSWrapper_allgatherv(pami_xfer_t *allgatherv,
                           void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype sendtype, recvtype;
   void *sbuf;
   MPIDI_coll_check_in_place(allgatherv->cmd.xfer_allgatherv_int.sndbuf, &sbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  allgatherv->cmd.xfer_allgatherv_int.stype,
                                   &sendtype,
                                    NULL,
                                    NULL);
   if(rc == -1) return rc;

   rc = MPIDI_Dtpami_to_dtmpi(  allgatherv->cmd.xfer_allgatherv_int.rtype,
                               &recvtype,
                                NULL,
                                NULL);
   if(rc == -1) return rc;

   rc  =  MPIR_Allgatherv(sbuf,
                          allgatherv->cmd.xfer_allgatherv_int.stypecount, sendtype,
                          allgatherv->cmd.xfer_allgatherv_int.rcvbuf,
                          allgatherv->cmd.xfer_allgatherv_int.rtypecounts,
                          allgatherv->cmd.xfer_allgatherv_int.rdispls, recvtype,
                          comm_ptr, &mpierrno);
   if(allgatherv->cb_done && rc == 0)
     allgatherv->cb_done(NULL, allgatherv->cookie, PAMI_SUCCESS);
   return rc;


}

