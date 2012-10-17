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
   pami_algorithm_t my_allred;
   const pami_metadata_t *my_allred_md = (pami_metadata_t *)NULL;
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
   if(likely(dt == MPI_DOUBLE || dt == MPI_DOUBLE_PRECISION))
   {
      rc = MPI_SUCCESS;
      pdt = PAMI_TYPE_DOUBLE;
      if(likely(op == MPI_SUM))
         pop = PAMI_DATA_SUM; 
      else if(likely(op == MPI_MAX))
         pop = PAMI_DATA_MAX; 
      else if(likely(op == MPI_MIN))
         pop = PAMI_DATA_MIN; 
      else rc = MPIDI_Datatype_to_pami(dt, &pdt, op, &pop, &mu);
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
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

  sbuf = (void *)sendbuf;
  if(unlikely(sendbuf == MPI_IN_PLACE))
   {
     if(unlikely(verbose))
         fprintf(stderr,"allreduce MPI_IN_PLACE buffering\n");
      sbuf = recvbuf;
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
   if(likely(selected_type == MPID_COLL_OPTIMIZED))
   {
     if(likely(pop == PAMI_DATA_SUM || pop == PAMI_DATA_MAX || pop == PAMI_DATA_MIN))
      {
         /* double protocol works on all message sizes */
         if(likely(pdt == PAMI_TYPE_DOUBLE && mpid->query_allred_dsmm == MPID_COLL_QUERY))
         {
            my_allred = mpid->cached_allred_dsmm;
            my_allred_md = &mpid->cached_allred_dsmm_md;
            alg_selected = 1;
         }
         else if(pdt == PAMI_TYPE_UNSIGNED_INT && mpid->query_allred_ismm == MPID_COLL_QUERY)
         {
            my_allred = mpid->cached_allred_ismm;
            my_allred_md = &mpid->cached_allred_ismm_md;
            alg_selected = 1;
         }
         /* The integer protocol at >1 ppn requires small messages only */
         else if(pdt == PAMI_TYPE_UNSIGNED_INT && mpid->query_allred_ismm == MPID_COLL_CHECK_FN_REQUIRED &&
                 count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = mpid->cached_allred_ismm;
            my_allred_md = &mpid->cached_allred_ismm_md;
            alg_selected = 1;
         }
         else if(mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_NOQUERY &&
                 count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
         else if(mpid->must_query[PAMI_XFER_ALLREDUCE][1] == MPID_COLL_NOQUERY &&
                 count > mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][1];
            my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
            alg_selected = 1;
         }
         else if((mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED) ||
		 (mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
		 (mpid->must_query[PAMI_XFER_ALLREDUCE][0] ==  MPID_COLL_ALWAYS_QUERY))
         {
            if((mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] == 0) || 
			(count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] && mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] > 0))
            {
              my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
              my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
              alg_selected = 1;
            }
         }
      }
      else
      {
         /* so we aren't one of the key ops... */
         if(mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_NOQUERY &&
            count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
         else if(mpid->must_query[PAMI_XFER_ALLREDUCE][1] == MPID_COLL_NOQUERY &&
                 count > mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][1];
            my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
            alg_selected = 1;
         }
         else if((mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED) ||
		 (mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
		 (mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_ALWAYS_QUERY))
         {
            if((mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] == 0) || 
               (count <= mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] && mpid->cutoff_size[PAMI_XFER_ALLREDUCE][0] > 0))
            {			
              my_allred = mpid->opt_protocol[PAMI_XFER_ALLREDUCE][0];
              my_allred_md = &mpid->opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
              alg_selected = 1;
            }
         }
      }
      TRACE_ERR("Alg selected: %d\n", alg_selected);
      if(likely(alg_selected))
      {
	if(unlikely(mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED))
        {
           if(my_allred_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
           {
              metadata_result_t result = {0};
              TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
                 my_allred_md->name,
                 mpid->must_query[PAMI_XFER_ALLREDUCE]);
              result = my_allred_md->check_fn(&allred);
              TRACE_ERR("bitmask: %#X\n", result.bitmask);
              /* \todo Ignore check_correct.values.nonlocal until we implement the
                 'pre-allreduce allreduce' or the 'safe' environment flag.
                 We will basically assume 'safe' -- that all ranks are aligned (or not).
              */
              result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
              if(!result.bitmask)
              {
                 allred.algorithm = my_allred;
              }
              else
              {
                 alg_selected = 0;
                 if(unlikely(verbose))
                    fprintf(stderr,"check_fn failed for %s.\n", my_allred_md->name);
              }
           }
         else alg_selected = 0;
	}
	else if(unlikely(((mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
			  (mpid->must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_ALWAYS_QUERY))))
        {
           if(my_allred_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
           {
              metadata_result_t result = {0};
              TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
                 my_allred_md->name,
                 mpid->must_query[PAMI_XFER_ALLREDUCE]);
              result = my_allred_md->check_fn(&allred);
              TRACE_ERR("bitmask: %#X\n", result.bitmask);
              /* \todo Ignore check_correct.values.nonlocal until we implement the
                 'pre-allreduce allreduce' or the 'safe' environment flag.
                 We will basically assume 'safe' -- that all ranks are aligned (or not).
              */
              result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
              if(!result.bitmask)
              {
                 allred.algorithm = my_allred;
              }
              else
              {
                 alg_selected = 0;
                 if(unlikely(verbose))
                    fprintf(stderr,"check_fn failed for %s.\n", my_allred_md->name);
              }
           }
	   else /* no check_fn, manually look at the metadata fields */
	   {
	     /* Check if the message range if restricted */
	     if(my_allred_md->check_correct.values.rangeminmax)
	     {
               MPI_Aint data_true_lb;
               MPID_Datatype *data_ptr;
               int data_size, data_contig;
               MPIDI_Datatype_get_info(count, dt, data_contig, data_size, data_ptr, data_true_lb); 
               if((my_allred_md->range_lo <= data_size) &&
                  (my_allred_md->range_hi >= data_size))
                 allred.algorithm = my_allred; /* query algorithm successfully selected */
               else
		 {
		   if(unlikely(verbose))
                     fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                             data_size,
                             my_allred_md->range_lo,
                             my_allred_md->range_hi,
                             my_allred_md->name);
		   alg_selected = 0;
		 }
	     }
	     /* \todo check the rest of the metadata */
	   }
        }
        else
        {
           TRACE_ERR("Using %s for allreduce\n", my_allred_md->name);
           allred.algorithm = my_allred;
        }
      }
   }
   else
   {
      my_allred = mpid->user_selected[PAMI_XFER_ALLREDUCE];
      my_allred_md = &mpid->user_metadata[PAMI_XFER_ALLREDUCE];
      allred.algorithm = my_allred;
      if(selected_type == MPID_COLL_QUERY ||
         selected_type == MPID_COLL_ALWAYS_QUERY ||
         selected_type == MPID_COLL_CHECK_FN_REQUIRED)
      {
         if(my_allred_md->check_fn != NULL)
         {
            /* For now, we don't distinguish between MPID_COLL_ALWAYS_QUERY &
               MPID_COLL_CHECK_FN_REQUIRED, we just call the fn                */
            metadata_result_t result = {0};
            TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
               my_allred_md->name,
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
                  fprintf(stderr,"check_fn failed for %s.\n", my_allred_md->name);
         }
         else /* no check_fn, manually look at the metadata fields */
         {
            /* Check if the message range if restricted */
            if(my_allred_md->check_correct.values.rangeminmax)
            {
               MPI_Aint data_true_lb;
               MPID_Datatype *data_ptr;
               int data_size, data_contig;
               MPIDI_Datatype_get_info(count, dt, data_contig, data_size, data_ptr, data_true_lb); 
               if((my_allred_md->range_lo <= data_size) &&
                  (my_allred_md->range_hi >= data_size))
                  alg_selected = 1; /* query algorithm successfully selected */
               else
                 if(unlikely(verbose))
                     fprintf(stderr,"message size (%u) outside range (%zu<->%zu) for %s.\n",
                             data_size,
                             my_allred_md->range_lo,
                             my_allred_md->range_hi,
                             my_allred_md->name);
            }
            /* \todo check the rest of the metadata */
         }
      }
      else alg_selected = 1; /* non-query algorithm selected */

   }

   if(unlikely(!alg_selected)) /* must be fallback to MPICH */
   {
     if(unlikely(verbose))
         fprintf(stderr,"Using MPICH allreduce\n");
      MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

   if(unlikely(verbose))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for allreduce on %u\n", 
              threadID,
              my_allred_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Post_coll_t allred_post;
   MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&allred);

   MPID_assert(rc == PAMI_SUCCESS);
   MPIDI_Update_last_algorithm(comm_ptr,my_allred_md->name);
   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("allreduce done\n");
   return MPI_SUCCESS;
}

