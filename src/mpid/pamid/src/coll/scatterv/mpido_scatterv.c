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
 * \file src/coll/scatterv/mpido_scatterv.c
 * \brief ???
 */

#include <mpidimpl.h>


/* basically, everyone receives recvcount via bcast */
/* requires a contiguous/continous buffer on root though */
int MPIDO_Scatterv_bcast(void *sendbuf,
                         int *sendcounts,
                         int *displs,
                         MPI_Datatype sendtype,
                         void *recvbuf,
                         int recvcount,
                         MPI_Datatype recvtype,
                         int root,
                         MPID_Comm *comm_ptr,
                         int *mpierrno)
{
  int rank = comm_ptr->rank;
  int np = comm_ptr->local_size;
  char *tempbuf;
  int i, sum = 0, dtsize, rc=0, contig;
  MPID_Datatype *dt_ptr;
  MPI_Aint dt_lb;

  for (i = 0; i < np; i++)
    if (sendcounts > 0)
      sum += sendcounts[i];

  MPIDI_Datatype_get_info(1,
			  recvtype,
			  contig,
			  dtsize,
			  dt_ptr,
			  dt_lb);

  if (rank != root)
  {
    tempbuf = MPIU_Malloc(dtsize * sum);
    if (!tempbuf)
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  __FUNCTION__,
                                  __LINE__,
                                  MPI_ERR_OTHER,
                                  "**nomem", 0);
  }
  else
    tempbuf = sendbuf;

  rc = MPIDO_Bcast(tempbuf, sum, sendtype, root, comm_ptr, mpierrno);

  if(rank == root && recvbuf == MPI_IN_PLACE)
    return rc;

  memcpy(recvbuf, tempbuf + displs[rank], sendcounts[rank] * dtsize);

  if (rank != root)
    MPIU_Free(tempbuf);

  return rc;
}

/* this guy requires quite a few buffers. maybe
 * we should somehow "steal" the comm_ptr alltoall ones? */
int MPIDO_Scatterv_alltoallv(void * sendbuf,
                             int * sendcounts,
                             int * displs,
                             MPI_Datatype sendtype,
                             void * recvbuf,
                             int recvcount,
                             MPI_Datatype recvtype,
                             int root,
                             MPID_Comm * comm_ptr,
                             int *mpierrno)
{
  int rank = comm_ptr->rank;
  int size = comm_ptr->local_size;

  int *sdispls, *scounts;
  int *rdispls, *rcounts;
  char *sbuf, *rbuf;
  int contig, rbytes, sbytes;
  int rc;

  MPID_Datatype *dt_ptr;
  MPI_Aint dt_lb=0;

  MPIDI_Datatype_get_info(recvcount,
                          recvtype,
                          contig,
                          rbytes,
                          dt_ptr,
                          dt_lb);

  if(rank == root)
    MPIDI_Datatype_get_info(1, sendtype, contig, sbytes, dt_ptr, dt_lb);

  rbuf = MPIU_Malloc(size * rbytes * sizeof(char));
  if(!rbuf)
  {
    return MPIR_Err_create_code(MPI_SUCCESS,
                                MPIR_ERR_RECOVERABLE,
                                __FUNCTION__,
                                __LINE__,
                                MPI_ERR_OTHER,
                                "**nomem", 0);
  }
  /*   memset(rbuf, 0, rbytes * size * sizeof(char));*/

  if(rank == root)
  {
    sdispls = displs;
    scounts = sendcounts;
    sbuf = sendbuf;
  }
  else
  {
    sdispls = MPIU_Malloc(size * sizeof(int));
    scounts = MPIU_Malloc(size * sizeof(int));
    sbuf = MPIU_Malloc(rbytes * sizeof(char));
    if(!sdispls || !scounts || !sbuf)
    {
      if(sdispls)
        MPIU_Free(sdispls);
      if(scounts)
        MPIU_Free(scounts);
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  __FUNCTION__,
                                  __LINE__,
                                  MPI_ERR_OTHER,
                                  "**nomem", 0);
    }
    memset(sdispls, 0, size*sizeof(int));
    memset(scounts, 0, size*sizeof(int));
    /*      memset(sbuf, 0, rbytes * sizeof(char));*/
  }

  rdispls = MPIU_Malloc(size * sizeof(int));
  rcounts = MPIU_Malloc(size * sizeof(int));
  if(!rdispls || !rcounts)
  {
    if(rdispls)
      MPIU_Free(rdispls);
    return MPIR_Err_create_code(MPI_SUCCESS,
                                MPIR_ERR_RECOVERABLE,
                                __FUNCTION__,
                                __LINE__,
                                MPI_ERR_OTHER,
                                "**nomem", 0);
  }

  memset(rdispls, 0, size*sizeof(unsigned));
  memset(rcounts, 0, size*sizeof(unsigned));

  rcounts[root] = rbytes;

  rc = MPIR_Alltoallv(sbuf,
                  scounts,
                  sdispls,
                  sendtype,
                  rbuf,
                  rcounts,
                  rdispls,
                  MPI_CHAR,
                  comm_ptr,
                  mpierrno);

  if(rank == root && recvbuf == MPI_IN_PLACE)
  {
    MPIU_Free(rbuf);
    MPIU_Free(rdispls);
    MPIU_Free(rcounts);
    return rc;
  }
  else
  {
    /*      memcpy(recvbuf, rbuf+(root*rbytes), rbytes);*/
    memcpy(recvbuf, rbuf, rbytes);
    MPIU_Free(rbuf);
    MPIU_Free(rdispls);
    MPIU_Free(rcounts);
    if(rank != root)
    {
      MPIU_Free(sbuf);
      MPIU_Free(sdispls);
      MPIU_Free(scounts);
    }
  }

  return rc;
}

