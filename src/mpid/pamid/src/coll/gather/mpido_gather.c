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
 * \file src/coll/gather/mpido_gather.c
 * \brief ???
 */

#include <mpidimpl.h>

static void cb_gather(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_gather enter, active: %u\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}

static void cb_allred(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_allred preallred enter, active: %u\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}
int MPIDO_Gather_reduce(void * sendbuf,
			int sendcount,
			MPI_Datatype sendtype,
			void *recvbuf,
			int recvcount,
			MPI_Datatype recvtype,
			int root,
			MPID_Comm * comm_ptr,
			int *mpierrno)
{
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb;
  int rank = comm_ptr->rank;
  int size = comm_ptr->local_size;
  int rc, sbytes, rbytes, contig;
  char *tempbuf = NULL;
  char *inplacetemp = NULL;

  MPIDI_Datatype_get_info(sendcount, sendtype, contig,
			  sbytes, data_ptr, true_lb);

  MPIDI_Datatype_get_info(recvcount, recvtype, contig,
			  rbytes, data_ptr, true_lb);

  if(rank == root)
  {
    tempbuf = recvbuf;
    /* zero the buffer if we aren't in place */
    if(sendbuf != MPI_IN_PLACE)
    {
      memset(tempbuf, 0, sbytes * size * sizeof(char));
      memcpy(tempbuf+(rank * sbytes), sendbuf, sbytes);
    }
    /* copy our contribution temporarily, zero the buffer, put it back */
    /* this will likely suck */
    else
    {
      inplacetemp = MPIU_Malloc(rbytes * sizeof(char));
      memcpy(inplacetemp, recvbuf+(rank * rbytes), rbytes);
      memset(tempbuf, 0, sbytes * size * sizeof(char));
      memcpy(tempbuf+(rank * rbytes), inplacetemp, rbytes);
      MPIU_Free(inplacetemp);
    }
  }
  /* everyone might need to speciifcally allocate a tempbuf, or
   * we might need to make sure we don't end up at mpich in the
   * mpido_() functions - seems to be a problem?
   */

  /* If we aren't root, malloc tempbuf and zero it,
   * then copy our contribution to the right spot in the big buffer */
  else
  {
    tempbuf = MPIU_Malloc(sbytes * size * sizeof(char));
    if(!tempbuf)
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  __FUNCTION__,
                                  __LINE__,
                                  MPI_ERR_OTHER,
                                  "**nomem", 0);

    memset(tempbuf, 0, sbytes * size * sizeof(char));
    memcpy(tempbuf+(rank*sbytes), sendbuf, sbytes);
  }
  /* #warning TODO need an optimal reduce */
  rc = MPIR_Reduce(MPI_IN_PLACE,
                    tempbuf,
                    (sbytes * size)/4,
                    MPI_INT,
                    MPI_BOR,
                    root,
                    comm_ptr,
                    mpierrno);

  if(rank != root)
    MPIU_Free(tempbuf);

  return rc;
}

/* works for simple data types, assumes fast reduce is available */

