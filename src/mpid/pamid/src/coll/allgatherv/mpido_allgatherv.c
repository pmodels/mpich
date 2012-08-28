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

/*#define TRACE_ON */
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
  int start, rc;
  int length;
  char *startbuf = NULL;
  char *destbuf = NULL;
  TRACE_ERR("Entering MPIDO_Allgatherv_allreduce\n");

  startbuf = (char *) recvbuf + recv_true_lb;
  destbuf = startbuf + displs[comm_ptr->rank] * recv_size;

  start = 0;
  length = displs[comm_ptr->rank] * recv_size;
  memset(startbuf + start, 0, length);

  start  = (displs[comm_ptr->rank] +
	    recvcounts[comm_ptr->rank]) * recv_size;
  length = buffer_sum - (displs[comm_ptr->rank] +
			 recvcounts[comm_ptr->rank]) * recv_size;
  memset(startbuf + start, 0, length);

  if (sendbuf != MPI_IN_PLACE)
  {
    char *outputbuf = (char *) sendbuf + send_true_lb;
    memcpy(destbuf, outputbuf, send_size);
  }

  /*if (0==comm_ptr->rank) puts("allreduce allgatherv");*/

   TRACE_ERR("Calling MPIDO_Allreduce from MPIDO_Allgatherv_allreduce\n");
   /* TODO: Change to PAMI allreduce */
  rc = MPIDO_Allreduce(MPI_IN_PLACE,
		       startbuf,
		       buffer_sum/sizeof(int),
		       MPI_INT,
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
   TRACE_ERR("Entering MPIDO_Allgatherv_bcast\n");
  int i, rc=MPI_ERR_INTERN;
  MPI_Aint extent;
  MPID_Datatype_get_extent_macro(recvtype, extent);

  if (sendbuf != MPI_IN_PLACE)
  {
    void *destbuffer = recvbuf + displs[comm_ptr->rank] * extent;
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   destbuffer,
                   recvcounts[comm_ptr->rank],
                   recvtype);
  }

   TRACE_ERR("Calling MPIDO_Bcasts in MPIDO_Allgatherv_bcast\n");
  for (i = 0; i < comm_ptr->local_size; i++)
  {
    void *destbuffer = recvbuf + displs[i] * extent;
    /* TODO: Change to PAMI */
    rc = MPIDO_Bcast(destbuffer,
                     recvcounts[i],
                     recvtype,
                     i,
                     comm_ptr,
                     mpierrno);
  }
  /*if (0==comm_ptr->rank) puts("bcast allgatherv");*/
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
  int a2a_sendcounts[comm_ptr->local_size];
  int a2a_senddispls[comm_ptr->local_size];

  total_send_size = recvcounts[comm_ptr->rank] * recv_size;
  for (i = 0; i < comm_ptr->local_size; ++i)
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
    destbuf = startbuf + displs[comm_ptr->rank] * recv_size;
    a2a_sendbuf = destbuf;
    a2a_sendcounts[comm_ptr->rank] = 0;
    my_recvcounts = recvcounts[comm_ptr->rank];
    recvcounts[comm_ptr->rank] = 0;
  }

   TRACE_ERR("Calling alltoallv in MPIDO_Allgatherv_alltoallv\n");
   /* TODO: Change to PAMI alltoallv */
  /*if (0==comm_ptr->rank) puts("all2all allgatherv");*/
  rc = MPIR_Alltoallv(a2a_sendbuf,
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
    recvcounts[comm_ptr->rank] = my_recvcounts;

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
   TRACE_ERR("Entering MPIDO_Allgatherv\n");
  /* function pointer to be used to point to approperiate algorithm */

  /* Check the nature of the buffers */
  MPID_Datatype *dt_null = NULL;
  MPI_Aint send_true_lb  = 0;
  MPI_Aint recv_true_lb  = 0;
  size_t   send_size     = 0;
  size_t   recv_size     = 0;
  int config[6];
  double msize;
  int scount=sendcount;

  int i, rc, buffer_sum = 0, np = comm_ptr->local_size;
  char use_tree_reduce, use_alltoall, use_bcast, use_pami, use_opt;
  char *sbuf, *rbuf;

  pami_xfer_t allred;
  volatile unsigned allred_active = 1;
  volatile unsigned allgatherv_active = 1;
  pami_type_t stype, rtype;
  int tmp;
  pami_metadata_t *my_md;

  for(i=0;i<6;i++) config[i] = 1;

  allred.cb_done = allred_cb_done;
  allred.cookie = (void *)&allred_active;
  allred.algorithm = comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][0][0];
  allred.cmd.xfer_allreduce.sndbuf = (void *)config;
  allred.cmd.xfer_allreduce.stype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.rcvbuf = (void *)config;
  allred.cmd.xfer_allreduce.rtype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.stypecount = 6;
  allred.cmd.xfer_allreduce.rtypecount = 6;
  allred.cmd.xfer_allreduce.op = PAMI_DATA_BAND;

   use_alltoall = comm_ptr->mpid.allgathervs[2];
   use_tree_reduce = comm_ptr->mpid.allgathervs[0];
   use_bcast = comm_ptr->mpid.allgathervs[1];
   /* Assuming PAMI doesn't support MPI_IN_PLACE */
   use_pami = comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] != MPID_COLL_USE_MPICH;
	 
   if((sendbuf != MPI_IN_PLACE) && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
     use_pami = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
     use_pami = 0;

   use_opt = use_alltoall || use_tree_reduce || use_bcast || use_pami;

   if(!use_opt) /* back to MPICH */
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
     fprintf(stderr,"Using MPICH allgatherv type %u.\n",
             comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
     TRACE_ERR("Using MPICH Allgatherv\n");
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_MPICH");
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
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
         fprintf(stderr,"allgatherv MPI_IN_PLACE buffering\n");
        sbuf = (char *)recvbuf+displs[comm_ptr->rank]*recv_size;
        send_true_lb = recv_true_lb;
        stype = rtype;
        scount = recvcounts[comm_ptr->rank];
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

      for (i = 1; i < np; i++)
      {
        buffer_sum += recvcounts[i - 1];
        if (buffer_sum != displs[i])
        {
          config[MPID_RECV_CONTINUOUS] = 0;
          break;
        }
      }

      buffer_sum += recvcounts[np - 1];

      buffer_sum *= recv_size;
      msize = (double)buffer_sum / (double)np;

      /* disable with "safe allgatherv" env var */
      if(comm_ptr->mpid.preallreduces[MPID_ALLGATHERV_PREALLREDUCE])
      {
         MPIDI_Post_coll_t allred_post;
         MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                            MPIDI_Pami_post_wrapper, (void *)&allred);

         MPID_PROGRESS_WAIT_WHILE(allred_active);
      }

      use_tree_reduce = comm_ptr->mpid.allgathervs[0] &&
         config[MPID_RECV_CONTIG] && config[MPID_SEND_CONTIG] &&
         config[MPID_RECV_CONTINUOUS] && buffer_sum % sizeof(int) == 0;

      use_alltoall = comm_ptr->mpid.allgathervs[2] &&
         config[MPID_RECV_CONTIG] && config[MPID_SEND_CONTIG];

      use_bcast = comm_ptr->mpid.allgathervs[1];
   }

   if(use_pami)
   {
      pami_xfer_t allgatherv;
      allgatherv.cb_done = allgatherv_cb_done;
      allgatherv.cookie = (void *)&allgatherv_active;
      if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_OPTIMIZED)
      {  
        allgatherv.algorithm = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLGATHERV_INT][0];
        my_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLGATHERV_INT][0];
      }
      else
      {  
        allgatherv.algorithm = comm_ptr->mpid.user_selected[PAMI_XFER_ALLGATHERV_INT];
        my_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_ALLGATHERV_INT];
      }
      
      allgatherv.cmd.xfer_allgatherv_int.sndbuf = sbuf;
      allgatherv.cmd.xfer_allgatherv_int.rcvbuf = rbuf;

      allgatherv.cmd.xfer_allgatherv_int.stype = stype;
      allgatherv.cmd.xfer_allgatherv_int.rtype = rtype;
      allgatherv.cmd.xfer_allgatherv_int.stypecount = scount;
      allgatherv.cmd.xfer_allgatherv_int.rtypecounts = (int *) recvcounts;
      allgatherv.cmd.xfer_allgatherv_int.rdispls = (int *) displs;

      if(unlikely (comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_ALWAYS_QUERY ||
                   comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT] == MPID_COLL_CHECK_FN_REQUIRED))
      {
         metadata_result_t result = {0};
         TRACE_ERR("Querying allgatherv_int protocol %s, type was %d\n", my_md->name,
            comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
         result = my_md->check_fn(&allgatherv);
         TRACE_ERR("Allgatherv bitmask: %#X\n", result.bitmask);
         if(!result.bitmask)
         {
            fprintf(stderr,"Query failed for %s\n", my_md->name);
         }
      }

      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
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

      TRACE_ERR("Calling allgatherv via %s()\n", MPIDI_Process.context_post.active>0?"PAMI_Collective":"PAMI_Context_post");
      MPIDI_Post_coll_t allgatherv_post;
      MPIDI_Context_post(MPIDI_Context[0], &allgatherv_post.state,
                         MPIDI_Pami_post_wrapper, (void *)&allgatherv);

      MPIDI_Update_last_algorithm(comm_ptr, my_md->name);

      TRACE_ERR("Rank %d waiting on active %d\n", comm_ptr->rank, allgatherv_active);
      MPID_PROGRESS_WAIT_WHILE(allgatherv_active);

      return PAMI_SUCCESS;
   }

   /* TODO These need ordered in speed-order */
   if(use_tree_reduce)
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using tree reduce allgatherv type %u.\n",
               comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
     rc = MPIDO_Allgatherv_allreduce(sendbuf, sendcount, sendtype,
             recvbuf, recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_ALLREDUCE");
     return rc;
   }

   if(use_bcast)
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using bcast allgatherv type %u.\n",
               comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
     rc = MPIDO_Allgatherv_bcast(sendbuf, sendcount, sendtype,
             recvbuf, recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_BCAST");
     return rc;
   }

   if(use_alltoall)
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using alltoall allgatherv type %u.\n",
               comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
     rc = MPIDO_Allgatherv_alltoall(sendbuf, sendcount, sendtype,
             recvbuf, (int *)recvcounts, buffer_sum, displs, recvtype,
             send_true_lb, recv_true_lb, send_size, recv_size,
             comm_ptr, mpierrno);
     MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_OPT_ALLTOALL");
     return rc;
   }

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
      fprintf(stderr,"Using MPICH allgatherv type %u.\n",
            comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLGATHERV_INT]);
   TRACE_ERR("Using MPICH for Allgatherv\n");
   MPIDI_Update_last_algorithm(comm_ptr, "ALLGATHERV_MPICH");
   return MPIR_Allgatherv(sendbuf, sendcount, sendtype,
                       recvbuf, recvcounts, displs, recvtype,
                       comm_ptr, mpierrno);
}
