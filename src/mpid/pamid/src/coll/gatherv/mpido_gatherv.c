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
 * \file src/coll/gather/mpido_gatherv.c
 * \brief ???
 */

/*#define TRACE_ON*/
#include <mpidimpl.h>

static void cb_gatherv(void *ctxt, void *clientdata, pami_result_t err)
{
   unsigned *active = (unsigned *)clientdata;
   TRACE_ERR("cb_gatherv enter, active: %u\n", (*active));
   (*active)--;
}

int MPIDO_Gatherv(const void *sendbuf, 
                  int sendcount, 
                  MPI_Datatype sendtype,
                  void *recvbuf, 
                  const int *recvcounts, 
                  const int *displs, 
                  MPI_Datatype recvtype,
                  int root, 
                  MPID_Comm * comm_ptr, 
                  int *mpierrno)

{
   TRACE_ERR("Entering MPIDO_Gatherv\n");
   int contig, rsize, ssize;
   int pamidt = 1;
   ssize = 0;
   MPID_Datatype *dt_ptr = NULL;
   MPI_Aint send_true_lb, recv_true_lb;
   char *sbuf, *rbuf;
   pami_type_t stype, rtype;
   int tmp;
   volatile unsigned gatherv_active = 1;

   /* Check for native PAMI types and MPI_IN_PLACE on sendbuf */
   /* MPI_IN_PLACE is a nonlocal decision. We will need a preallreduce if we ever have
    * multiple "good" gathervs that work on different counts for example */
   if((sendbuf != MPI_IN_PLACE) && (MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS))
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   if(pamidt == 0 || comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHERV_INT] == MPID_COLL_USE_MPICH)
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH gatherv algorithm\n");
      TRACE_ERR("GATHERV using MPICH\n");
      MPIDI_Update_last_algorithm(comm_ptr, "GATHERV_MPICH");
      return MPIR_Gatherv(sendbuf, sendcount, sendtype,
               recvbuf, recvcounts, displs, recvtype,
               root, comm_ptr, mpierrno);
   }

   MPIDI_Datatype_get_info(1, recvtype, contig, rsize, dt_ptr, recv_true_lb);
   rbuf = (char *)recvbuf + recv_true_lb;
   sbuf = (void *) sendbuf;

   pami_xfer_t gatherv;

   gatherv.cb_done = cb_gatherv;
   gatherv.cookie = (void *)&gatherv_active;
   gatherv.cmd.xfer_gatherv_int.root = MPID_VCR_GET_LPID(comm_ptr->vcr, root);
   gatherv.cmd.xfer_gatherv_int.rcvbuf = rbuf;
   gatherv.cmd.xfer_gatherv_int.rtype = rtype;
   gatherv.cmd.xfer_gatherv_int.rtypecounts = (int *) recvcounts;
   gatherv.cmd.xfer_gatherv_int.rdispls = (int *) displs;

   gatherv.cmd.xfer_gatherv_int.sndbuf = NULL;
   gatherv.cmd.xfer_gatherv_int.stype = stype;
   gatherv.cmd.xfer_gatherv_int.stypecount = sendcount;

   if(comm_ptr->rank == root)
   {
      if(sendbuf == MPI_IN_PLACE) 
      {
         if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
            fprintf(stderr,"gatherv MPI_IN_PLACE buffering\n");
         sbuf = (char*)rbuf + rsize*displs[comm_ptr->rank];
         gatherv.cmd.xfer_gatherv_int.stype = rtype;
         gatherv.cmd.xfer_gatherv_int.stypecount = recvcounts[comm_ptr->rank];
      }
      else
      {
         MPIDI_Datatype_get_info(1, sendtype, contig, ssize, dt_ptr, send_true_lb);
         sbuf = (char *)sbuf + send_true_lb;
      }
   }
   gatherv.cmd.xfer_gatherv_int.sndbuf = sbuf;

   pami_algorithm_t my_gatherv;
   pami_metadata_t *my_gatherv_md;
   int queryreq = 0;

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHERV_INT] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized gatherv %s was selected\n",
         comm_ptr->mpid.opt_protocol_md[PAMI_XFER_GATHERV_INT][0].name);
      my_gatherv = comm_ptr->mpid.opt_protocol[PAMI_XFER_GATHERV_INT][0];
      my_gatherv_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_GATHERV_INT][0];
      queryreq = comm_ptr->mpid.must_query[PAMI_XFER_GATHERV_INT][0];
   }
   else
   {
      TRACE_ERR("Optimized gatherv %s was set by user\n",
         comm_ptr->mpid.user_metadata[PAMI_XFER_GATHERV_INT].name);
         my_gatherv = comm_ptr->mpid.user_selected[PAMI_XFER_GATHERV_INT];
         my_gatherv_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_GATHERV_INT];
         queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_GATHERV_INT];
   }

   gatherv.algorithm = my_gatherv;


   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying gatherv protocol %s, type was %d\n", 
         my_gatherv_md->name, queryreq);
      result = my_gatherv_md->check_fn(&gatherv);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(!result.bitmask)
      {
         fprintf(stderr,"Query failed for %s\n", my_gatherv_md->name);
      }
   }
   
   MPIDI_Update_last_algorithm(comm_ptr, my_gatherv_md->name);

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for gatherv on %u\n", 
              threadID,
              my_gatherv_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Post_coll_t gatherv_post;
   TRACE_ERR("%s gatherv\n", MPIDI_Process.context_post.active>0?"Posting":"Invoking");
   MPIDI_Context_post(MPIDI_Context[0], &gatherv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&gatherv);
   TRACE_ERR("Gatherv %s\n", MPIDI_Process.context_post.active>0?"posted":"invoked");
   
   TRACE_ERR("Waiting on active %d\n", gatherv_active);
   MPID_PROGRESS_WAIT_WHILE(gatherv_active);

   TRACE_ERR("Leaving MPIDO_Gatherv\n");
   return 0;
}