static void allred_cb_done(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   (*active)--;
}


static void cb_scatterv(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_scatterv enter, active: %u\n", (*active));
   (*active)--;
}

int MPIDO_Scatterv(const void *sendbuf,
                   const int *sendcounts,
                   const int *displs,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   int recvcount,
                   MPI_Datatype recvtype,
                   int root,
                   MPID_Comm *comm_ptr,
                   int *mpierrno)
{
  int contig, tmp, pamidt = 1;
  int ssize, rsize;
  MPID_Datatype *dt_ptr = NULL;
  MPI_Aint send_true_lb=0, recv_true_lb;
  char *sbuf, *rbuf;
  volatile unsigned allred_active = 1;
  pami_xfer_t allred;
  int optscatterv[3];
  pami_type_t stype, rtype;

  allred.cb_done = allred_cb_done;
  allred.cookie = (void *)&allred_active;
  allred.algorithm = comm_ptr->mpid.coll_algorithm[PAMI_XFER_ALLREDUCE][0][0];
  allred.cmd.xfer_allreduce.sndbuf = (void *)optscatterv;
  allred.cmd.xfer_allreduce.stype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.rcvbuf = (void *)optscatterv;
  allred.cmd.xfer_allreduce.rtype = PAMI_TYPE_SIGNED_INT;
  allred.cmd.xfer_allreduce.stypecount = 3;
  allred.cmd.xfer_allreduce.rtypecount = 3; 
  allred.cmd.xfer_allreduce.op = PAMI_DATA_BAND;


   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_USE_MPICH)
  {
    if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
      fprintf(stderr,"Using MPICH scatterv algorithm\n");
    MPIDI_Update_last_algorithm(comm_ptr, "SCATTERV_MPICH");
    return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                         recvbuf, recvcount, recvtype,
                         root, comm_ptr, mpierrno);
  }


   pami_xfer_t scatterv;
   pami_algorithm_t my_scatterv;
   pami_metadata_t *my_scatterv_md;
   volatile unsigned scatterv_active = 1;
   int queryreq = 0;

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized scatterv %s was selected\n",
         comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTERV_INT][0].name);
      my_scatterv = comm_ptr->mpid.opt_protocol[PAMI_XFER_SCATTERV_INT][0];
      my_scatterv_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_SCATTERV_INT][0];
      queryreq = comm_ptr->mpid.must_query[PAMI_XFER_SCATTERV_INT][0];
   }
   else
   {
      TRACE_ERR("User selected %s for scatterv\n",
         comm_ptr->mpid.user_selected[PAMI_XFER_SCATTERV_INT]);
      my_scatterv = comm_ptr->mpid.user_selected[PAMI_XFER_SCATTERV_INT];
      my_scatterv_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_SCATTERV_INT];
      queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT];
   }

   if((recvbuf != MPI_IN_PLACE) && MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   if(MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   if(pamidt == 0 || comm_ptr->mpid.user_selected_type[PAMI_XFER_SCATTERV_INT] == MPID_COLL_USE_MPICH)
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using MPICH scatterv algorithm\n");
      TRACE_ERR("Scatterv using MPICH\n");
      MPIDI_Update_last_algorithm(comm_ptr, "SCATTERV_MPICH");
      return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                           recvbuf, recvcount, recvtype,
                           root, comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(1, sendtype, contig, ssize, dt_ptr, send_true_lb);
   sbuf = (char *)sendbuf + send_true_lb;
   rbuf = recvbuf;

   if(comm_ptr->rank == root)
   {
      if(recvbuf == MPI_IN_PLACE) 
      {
        if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
          fprintf(stderr,"scatterv MPI_IN_PLACE buffering\n");
        rbuf = (char *)sendbuf + ssize*displs[comm_ptr->rank] + send_true_lb;
      }
      else
      {  
        MPIDI_Datatype_get_info(1, recvtype, contig, rsize, dt_ptr, recv_true_lb);
        rbuf = (char *)recvbuf + recv_true_lb;
      }
   }

   scatterv.cb_done = cb_scatterv;
   scatterv.cookie = (void *)&scatterv_active;
   scatterv.cmd.xfer_scatterv_int.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);

   scatterv.algorithm = my_scatterv;

   scatterv.cmd.xfer_scatterv_int.rcvbuf = rbuf;
   scatterv.cmd.xfer_scatterv_int.sndbuf = sbuf;
   scatterv.cmd.xfer_scatterv_int.stype = stype;
   scatterv.cmd.xfer_scatterv_int.rtype = rtype;
   scatterv.cmd.xfer_scatterv_int.stypecounts = (int *) sendcounts;
   scatterv.cmd.xfer_scatterv_int.rtypecount = recvcount;
   scatterv.cmd.xfer_scatterv_int.sdispls = (int *) displs;

   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying scatterv protocol %s, type was %d\n",
         my_scatterv_md->name, queryreq);
      result = my_scatterv_md->check_fn(&scatterv);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(!result.bitmask)
      {
         fprintf(stderr,"Query failed for %s\n", my_scatterv_md->name);
      }
   }

   MPIDI_Update_last_algorithm(comm_ptr, my_scatterv_md->name);

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for scatterv on %u\n", 
              threadID,
              my_scatterv_md->name,
              (unsigned) comm_ptr->context_id);
   }
   MPIDI_Post_coll_t scatterv_post;
   TRACE_ERR("%s scatterv\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
   MPIDI_Context_post(MPIDI_Context[0], &scatterv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&scatterv);

   TRACE_ERR("Waiting on active %d\n", scatterv_active);
   MPID_PROGRESS_WAIT_WHILE(scatterv_active);

   TRACE_ERR("Leaving MPIDO_Scatterv\n");
   return 0;
}




