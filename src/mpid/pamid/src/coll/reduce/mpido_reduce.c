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
#ifndef HAVE_PAMI_IN_PLACE
  if (sendbuf == MPI_IN_PLACE)
  {
    MPID_Abort (NULL, 0, 1, "'MPI_IN_PLACE' requries support for `PAMI_IN_PLACE`");
    return -1;
  }
#endif
   MPID_Datatype *dt_null = NULL;
   MPI_Aint true_lb = 0;
   int dt_contig ATTRIBUTE((unused)), tsize;
   int mu;
   char *sbuf, *rbuf;
   pami_data_function pop;
   pami_type_t pdt;
   int rc;
   int alg_selected = 0;
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int selected_type = mpid->user_selected_type[PAMI_XFER_REDUCE];

   rc = MPIDI_Datatype_to_pami(datatype, &pdt, op, &pop, &mu);
   if(unlikely(verbose))
      fprintf(stderr,"reduce - rc %u, root %u, count %d, dt: %p, op: %p, mu: %u, selectedvar %u != %u (MPICH) sendbuf %p, recvbuf %p\n",
	      rc, root, count, pdt, pop, mu, 
	      (unsigned)selected_type, MPID_COLL_USE_MPICH,sendbuf, recvbuf);

   pami_xfer_t reduce;
   pami_algorithm_t my_reduce=0;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   int queryreq = 0;
   volatile unsigned reduce_active = 1;

   MPIDI_Datatype_get_info(count, datatype, dt_contig, tsize, dt_null, true_lb);
   rbuf = (char *)recvbuf + true_lb;
   sbuf = (char *)sendbuf + true_lb;
   if(sendbuf == MPI_IN_PLACE) 
   {
      if(unlikely(verbose))
	fprintf(stderr,"reduce MPI_IN_PLACE send buffering (%d,%d)\n",count,tsize);
      sbuf = PAMI_IN_PLACE;
   }

   reduce.cb_done = reduce_cb_done;
   reduce.cookie = (void *)&reduce_active;
   if(mpid->optreduce) /* GLUE_ALLREDUCE */
   {
      char* tbuf = NULL;
      if(unlikely(verbose))
         fprintf(stderr,"Using protocol GLUE_ALLREDUCE for reduce (%d,%d)\n",count,tsize);
      MPIDI_Update_last_algorithm(comm_ptr, "REDUCE_OPT_ALLREDUCE");
      void *destbuf = recvbuf;
      if(rank != root) /* temp buffer for non-root destbuf */
      {
         tbuf = destbuf = MPIU_Malloc(tsize);
      }
      /* Switch to comm->coll_fns->fn() */
      MPIDO_Allreduce(sendbuf,
                      destbuf,
                      count,
                      datatype,
                      op,
                      comm_ptr,
                      mpierrno);
      if(tbuf)
         MPIU_Free(tbuf);
      return 0;
   }
   if(selected_type == MPID_COLL_USE_MPICH || rc != MPI_SUCCESS)
   {
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH reduce algorithm\n");
#if CUDA_AWARE_SUPPORT
      if(MPIDI_Process.cuda_aware_support_on)
      {
         MPI_Aint dt_extent;
         MPID_Datatype_get_extent_macro(datatype, dt_extent);
         char *scbuf = NULL;
         char *rcbuf = NULL;
         int is_send_dev_buf = MPIDI_cuda_is_device_buf(sendbuf);
         int is_recv_dev_buf = MPIDI_cuda_is_device_buf(recvbuf);
         if(is_send_dev_buf)
         {
           scbuf = MPIU_Malloc(dt_extent * count);
           cudaError_t cudaerr = CudaMemcpy(scbuf, sendbuf, dt_extent * count, cudaMemcpyDeviceToHost);
           if (cudaSuccess != cudaerr) 
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
         }
         else
           scbuf = sendbuf;
         if(is_recv_dev_buf)
         {
           rcbuf = MPIU_Malloc(dt_extent * count);
           if(sendbuf == MPI_IN_PLACE)
           {
             cudaError_t cudaerr = CudaMemcpy(rcbuf, recvbuf, dt_extent * count, cudaMemcpyDeviceToHost);
             if (cudaSuccess != cudaerr)
               fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
           }
           else
             memset(rcbuf, 0, dt_extent * count);
         }
         else
           rcbuf = recvbuf;
         int cuda_res =  MPIR_Reduce(scbuf, rcbuf, count, datatype, op, root, comm_ptr, mpierrno);
         if(is_send_dev_buf)MPIU_Free(scbuf);
         if(is_recv_dev_buf)
         {
           cudaError_t cudaerr = CudaMemcpy(recvbuf, rcbuf, dt_extent * count, cudaMemcpyHostToDevice);
           if (cudaSuccess != cudaerr)
             fprintf(stderr, "cudaMemcpy failed: %s\n", CudaGetErrorString(cudaerr));
           MPIU_Free(rcbuf);
         }
         return cuda_res;
      }
      else
#endif
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
   }

   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      if((mpid->cutoff_size[PAMI_XFER_REDUCE][0] == 0) || 
          (mpid->cutoff_size[PAMI_XFER_REDUCE][0] >= tsize && mpid->cutoff_size[PAMI_XFER_REDUCE][0] > 0))
      {
        TRACE_ERR("Optimized Reduce (%s) was pre-selected\n",
         mpid->opt_protocol_md[PAMI_XFER_REDUCE][0].name);
        my_reduce    = mpid->opt_protocol[PAMI_XFER_REDUCE][0];
        my_md = &mpid->opt_protocol_md[PAMI_XFER_REDUCE][0];
        queryreq     = mpid->must_query[PAMI_XFER_REDUCE][0];
      }

   }
   else
   {
      TRACE_ERR("Optimized reduce (%s) was specified by user\n",
      mpid->user_metadata[PAMI_XFER_REDUCE].name);
      my_reduce    =  mpid->user_selected[PAMI_XFER_REDUCE];
      my_md = &mpid->user_metadata[PAMI_XFER_REDUCE];
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
   reduce.cmd.xfer_reduce.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);


   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || 
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("Querying reduce protocol %s, type was %d\n",
                my_md->name,
                queryreq);
      if(my_md->check_fn == NULL)
      {
         /* process metadata bits */
         if((!my_md->check_correct.values.inplace) && (sendbuf == MPI_IN_PLACE))
            result.check.unspecified = 1;
         if(my_md->check_correct.values.rangeminmax)
         {
            MPI_Aint data_true_lb ATTRIBUTE((unused));
            MPID_Datatype *data_ptr;
            int data_size, data_contig ATTRIBUTE((unused));
            MPIDI_Datatype_get_info(count, datatype, data_contig, data_size, data_ptr, data_true_lb); 
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
         result = my_md->check_fn(&reduce);
      TRACE_ERR("Bitmask: %#X\n", result.bitmask);
      result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
      if(result.bitmask)
      {
         if(unlikely(verbose))
            fprintf(stderr,"Query failed for %s.  Using MPICH reduce.\n",
                    my_md->name);
      }  
      else 
      {   
         if(my_md->check_correct.values.asyncflowctl && !(--(comm_ptr->mpid.num_requests))) 
         { 
            comm_ptr->mpid.num_requests = MPIDI_Process.optimized.num_requests;
            int tmpmpierrno;   
            if(unlikely(verbose))
               fprintf(stderr,"Query barrier required for %s\n", my_md->name);
            MPIDO_Barrier(comm_ptr, &tmpmpierrno);
         }
         alg_selected = 1;
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
                 my_md->name,
              (unsigned) comm_ptr->context_id);
      }
      MPIDI_Post_coll_t reduce_post;
      MPIDI_Context_post(MPIDI_Context[0], &reduce_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&reduce);
   }
   else
   {
      MPIDI_Update_last_algorithm(comm_ptr, "REDUCE_MPICH");
      if(unlikely(verbose))
         fprintf(stderr,"Using MPICH reduce algorithm\n");
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
   }

   MPIDI_Update_last_algorithm(comm_ptr,
                               my_md->name);
   MPID_PROGRESS_WAIT_WHILE(reduce_active);
   TRACE_ERR("Reduce done\n");
   return 0;
}


