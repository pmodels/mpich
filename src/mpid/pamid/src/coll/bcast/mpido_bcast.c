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

/*#define TRACE_ON*/

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
   int data_size, data_contig;
   void *data_buffer    = NULL,
        *noncontig_buff = NULL;
   volatile unsigned active = 1;
   MPI_Aint data_true_lb = 0;
   MPID_Datatype *data_ptr;
   MPID_Segment segment;
   MPIDI_Post_coll_t bcast_post;
/*   MPIDI_Post_coll_t allred_post; eventually for
   preallreduces*/
   if(count == 0)
   {
      MPIDI_Update_last_algorithm(comm_ptr,"BCAST_NONE");
      return MPI_SUCCESS;
   }
   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] == MPID_COLL_USE_MPICH)
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH bcast algorithm\n");
      return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(count, datatype,
               data_contig, data_size, data_ptr, data_true_lb);

   /* If the user has constructed some weird 0-length datatype but 
    * count is not 0, we'll let mpich handle it */
   if(unlikely( data_size == 0) )
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH bcast algorithm for data_size 0\n");
      return MPIR_Bcast_intra(buffer, count, datatype, root, comm_ptr, mpierrno);
   }
   data_buffer = (char *)buffer + data_true_lb;

   if(!data_contig)
   {
      if(comm_ptr->rank == root)
         TRACE_ERR("noncontig data\n");
      noncontig_buff = MPIU_Malloc(data_size);
      data_buffer = noncontig_buff;
      if(noncontig_buff == NULL)
      {
         fprintf(stderr,
            "Pack: Tree Bcast cannot allocate local non-contig pack buffer\n");
/*         MPIX_Dump_stacks();*/
         MPID_Abort(NULL, MPI_ERR_NO_SPACE, 1,
            "Fatal:  Cannot allocate pack buffer");
      }
      if(comm_ptr->rank == root)
      {
         DLOOP_Offset last = data_size;
         MPID_Segment_init(buffer, count, datatype, &segment, 0);
         MPID_Segment_pack(&segment, 0, &last, noncontig_buff);
      }
   }

   pami_xfer_t bcast;
   pami_algorithm_t my_bcast;
   pami_metadata_t *my_bcast_md;
   int queryreq = 0;

   bcast.cb_done = cb_bcast;
   bcast.cookie = (void *)&active;
   bcast.cmd.xfer_broadcast.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);
   bcast.algorithm = comm_ptr->mpid.user_selected[PAMI_XFER_BROADCAST];
   bcast.cmd.xfer_broadcast.buf = data_buffer;
   bcast.cmd.xfer_broadcast.type = PAMI_TYPE_BYTE;
   /* Needs to be sizeof(type)*count since we are using bytes as * the generic type */
   bcast.cmd.xfer_broadcast.typecount = data_size;

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized bcast (%s) and (%s) were pre-selected\n",
         comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0].name,
         comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][1].name);

      if(data_size > comm_ptr->mpid.cutoff_size[PAMI_XFER_BROADCAST][0])
      {
         my_bcast = comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][1];
         my_bcast_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][1];
         queryreq = comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][1];
      }
      else
      {
         my_bcast = comm_ptr->mpid.opt_protocol[PAMI_XFER_BROADCAST][0];
         my_bcast_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_BROADCAST][0];
         queryreq = comm_ptr->mpid.must_query[PAMI_XFER_BROADCAST][0];
      }
   }
   else
   {
      TRACE_ERR("Optimized bcast (%s) was specified by user\n",
         comm_ptr->mpid.user_metadata[PAMI_XFER_BROADCAST].name);
      my_bcast =  comm_ptr->mpid.user_selected[PAMI_XFER_BROADCAST];
      my_bcast_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_BROADCAST];
      queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_BROADCAST];
   }

   bcast.algorithm = my_bcast;

   if(queryreq == MPID_COLL_ALWAYS_QUERY || queryreq == MPID_COLL_CHECK_FN_REQUIRED)
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying bcast protocol %s, type was: %d\n",
         my_bcast_md->name, queryreq);
      result = my_bcast_md->check_fn(&bcast);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(!result.bitmask)
      {
         fprintf(stderr,"query failed for %s.\n", my_bcast_md->name);
      }
   }


   TRACE_ERR("%s bcast, context: %d, algoname: %s\n",
             MPIDI_Process.context_post.active>0?"posting":"invoking", 0, my_bcast_md->name);
   MPIDI_Update_last_algorithm(comm_ptr, my_bcast_md->name);

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for bcast on %u\n", 
              threadID,
              my_bcast_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Context_post(MPIDI_Context[0], &bcast_post.state, MPIDI_Pami_post_wrapper, (void *)&bcast);

   TRACE_ERR("bcast %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");

   MPID_PROGRESS_WAIT_WHILE(active);
   TRACE_ERR("bcast done\n");

   if(!data_contig)
   {
      TRACE_ERR("cleaning up noncontig\n");
      if(comm_ptr->rank != root)
         MPIR_Localcopy(noncontig_buff, data_size, MPI_CHAR,
                        buffer,         count,     datatype);
      MPIU_Free(noncontig_buff);
   }

   TRACE_ERR("leaving bcast\n");
   return 0;
}
