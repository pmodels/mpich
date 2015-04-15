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
 * \file src/coll/allreduce/mpido_allreduce.c
 * \brief ???
 */

/* #define TRACE_ON */

#include <mpidimpl.h>
/* 
#undef TRACE_ERR
#define TRACE_ERR(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
*/
static void cb_allreduce(void *ctxt, void *clientdata, pami_result_t err)
{
  int *active = (int *) clientdata;
  TRACE_ERR("callback enter, active: %d\n", (*active));
  MPIDI_Progress_signal();
  (*active)--;
}

int MPIDO_Allreduce(const void *sendbuf,
                    void *recvbuf,
                    int count,
                    MPI_Datatype dt,
                    MPI_Op op,
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
  void *sbuf;
  TRACE_ERR("Entering mpido_allreduce\n");
  pami_type_t pdt;
  pami_data_function pop;
  int mu;
  int rc;
#ifdef TRACE_ON
  int len; 
  char op_str[255]; 
  char dt_str[255]; 
  MPIDI_Op_to_string(op, op_str); 
  PMPI_Type_get_name(dt, dt_str, &len); 
#endif
  volatile unsigned active = 1;
  pami_xfer_t allred;
  pami_algorithm_t my_allred = 0;
  const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
  int alg_selected = 0;
  const int rank = comm_ptr->rank;
  const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
  const int selected_type = mpid->user_selected_type[PAMI_XFER_ALLREDUCE];
#if ASSERT_LEVEL==0
  /* We can't afford the tracing in ndebug/performance libraries */
  const unsigned verbose = 0;
#else
  const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
  int queryreq = 0;
  if(likely(dt == MPI_DOUBLE || dt == MPI_DOUBLE_PRECISION))
  {
    rc = MPI_SUCCESS;
    pdt = PAMI_TYPE_DOUBLE;
    if(likely(selected_type == MPID_COLL_OPTIMIZED) &&
       (mpid->query_cached_allreduce != MPID_COLL_USE_MPICH))
    {
      /* double protocol works on all message sizes */
      my_allred = mpid->cached_allreduce;
      my_md = &mpid->cached_allreduce_md;
      alg_selected = 1;
    }
    if(likely(op == MPI_SUM))
      pop = PAMI_DATA_SUM;
    else if(likely(op == MPI_MAX))
      pop = PAMI_DATA_MAX;
    else if(likely(op == MPI_MIN))
      pop = PAMI_DATA_MIN;
    else 
    {
      alg_selected = 0;
      rc = MPIDI_Datatype_to_pami(dt, &pdt, op, &pop, &mu);
    }
  }
  else rc = MPIDI_Datatype_to_pami(dt, &pdt, op, &pop, &mu);

  if(unlikely(verbose))
    fprintf(stderr,"allred rc %u,count %d, Datatype %p, op %p, mu %u, selectedvar %u != %u, sendbuf %p, recvbuf %p\n",
            rc, count, pdt, pop, mu, 
            (unsigned)selected_type,MPID_COLL_USE_MPICH, sendbuf, recvbuf);
  /* convert to metadata query */
  /* Punt count 0 allreduce to MPICH. Let them do whatever's 'right' */
  if(unlikely(rc != MPI_SUCCESS || (count==0) ||
              selected_type == MPID_COLL_USE_MPICH))
  {
    if(unlikely(verbose))
      fprintf(stderr,"Using MPICH allreduce type %u.\n",
              selected_type);
    MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
#if CUDA_AWARE_SUPPORT
    if(MPIDI_Process.cuda_aware_support_on)
    {
       MPI_Aint dt_extent;
       MPID_Datatype_get_extent_macro(dt, dt_extent);
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
       int cuda_res =  MPIR_Allreduce(scbuf, rcbuf, count, dt, op, comm_ptr, mpierrno);
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
    return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
  }

  sbuf = (void *)sendbuf;
  if(unlikely(sendbuf == MPI_IN_PLACE))
   {
     if(unlikely(verbose))
         fprintf(stderr,"allreduce MPI_IN_PLACE buffering\n");
      sbuf = PAMI_IN_PLACE;
   }

  allred.cb_done = cb_allreduce;
  allred.cookie = (void *)&active;
  allred.cmd.xfer_allreduce.sndbuf = sbuf;
  allred.cmd.xfer_allreduce.stype = pdt;
  allred.cmd.xfer_allreduce.rcvbuf = recvbuf;
  allred.cmd.xfer_allreduce.rtype = pdt;
  allred.cmd.xfer_allreduce.stypecount = count;
  allred.cmd.xfer_allreduce.rtypecount = count;
  allred.cmd.xfer_allreduce.op = pop;

  TRACE_ERR("Allreduce - Basic Collective Selection\n");

  if(unlikely(!alg_selected)) /* Cached double algorithm not selected above */
  {
    if(likely(selected_type == MPID_COLL_OPTIMIZED))
    {
      if(mpid->query_cached_allreduce != MPID_COLL_USE_MPICH)
      { /* try the cached algorithm first, assume it's always a query algorithm so query now */
        my_allred = mpid->cached_allreduce;
        my_md = &mpid->cached_allreduce_md;
        alg_selected = 1;
        if(my_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
        {
          metadata_result_t result = {0};
          TRACE_ERR("querying allreduce algorithm %s\n",
                  my_md->name);
          result = my_md->check_fn(&allred);
          TRACE_ERR("bitmask: %#X\n", result.bitmask);
          /* \todo Ignore check_correct.values.nonlocal until we implement the
                   'pre-allreduce allreduce' or the 'safe' environment flag.
                   We will basically assume 'safe' -- that all ranks are aligned (or not).
          */
          result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
          if(!result.bitmask)
            ; /* ok, algorithm selected */
          else
          {
            alg_selected = 0;
            if(unlikely(verbose))
              fprintf(stderr,"check_fn failed for %s.\n", my_md->name);
          }
        }
        else /* no check_fn, manually look at the metadata fields */
        {
          TRACE_ERR("Optimzed selection line %d\n",__LINE__);
          /* Check if the message range if restricted */
          if(my_md->check_correct.values.rangeminmax)
          {
            int data_size;
            MPIDI_Datatype_get_data_size(count, dt, data_size); 
            if((my_md->range_lo <= data_size) &&
               (my_md->range_hi >= data_size))
              ; /* ok, algorithm selected */
            else
            {
              if(unlikely(verbose))
                fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                        data_size,
                        my_md->range_lo,
                        my_md->range_hi,
                        my_md->name);
              alg_selected = 0;
            }
          }
          /* \todo check the rest of the metadata */
        }
      }
      /* If we didn't use the cached protocol above (query failed?) then check regular optimized protocol fields */
      if(!alg_selected)
      {
        const int queryreq0 = mpid->must_query[PAMI_XFER_ALLREDUCE][0];
        const int queryreq1 = mpid->must_query[PAMI_XFER_ALLREDUCE][1];
        /* TODO this really needs to be cleaned up for BGQ and fca  */
        if(queryreq0 == MPID_COLL_NOQUERY &&
           count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
        {
          TRACE_ERR("Optimzed selection line %d\n",__LINE__);
          my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
          alg_selected = 1;
        }
        else if(queryreq1 == MPID_COLL_NOQUERY &&
                count > mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
        {
          TRACE_ERR("Optimzed selection line %d\n",__LINE__);
          my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][1];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
          alg_selected = 1;
        }
        else if(((queryreq0 == MPID_COLL_CHECK_FN_REQUIRED) ||
                 (queryreq0 == MPID_COLL_QUERY) ||
                 (queryreq0 ==  MPID_COLL_ALWAYS_QUERY)) &&
                ((mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] == 0) || 
                 (count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] && mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] > 0)))
        {
          TRACE_ERR("Optimzed selection line %d\n",__LINE__);
          my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
          alg_selected = 1;
          queryreq = queryreq0;
        }
        else if((queryreq1 == MPID_COLL_CHECK_FN_REQUIRED) ||
                (queryreq1 == MPID_COLL_QUERY) ||
                (queryreq1 ==  MPID_COLL_ALWAYS_QUERY))
        {  
          TRACE_ERR("Optimzed selection line %d\n",__LINE__);
          my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][1];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
          alg_selected = 1;
          queryreq = queryreq1;
        }
        TRACE_ERR("Alg selected: %d\n", alg_selected);
        if(likely(alg_selected))
        {
          if(unlikely((queryreq == MPID_COLL_CHECK_FN_REQUIRED) ||
                      (queryreq == MPID_COLL_QUERY) ||
                      (queryreq == MPID_COLL_ALWAYS_QUERY)))
          {
            TRACE_ERR("Optimzed selection line %d\n",__LINE__);
            if(my_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
            {
              metadata_result_t result = {0};
              TRACE_ERR("querying allreduce algorithm %s\n",
                        my_md->name);
              result = my_md->check_fn(&allred);
              TRACE_ERR("bitmask: %#X\n", result.bitmask);
              /* \todo Ignore check_correct.values.nonlocal until we implement the
                'pre-allreduce allreduce' or the 'safe' environment flag.
                We will basically assume 'safe' -- that all ranks are aligned (or not).
              */
              result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
              if(!result.bitmask)
                ; /* ok, algorithm selected */
              else
              {
                alg_selected = 0;
                if(unlikely(verbose))
                  fprintf(stderr,"check_fn failed for %s.\n", my_md->name);
              }
            } 
            else /* no check_fn, manually look at the metadata fields */
            {
              TRACE_ERR("Optimzed selection line %d\n",__LINE__);
              /* Check if the message range if restricted */
              if(my_md->check_correct.values.rangeminmax)
              {
                int data_size;
                MPIDI_Datatype_get_data_size(count, dt, data_size); 
                if((my_md->range_lo <= data_size) &&
                   (my_md->range_hi >= data_size))
                  ; /* ok, algorithm selected */
                else
                {
                  if(unlikely(verbose))
                    fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                            data_size,
                            my_md->range_lo,
                            my_md->range_hi,
                            my_md->name);
                  alg_selected = 0;
                }
              }
              /* \todo check the rest of the metadata */
            }
          }
          else
          {
            TRACE_ERR("Using %s for allreduce\n", my_md->name);
          }
        }
      }
    }
    else
    {
      TRACE_ERR("Non-Optimzed selection line %d\n",__LINE__);
      my_allred = mpid->user_selected[PAMI_XFER_ALLREDUCE];
      my_md = &mpid->user_metadata[PAMI_XFER_ALLREDUCE];
      if(selected_type == MPID_COLL_QUERY ||
         selected_type == MPID_COLL_ALWAYS_QUERY ||
         selected_type == MPID_COLL_CHECK_FN_REQUIRED)
      {
        TRACE_ERR("Non-Optimzed selection line %d\n",__LINE__);
        if(my_md->check_fn != NULL)
        {
          /* For now, we don't distinguish between MPID_COLL_ALWAYS_QUERY &
             MPID_COLL_CHECK_FN_REQUIRED, we just call the fn                */
          metadata_result_t result = {0};
          TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
                    my_md->name,
                    selected_type);
          result = mpid->user_metadata[PAMI_XFER_ALLREDUCE].check_fn(&allred);
          TRACE_ERR("bitmask: %#X\n", result.bitmask);
          /* \todo Ignore check_correct.values.nonlocal until we implement the
             'pre-allreduce allreduce' or the 'safe' environment flag.
             We will basically assume 'safe' -- that all ranks are aligned (or not).
          */
          result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
          if(!result.bitmask)
            alg_selected = 1; /* query algorithm successfully selected */
          else
            if(unlikely(verbose))
              fprintf(stderr,"check_fn failed for %s.\n", my_md->name);
        }
        else /* no check_fn, manually look at the metadata fields */
        {
          TRACE_ERR("Non-Optimzed selection line %d\n",__LINE__);
          /* Check if the message range if restricted */
          if(my_md->check_correct.values.rangeminmax)
          {
            int data_size;
            MPIDI_Datatype_get_data_size(count, dt, data_size); 
            if((my_md->range_lo <= data_size) &&
               (my_md->range_hi >= data_size))
              alg_selected = 1; /* query algorithm successfully selected */
            else
              if(unlikely(verbose))
                fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                      data_size,
                      my_md->range_lo,
                      my_md->range_hi,
                      my_md->name);
          }
          /* \todo check the rest of the metadata */
        }
      }
      else alg_selected = 1; /* non-query algorithm selected */
  
    }
  }

  if(unlikely(!alg_selected)) /* must be fallback to MPICH */
  {
    if(unlikely(verbose))
      fprintf(stderr,"Using MPICH allreduce\n");
    MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
    return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
  }
  allred.algorithm = my_allred;

  if(unlikely(verbose))
  {
    unsigned long long int threadID;
    MPIU_Thread_id_t tid;
    MPIU_Thread_self(&tid);
    threadID = (unsigned long long int)tid;
    fprintf(stderr,"<%llx> Using protocol %s for allreduce on %u\n", 
            threadID,
            my_md->name,
            (unsigned) comm_ptr->context_id);
  }

  MPIDI_Post_coll_t allred_post;
  MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                     MPIDI_Pami_post_wrapper, (void *)&allred);

  MPID_assert(rc == PAMI_SUCCESS);
  MPIDI_Update_last_algorithm(comm_ptr,my_md->name);
  MPID_PROGRESS_WAIT_WHILE(active);
  TRACE_ERR("allreduce done\n");
  return MPI_SUCCESS;
}