#if 0
/* remove the glue-based optimized scattervs for now. */


  /* we can't call scatterv-via-bcast unless we know all nodes have
   * valid sendcount arrays. so the user must explicitly ask for it.
   */

   /* optscatterv[0] == optscatterv bcast?
    * optscatterv[1] == optscatterv alltoall?
    *  (having both allows cutoff agreement)
    * optscatterv[2] == sum of sendcounts
    */

   optscatterv[0] = !comm_ptr->mpid.scattervs[0];
   optscatterv[1] = !comm_ptr->mpid.scattervs[1];
   optscatterv[2] = 1;

   if(rank == root)
   {
      if(!optscatterv[1])
      {
         if(sendcounts)
         {
            for(i=0;i<size;i++)
               sum+=sendcounts[i];
         }
         optscatterv[2] = sum;
      }

      MPIDI_Datatype_get_info(1,
                            sendtype,
                            contig,
                            nbytes,
                            dt_ptr,
                            true_lb);
      if(recvtype == MPI_DATATYPE_NULL || recvcount <= 0 || !contig)
      {
         optscatterv[0] = 1;
         optscatterv[1] = 1;
      }
  }
  else
  {
      MPIDI_Datatype_get_info(1,
                            recvtype,
                            contig,
                            nbytes,
                            dt_ptr,
                            true_lb);
      if(sendtype == MPI_DATATYPE_NULL || !contig)
      {
         optscatterv[0] = 1;
         optscatterv[1] = 1;
      }
   }

  /* Make sure parameters are the same on all the nodes */
  /* specifically, noncontig on the receive */
  /* set the internal control flow to disable internal star tuning */
   if(comm_ptr->mpid.preallreduces[MPID_SCATTERV_PREALLREDUCE])
   {
     TRACE_ERR("%s scatterv pre-allreduce\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
     MPIDI_Post_coll_t allred_post;
     rc = MPIDI_Context_post(MPIDI_Context[0], &allred_post.state,
                             MPIDI_Pami_post_wrapper, (void *)&allred);

      MPID_PROGRESS_WAIT_WHILE(allred_active);
   }
  /* reset flag */

   if(!optscatterv[0] || (!optscatterv[1]))
   {
      char *newrecvbuf = recvbuf;
      char *newsendbuf = sendbuf;
      if(rank == root)
      {
         newsendbuf = (char *)sendbuf + true_lb;
      }
      else
      {
         newrecvbuf = (char *)recvbuf + true_lb;
      }
      if(!optscatterv[0])
      {
         MPIDI_Update_last_algorithm(comm_ptr, "SCATTERV_OPT_ALLTOALL");
        return MPIDO_Scatterv_alltoallv(newsendbuf,
                                        sendcounts,
                                        displs,
                                        sendtype,
                                        newrecvbuf,
                                        recvcount,
                                        recvtype,
                                        root,
                                        comm_ptr,
                                        mpierrno);

      }
      else
      {
         MPIDI_Update_last_algorithm(comm_ptr, "SCATTERV_OPT_BCAST");
        return MPIDO_Scatterv_bcast(newsendbuf,
                                    sendcounts,
                                    displs,
                                    sendtype,
                                    newrecvbuf,
                                    recvcount,
                                    recvtype,
                                    root,
                                    comm_ptr,
                                    mpierrno);
      }
   } /* nothing valid to try, go to mpich */
   else
   {
     if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
       fprintf(stderr,"Using MPICH scatterv algorithm\n");
      MPIDI_Update_last_algorithm(comm_ptr, "SCATTERV_MPICH");
      return MPIR_Scatterv(sendbuf, sendcounts, displs, sendtype,
                           recvbuf, recvcount, recvtype,
                           root, comm_ptr, mpierrno);
   }
#endif