int MPIDO_Reduce_simple(const void *sendbuf, 
                 void *recvbuf, 
                 int count, 
                 MPI_Datatype datatype,
                 MPI_Op op, 
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
   MPID_Datatype *dt_null = NULL;
   MPI_Aint true_lb = 0;
   int dt_contig, tsize;
   int mu;
   char *sbuf, *rbuf;
   pami_data_function pop;
   pami_type_t pdt;
   int rc;
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);

   MPIDI_Datatype_get_info(count, datatype, dt_contig, tsize, dt_null, true_lb);
   if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
   {
     advisor_algorithm_t advisor_algorithms[1];
     int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_REDUCE, tsize, advisor_algorithms, 1);
     if(num_algorithms)
     {
       if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
       {
         return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
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

   rc = MPIDI_Datatype_to_pami(datatype, &pdt, op, &pop, &mu);

   pami_xfer_t reduce;
   const pami_metadata_t *my_reduce_md=NULL;
   volatile unsigned reduce_active = 1;

   if(rc != MPI_SUCCESS || !dt_contig)
   {
      return MPIR_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm_ptr, mpierrno);
   }


   rbuf = (char *)recvbuf + true_lb;
   sbuf = (char *)sendbuf + true_lb;
   if(sendbuf == MPI_IN_PLACE) 
   {
      sbuf = PAMI_IN_PLACE;
   }

   reduce.cb_done = reduce_cb_done;
   reduce.cookie = (void *)&reduce_active;
   reduce.algorithm = mpid->coll_algorithm[PAMI_XFER_REDUCE][0][0];
   reduce.cmd.xfer_reduce.sndbuf = sbuf;
   reduce.cmd.xfer_reduce.rcvbuf = rbuf;
   reduce.cmd.xfer_reduce.stype = pdt;
   reduce.cmd.xfer_reduce.rtype = pdt;
   reduce.cmd.xfer_reduce.stypecount = count;
   reduce.cmd.xfer_reduce.rtypecount = count;
   reduce.cmd.xfer_reduce.op = pop;
   reduce.cmd.xfer_reduce.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
   my_reduce_md = &mpid->coll_metadata[PAMI_XFER_REDUCE][0][0];

   TRACE_ERR("%s reduce, context %d, algoname: %s, exflag: %d\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking", 0,
                my_reduce_md->name, exflag);
   MPIDI_Post_coll_t reduce_post;
   MPIDI_Context_post(MPIDI_Context[0], &reduce_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&reduce);
   TRACE_ERR("Reduce %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");

   MPIDI_Update_last_algorithm(comm_ptr,
                               my_reduce_md->name);
   MPID_PROGRESS_WAIT_WHILE(reduce_active);
   TRACE_ERR("Reduce done\n");
   return MPI_SUCCESS;
}


int
MPIDO_CSWrapper_reduce(pami_xfer_t *reduce,
                       void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype type;
   MPI_Op op;
   void *sbuf;
   MPIDI_coll_check_in_place(reduce->cmd.xfer_reduce.sndbuf, &sbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  reduce->cmd.xfer_reduce.stype,
                                   &type,
                                    reduce->cmd.xfer_reduce.op,
                                   &op);
   if(rc == -1) return rc;


   rc  =  MPIR_Reduce(sbuf,
                      reduce->cmd.xfer_reduce.rcvbuf,
                      reduce->cmd.xfer_reduce.rtypecount, type, op,
                      reduce->cmd.xfer_reduce.root, comm_ptr, &mpierrno);
   if(reduce->cb_done && rc == 0)
     reduce->cb_done(NULL, reduce->cookie, PAMI_SUCCESS);
   return rc;

}