int MPIDO_Gather(const void *sendbuf,
                 int sendcount,
                 MPI_Datatype sendtype,
                 void *recvbuf,
                 int recvcount,
                 MPI_Datatype recvtype,
                 int root,
                 MPID_Comm *comm_ptr,
		 int *mpierrno)
{
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb = 0;
  pami_xfer_t gather;
  MPIDI_Post_coll_t gather_post;
/*  char *sbuf = sendbuf, *rbuf = recvbuf;*/
  int success = 1, contig, send_bytes=-1, recv_bytes = 0;
  int rank = comm_ptr->rank;

  if ((sendbuf == MPI_IN_PLACE) && sendtype != MPI_DATATYPE_NULL && sendcount >= 0)
  {
    MPIDI_Datatype_get_info(sendcount, sendtype, contig,
                            send_bytes, data_ptr, true_lb);
    if (!contig || ((send_bytes * comm_ptr->local_size) % sizeof(int)))
      success = 0;
  }
  else
    success = 0;

  if (success && rank == root)
  {
    if (recvtype != MPI_DATATYPE_NULL && recvcount >= 0)
    {
      MPIDI_Datatype_get_info(recvcount, recvtype, contig,
                              recv_bytes, data_ptr, true_lb);
      if (!contig) success = 0;
    }
    else
      success = 0;
  }

  MPIDI_Update_last_algorithm(comm_ptr, "GATHER_MPICH");
  if(!comm_ptr->mpid.optgather ||
   comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] == MPID_COLL_USE_MPICH)
  {
    if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
      fprintf(stderr,"Using MPICH gather algorithm\n");
    return MPIR_Gather(sendbuf, sendcount, sendtype,
                       recvbuf, recvcount, recvtype,
                       root, comm_ptr, mpierrno);
  }

   if(comm_ptr->mpid.preallreduces[MPID_GATHER_PREALLREDUCE])
   {
      volatile unsigned allred_active = 1;
      pami_xfer_t allred;
      MPIDI_Post_coll_t allred_post;
      allred.cb_done = cb_allred;
      allred.cookie = (void *)&allred_active;
      /* Guaranteed to work allreduce */
      allred.algorithm = comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][0][0];
      allred.cmd.xfer_allreduce.sndbuf = (void *)(size_t)success;
      allred.cmd.xfer_allreduce.stype = PAMI_TYPE_SIGNED_INT;
      allred.cmd.xfer_allreduce.rcvbuf = (void *)(size_t)success;
      allred.cmd.xfer_allreduce.rtype = PAMI_TYPE_SIGNED_INT;
      allred.cmd.xfer_allreduce.stypecount = 1;
      allred.cmd.xfer_allreduce.rtypecount = 1;
      allred.cmd.xfer_allreduce.op = PAMI_DATA_BAND;

      MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&allred);
      MPID_PROGRESS_WAIT_WHILE(allred_active);
   }

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] == MPID_COLL_USE_MPICH || !success)
   {
    if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
      fprintf(stderr,"Using MPICH gather algorithm\n");
    return MPIR_Gather(sendbuf, sendcount, sendtype,
                       recvbuf, recvcount, recvtype,
                       root, comm_ptr, mpierrno);
   }


   pami_algorithm_t my_gather;
   pami_metadata_t *my_gather_md;
   int queryreq = 0;
   volatile unsigned active = 1;

   gather.cb_done = cb_gather;
   gather.cookie = (void *)&active;
   gather.cmd.xfer_gather.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);
   if(sendbuf == MPI_IN_PLACE) 
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
       fprintf(stderr,"gather MPI_IN_PLACE buffering\n");
     gather.cmd.xfer_gather.stypecount = recv_bytes;
     gather.cmd.xfer_gather.sndbuf = (char *)recvbuf + recv_bytes*comm_ptr->rank;
   }
   else
   {
     gather.cmd.xfer_gather.stypecount = send_bytes;
     gather.cmd.xfer_gather.sndbuf = (void *)sendbuf;
   }
   gather.cmd.xfer_gather.stype = PAMI_TYPE_BYTE;
   gather.cmd.xfer_gather.rcvbuf = (void *)recvbuf;
   gather.cmd.xfer_gather.rtype = PAMI_TYPE_BYTE;
   gather.cmd.xfer_gather.rtypecount = recv_bytes;

   /* If glue-level protocols are good, this will require some changes */
   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized gather (%s) was pre-selected\n",
         comm_ptr->mpid.opt_protocol_md[PAMI_XFER_GATHER][0].name);
      my_gather = comm_ptr->mpid.opt_protocol[PAMI_XFER_GATHER][0];
      my_gather_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_GATHER][0];
      queryreq = comm_ptr->mpid.must_query[PAMI_XFER_GATHER][0];
   }
   else
   {
      TRACE_ERR("Optimized gather (%s) was specified by user\n",
         comm_ptr->mpid.user_metadata[PAMI_XFER_GATHER].name);
      my_gather = comm_ptr->mpid.user_selected[PAMI_XFER_GATHER];
      my_gather_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_GATHER];
      queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHER];
   }

   gather.algorithm = my_gather;
   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || 
               queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying gather protocol %s, type was %d\n",
         my_gather_md->name, queryreq);
      result = my_gather_md->check_fn(&gather);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(!result.bitmask)
      {
         fprintf(stderr,"query failed for %s\n", my_gather_md->name);
      }
   }

   MPIDI_Update_last_algorithm(comm_ptr,
            comm_ptr->mpid.user_metadata[PAMI_XFER_GATHER].name);


   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for gather on %u\n", 
              threadID,
              my_gather_md->name,
              (unsigned) comm_ptr->context_id);
   }

   TRACE_ERR("%s gather\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
   MPIDI_Context_post(MPIDI_Context[0], &gather_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&gather);

   TRACE_ERR("Waiting on active: %d\n", active);
   MPID_PROGRESS_WAIT_WHILE(active);

   TRACE_ERR("Leaving MPIDO_Gather\n");
   return 0;
}
