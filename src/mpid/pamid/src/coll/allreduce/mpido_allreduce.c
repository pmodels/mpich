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

/*#define TRACE_ON*/

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
   pami_metadata_t *my_allred_md = (pami_metadata_t *)NULL;
   int alg_selected = 0;

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

  if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
      fprintf(stderr,"allred rc %u, Datatype %p, op %p, mu %u, selectedvar %u != %u\n",
              rc, pdt, pop, mu, 
              (unsigned)comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE],MPID_COLL_USE_MPICH);
      /* convert to metadata query */
  if(unlikely(rc != MPI_SUCCESS || 
	      comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_USE_MPICH))
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH allreduce type %u.\n",
                 comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE]);
      MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

  if(unlikely(sendbuf == MPI_IN_PLACE))
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
         fprintf(stderr,"allreduce MPI_IN_PLACE buffering\n");
      sbuf = recvbuf;
   }
   else sbuf = (void *)sendbuf;

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
   if(likely(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_OPTIMIZED))
   {
     if(likely(pop == PAMI_DATA_SUM || pop == PAMI_DATA_MAX || pop == PAMI_DATA_MIN))
      {
         /* double protocol works on all message sizes */
         if(likely(pdt == PAMI_TYPE_DOUBLE && comm_ptr->mpid.query_allred_dsmm == MPID_COLL_QUERY))
         {
            my_allred = comm_ptr->mpid.cached_allred_dsmm;
            my_allred_md = &comm_ptr->mpid.cached_allred_dsmm_md;
            alg_selected = 1;
         }
         else if(pdt == PAMI_TYPE_UNSIGNED_INT && comm_ptr->mpid.query_allred_ismm == MPID_COLL_QUERY)
         {
            my_allred = comm_ptr->mpid.cached_allred_ismm;
            my_allred_md = &comm_ptr->mpid.cached_allred_ismm_md;
            alg_selected = 1;
         }
         /* The integer protocol at >1 ppn requires small messages only */
         else if(pdt == PAMI_TYPE_UNSIGNED_INT && comm_ptr->mpid.query_allred_ismm == MPID_COLL_CHECK_FN_REQUIRED &&
                 count <= comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = comm_ptr->mpid.cached_allred_ismm;
            my_allred_md = &comm_ptr->mpid.cached_allred_ismm_md;
            alg_selected = 1;
         }
         else if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_NOQUERY &&
                 count <= comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
         else if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] == MPID_COLL_NOQUERY &&
                 count > comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][1];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
            alg_selected = 1;
         }
         else if((comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED) ||
		 (comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
		 (comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] ==  MPID_COLL_ALWAYS_QUERY))
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
      }
      else
      {
         /* so we aren't one of the key ops... */
         if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_NOQUERY &&
            count <= comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
         else if(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][1] == MPID_COLL_NOQUERY &&
                 count > comm_ptr->mpid.cutoff_size[PAMI_XFER_ALLREDUCE][0])
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][1];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][1];
            alg_selected = 1;
         }
         else if((comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED) ||
		 (comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
		 (comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_ALWAYS_QUERY))
         {
            my_allred = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLREDUCE][0];
            my_allred_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLREDUCE][0];
            alg_selected = 1;
         }
      }
      TRACE_ERR("Alg selected: %d\n", alg_selected);
      if(likely(alg_selected))
      {
	if(unlikely(comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_CHECK_FN_REQUIRED))
        {
           if(my_allred_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
           {
              metadata_result_t result = {0};
              TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
                 my_allred_md->name,
                 comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE]);
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
                 if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
                    fprintf(stderr,"check_fn failed for %s.\n", my_allred_md->name);
              }
           }
         else alg_selected = 0;
	}
	else if(unlikely(((comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_QUERY) ||
			  (comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE][0] == MPID_COLL_ALWAYS_QUERY))))
        {
           if(my_allred_md->check_fn != NULL)/*This should always be the case in FCA.. Otherwise punt to mpich*/
           {
              metadata_result_t result = {0};
              TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
                 my_allred_md->name,
                 comm_ptr->mpid.must_query[PAMI_XFER_ALLREDUCE]);
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
                 if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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
		   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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
      my_allred = comm_ptr->mpid.user_selected[PAMI_XFER_ALLREDUCE];
      my_allred_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_ALLREDUCE];
      allred.algorithm = my_allred;
      if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_QUERY ||
         comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_ALWAYS_QUERY ||
         comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE] == MPID_COLL_CHECK_FN_REQUIRED)
      {
         if(my_allred_md->check_fn != NULL)
         {
            /* For now, we don't distinguish between MPID_COLL_ALWAYS_QUERY &
               MPID_COLL_CHECK_FN_REQUIRED, we just call the fn                */
            metadata_result_t result = {0};
            TRACE_ERR("querying allreduce algorithm %s, type was %d\n",
               my_allred_md->name,
               comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLREDUCE]);
            result = comm_ptr->mpid.user_metadata[PAMI_XFER_ALLREDUCE].check_fn(&allred);
            TRACE_ERR("bitmask: %#X\n", result.bitmask);
            /* \todo Ignore check_correct.values.nonlocal until we implement the
               'pre-allreduce allreduce' or the 'safe' environment flag.
               We will basically assume 'safe' -- that all ranks are aligned (or not).
            */
            result.check.nonlocal = 0; /* #warning REMOVE THIS WHEN IMPLEMENTED */
            if(!result.bitmask)
               alg_selected = 1; /* query algorithm successfully selected */
            else 
               if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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
                 if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH allreduce\n");
      MPIDI_Update_last_algorithm(comm_ptr, "ALLREDUCE_MPICH");
      return MPIR_Allreduce(sendbuf, recvbuf, count, dt, op, comm_ptr, mpierrno);
   }

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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

   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("allreduce done\n");
   return MPI_SUCCESS;
}