int MPIDO_Allreduce_simple(const void *sendbuf,
                    void *recvbuf,
                    int count,
                    MPI_Datatype dt,
                    MPI_Op op,
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
   void *sbuf;
   TRACE_ERR("Entering MPIDO_Allreduce_optimized\n");
   pami_type_t pdt;
   pami_data_function pop;
   int mu;
   int rc;
#ifdef TRACE_ON
    int len; 
    char op_str[255]; 
    char dt_str[255]; 
    MPIDI_Op_to_string(op, op_str); 
    PMPI_Type_get_name(dt, dt_str, &len); 
#endif
   volatile unsigned active = 1;
   pami_xfer_t allred;
   const pami_metadata_t *my_allred_md = (pami_metadata_t *)NULL;
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   MPID_Datatype *data_ptr;
   MPI_Aint data_true_lb = 0;
   int data_size, data_contig;

   MPIDI_Datatype_get_info(1, dt,
                           data_contig, data_size, data_ptr, data_true_lb);


   if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
   {
     advisor_algorithm_t advisor_algorithms[1];
     int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_ALLREDUCE, data_size * count, advisor_algorithms, 1);
     if(num_algorithms)
     {
       if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
       {
         return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
       }
     }
   }


   rc = MPIDI_Datatype_to_pami(dt, &pdt, op, &pop, &mu);

      /* convert to metadata query */
  /* Punt count 0 allreduce to MPICH. Let them do whatever's 'right' */
  if(unlikely(rc != MPI_SUCCESS || (count==0)))
   {
      MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

   if(!data_contig)
   {
      MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

  sbuf = (void *)sendbuf;
  if(unlikely(sendbuf == MPI_IN_PLACE))
  {
     sbuf = PAMI_IN_PLACE;
  }

  allred.cb_done = cb_allreduce;
  allred.cookie = (void *)&active;
  allred.cmd.xfer_allreduce.sndbuf = sbuf;
  allred.cmd.xfer_allreduce.stype = pdt;
  allred.cmd.xfer_allreduce.rcvbuf = recvbuf;
  allred.cmd.xfer_allreduce.rtype = pdt;
  allred.cmd.xfer_allreduce.stypecount = count;
  allred.cmd.xfer_allreduce.rtypecount = count;
  allred.cmd.xfer_allreduce.op = pop;
  allred.algorithm = mpid->coll_algorithm[PAMI_XFER_ALLREDUCE][0][0];
  my_allred_md = &mpid->coll_metadata[PAMI_XFER_ALLREDUCE][0][0];

  MPIDI_Post_coll_t allred_post;
  MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&allred);

  MPID_assert(rc == PAMI_SUCCESS);
  MPIDI_Update_last_algorithm(comm_ptr,my_allred_md->name);
  MPID_PROGRESS_WAIT_WHILE(active);
  TRACE_ERR("Allreduce done\n");
  return MPI_SUCCESS;
}

int
MPIDO_CSWrapper_allreduce(pami_xfer_t *allreduce,
                          void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype type;
   MPI_Op op;
   void *sbuf;
   MPIDI_coll_check_in_place(allreduce->cmd.xfer_allreduce.sndbuf, &sbuf);
   int rc = MPIDI_Dtpami_to_dtmpi(  allreduce->cmd.xfer_allreduce.stype,
                                   &type,
                                    allreduce->cmd.xfer_allreduce.op,
                                   &op);
   if(rc == -1) return rc;

   rc  =  MPIR_Allreduce(sbuf,
                         allreduce->cmd.xfer_allreduce.rcvbuf,
                         allreduce->cmd.xfer_allreduce.rtypecount,
                         type, op, comm_ptr, &mpierrno);
   if(allreduce->cb_done && rc == 0)
     allreduce->cb_done(NULL, allreduce->cookie, PAMI_SUCCESS);
   return rc;

}

