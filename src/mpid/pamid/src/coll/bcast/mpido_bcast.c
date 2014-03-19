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
 * \file src/coll/bcast/mpido_bcast.c
 * \brief ???
 */

/* #define TRACE_ON */

#include <mpidimpl.h>

static void cb_bcast(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *) clientdata;
   TRACE_ERR("callback enter, active: %d\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}

int MPIDO_Bcast(void *buffer,
                int count,
                MPI_Datatype datatype,
                int root,
                MPID_Comm *comm_ptr,
                int *mpierrno)
{
   TRACE_ERR("in mpido_bcast\n");
   const size_t BCAST_LIMIT =      0x40000000;
   int data_contig, rc;
   void *data_buffer    = NULL,
        *noncontig_buff = NULL;
   volatile unsigned active = 1;
   MPI_Aint data_true_lb = 0;
   MPID_Datatype *data_ptr;
   MPID_Segment segment;
   MPIDI_Post_coll_t bcast_post;
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
   const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif
   const int selected_type = mpid->user_selected_type[PAMI_XFER_BROADCAST];

   /* Must calculate data_size based on count=1 in case it's total size is > integer */
   int data_size_one;
   MPIDI_Datatype_get_info(1, datatype,
			   data_contig, data_size_one, data_ptr, data_true_lb);
   /* do this calculation once and use twice */
   const size_t data_size_sz = (size_t)data_size_one*(size_t)count;
   if(unlikely(verbose))
     fprintf(stderr,"bcast count %d, size %d (%#zX), root %d, buffer %p\n",
	     count,data_size_one, (size_t)data_size_one*(size_t)count, root,buffer);
   if(unlikely( data_size_sz > BCAST_LIMIT) )
   {
      void *new_buffer=buffer;
      int c, new_count = (int)BCAST_LIMIT/data_size_one;
      MPID_assert(new_count > 0);

      for(c=1; ((size_t)c*(size_t)new_count) <= (size_t)count; ++c)
      {
        if ((rc = MPIDO_Bcast(new_buffer,
                        new_count,
                        datatype,
                        root,
                        comm_ptr,
                              mpierrno)) != MPI_SUCCESS)
         return rc;
	 new_buffer = (char*)new_buffer + (size_t)data_size_one*(size_t)new_count;
      }
      new_count = count % new_count; /* 0 is ok, just returns no-op */
      return MPIDO_Bcast(new_buffer,
                         new_count,
                         datatype,
                         root,
                         comm_ptr,
                         mpierrno);
   }

   /* Must use data_size based on count for byte bcast processing.
      Previously calculated as a size_t but large data_sizes were 
      handled above so this cast to int should be fine here.  
   */
   const int data_size = (int)data_size_sz;

   if(selected_type == MPID_COLL_USE_MPICH || data_size == 0)
   {
     if(unlikely(verbose))
       fprintf(stderr,"Using MPICH bcast algorithm\n");
      MPIDI_Update_last_algorithm(comm_ptr,"BCAST_MPICH");
      return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
   }

   data_buffer = (char *)buffer + data_true_lb;

   if(!data_contig)
   {
      noncontig_buff = MPIU_Malloc(data_size);
      data_buffer = noncontig_buff;
      if(noncontig_buff == NULL)
      {
         MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
            "Fatal:  Cannot allocate pack buffer");
      }
      if(rank == root)
      {
         DLOOP_Offset last = data_size;
         MPID_Segment_init(buffer, count, datatype, &segment, 0);
         MPID_Segment_pack(&segment, 0, &last, noncontig_buff);
      }
   }

   pami_xfer_t bcast;
   pami_algorithm_t my_bcast;
   const pami_metadata_t *my_md = (pami_metadata_t *)NULL;
   int queryreq = 0;

   bcast.cb_done = cb_bcast;
   bcast.cookie = (void *)&active;
   bcast.cmd.xfer_broadcast.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
   bcast.algorithm = mpid->user_selected[PAMI_XFER_BROADCAST];
   bcast.cmd.xfer_broadcast.buf = data_buffer;
   bcast.cmd.xfer_broadcast.type = PAMI_TYPE_BYTE;
   /* Needs to be sizeof(type)*count since we are using bytes as * the generic type */
   bcast.cmd.xfer_broadcast.typecount = data_size;

   if(selected_type == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized bcast (%s) and (%s) were pre-selected\n",
         mpid->opt_protocol_md[PAMI_XFER_BROADCAST][0].name,
         mpid->opt_protocol_md[PAMI_XFER_BROADCAST][1].name);

      if(mpid->cutoff_size[PAMI_XFER_BROADCAST][1] != 0)/* SSS: There is FCA cutoff (FCA only sets cutoff for [PAMI_XFER_BROADCAST][1]) */
      {
        if(data_size <= mpid->cutoff_size[PAMI_XFER_BROADCAST][1])
        {
          my_bcast = mpid->opt_protocol[PAMI_XFER_BROADCAST][1];
          my_md = &mpid->opt_protocol_md[PAMI_XFER_BROADCAST][1];
          queryreq = mpid->must_query[PAMI_XFER_BROADCAST][1];
        }
        else
        {
          return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
        }
      }

      if(data_size > mpid->cutoff_size[PAMI_XFER_BROADCAST][0])
      {
         my_bcast = mpid->opt_protocol[PAMI_XFER_BROADCAST][1];
         my_md = &mpid->opt_protocol_md[PAMI_XFER_BROADCAST][1];
         queryreq = mpid->must_query[PAMI_XFER_BROADCAST][1];
      }
      else
      {
         my_bcast = mpid->opt_protocol[PAMI_XFER_BROADCAST][0];
         my_md = &mpid->opt_protocol_md[PAMI_XFER_BROADCAST][0];
         queryreq = mpid->must_query[PAMI_XFER_BROADCAST][0];
      }
   }
   else
   {
      TRACE_ERR("Bcast (%s) was specified by user\n",
         mpid->user_metadata[PAMI_XFER_BROADCAST].name);
      my_bcast =  mpid->user_selected[PAMI_XFER_BROADCAST];
      my_md = &mpid->user_metadata[PAMI_XFER_BROADCAST];
      queryreq = selected_type;
   }

   bcast.algorithm = my_bcast;

   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY ||
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying bcast protocol %s, type was: %d\n",
                my_md->name, queryreq);
      if(my_md->check_fn != NULL) /* calling the check fn is sufficient */
      {
         result = my_md->check_fn(&bcast);
         result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
      } 
      else /* no check_fn, manually look at the metadata fields */
      {
         TRACE_ERR("Optimzed selection line %d\n",__LINE__);
         /* Check if the message range if restricted */
         if(my_md->check_correct.values.rangeminmax)
         {
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
         /* \todo check the rest of the metadata */
      }
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(result.bitmask)
      {
         if(unlikely(verbose))
            fprintf(stderr,"Using MPICH bcast algorithm - query fn failed\n");
         MPIDI_Update_last_algorithm(comm_ptr,"BCAST_MPICH");
         return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
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
      fprintf(stderr,"<%llx> Using protocol %s for bcast on %u\n", 
              threadID,
              my_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Context_post(MPIDI_Context[0], &bcast_post.state, MPIDI_Pami_post_wrapper, (void *)&bcast);
   MPIDI_Update_last_algorithm(comm_ptr, my_md->name);
   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("bcast done\n");

   if(!data_contig)
   {
      if(rank != root)
         MPIR_Localcopy(noncontig_buff, data_size, MPI_CHAR,
                        buffer,         count,     datatype);
      MPIU_Free(noncontig_buff);
   }

   TRACE_ERR("leaving bcast\n");
   return 0;
}


int MPIDO_Bcast_simple(void *buffer,
                int count,
                MPI_Datatype datatype,
                int root,
                MPID_Comm *comm_ptr,
                int *mpierrno)
{
   TRACE_ERR("Entering MPIDO_Bcast_optimized\n");

   int data_contig;
   void *data_buffer    = NULL,
        *noncontig_buff = NULL;
   volatile unsigned active = 1;
   MPI_Aint data_true_lb = 0;
   MPID_Datatype *data_ptr;
   MPID_Segment segment;
   MPIDI_Post_coll_t bcast_post;
   const struct MPIDI_Comm* const mpid = &(comm_ptr->mpid);
   const int rank = comm_ptr->rank;
#if ASSERT_LEVEL==0
   /* We can't afford the tracing in ndebug/performance libraries */
    const unsigned verbose = 0;
#else
    const unsigned verbose = (MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL) && (rank == 0);
#endif

   /* Must calculate data_size based on count=1 in case it's total size is > integer */
   int data_size_one;
   MPIDI_Datatype_get_info(1, datatype,
			   data_contig, data_size_one, data_ptr, data_true_lb);
   if(MPIDI_Pamix_collsel_advise != NULL && mpid->collsel_fast_query != NULL)
   {
     advisor_algorithm_t advisor_algorithms[1];
     int num_algorithms = MPIDI_Pamix_collsel_advise(mpid->collsel_fast_query, PAMI_XFER_BROADCAST, data_size_one * count, advisor_algorithms, 1);
     if(num_algorithms)
     {
       if(advisor_algorithms[0].algorithm_type == COLLSEL_EXTERNAL_ALGO)
       {
         return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
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

   const int data_size = data_size_one*(size_t)count;

   data_buffer = (char *)buffer + data_true_lb;

   if(!data_contig)
   {
      noncontig_buff = MPIU_Malloc(data_size);
      data_buffer = noncontig_buff;
      if(noncontig_buff == NULL)
      {
         MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
            "Fatal:  Cannot allocate pack buffer");
      }
      if(rank == root)
      {
         DLOOP_Offset last = data_size;
         MPID_Segment_init(buffer, count, datatype, &segment, 0);
         MPID_Segment_pack(&segment, 0, &last, noncontig_buff);
      }
   }

   pami_xfer_t bcast;
   const pami_metadata_t *my_bcast_md;

   bcast.cb_done = cb_bcast;
   bcast.cookie = (void *)&active;
   bcast.cmd.xfer_broadcast.root = MPIDI_Task_to_endpoint(MPID_VCR_GET_LPID(comm_ptr->vcr, root), 0);
   bcast.algorithm = mpid->coll_algorithm[PAMI_XFER_BROADCAST][0][0];
   bcast.cmd.xfer_broadcast.buf = data_buffer;
   bcast.cmd.xfer_broadcast.type = PAMI_TYPE_BYTE;
   /* Needs to be sizeof(type)*count since we are using bytes as * the generic type */
   bcast.cmd.xfer_broadcast.typecount = data_size;
   my_bcast_md = &mpid->coll_metadata[PAMI_XFER_BROADCAST][0][0];

   MPIDI_Context_post(MPIDI_Context[0], &bcast_post.state, MPIDI_Pami_post_wrapper, (void *)&bcast);
   MPIDI_Update_last_algorithm(comm_ptr, my_bcast_md->name);
   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("bcast done\n");

   if(!data_contig)
   {
      if(rank != root)
         MPIR_Localcopy(noncontig_buff, data_size, MPI_CHAR,
                        buffer,         count,     datatype);
      MPIU_Free(noncontig_buff);
   }

   TRACE_ERR("Exiting MPIDO_Bcast_optimized\n");
   return 0;
}


int
MPIDO_CSWrapper_bcast(pami_xfer_t *bcast,
                      void        *comm)
{
   int mpierrno = 0;
   MPID_Comm   *comm_ptr = (MPID_Comm*)comm;
   MPI_Datatype type;
   int rc = MPIDI_Dtpami_to_dtmpi(  bcast->cmd.xfer_broadcast.type,
                                   &type,
                                    NULL,
                                    NULL);
   if(rc == -1) return rc;

   rc  =  MPIR_Bcast_intra(bcast->cmd.xfer_broadcast.buf, 
                           bcast->cmd.xfer_broadcast.typecount, type, 
                           bcast->cmd.xfer_broadcast.root, comm_ptr, &mpierrno);
   if(bcast->cb_done && rc == 0)
     bcast->cb_done(NULL, bcast->cookie, PAMI_SUCCESS);
   return rc;
}

