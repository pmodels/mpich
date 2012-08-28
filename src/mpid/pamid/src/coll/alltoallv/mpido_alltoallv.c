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
 * \file src/coll/alltoallv/mpido_alltoallv.c
 * \brief ???
 */
/*#define TRACE_ON*/

#include <mpidimpl.h>

static void cb_alltoallv(void *ctxt, void *clientdata, pami_result_t err)
{
   int *active = (int *)clientdata;
   TRACE_ERR("alltoallv callback enter, active: %d\n", (*active));
   MPIDI_Progress_signal();
   (*active)--;
}


int MPIDO_Alltoallv(const void *sendbuf,
                   const int *sendcounts,
                   const int *senddispls,
                   MPI_Datatype sendtype,
                   void *recvbuf,
                   const int *recvcounts,
                   const int *recvdispls,
                   MPI_Datatype recvtype,
                   MPID_Comm *comm_ptr,
                   int *mpierrno)
{
   if(comm_ptr->rank == 0)
      TRACE_ERR("Entering MPIDO_Alltoallv\n");
   volatile unsigned active = 1;
   int sndtypelen, rcvtypelen, snd_contig, rcv_contig;
   MPID_Datatype *sdt, *rdt;
   pami_type_t stype, rtype;
   MPI_Aint sdt_true_lb, rdt_true_lb;
   MPIDI_Post_coll_t alltoallv_post;
   int pamidt = 1;
   int tmp;

   if(sendbuf == MPI_IN_PLACE) 
     pamidt = 0; /* Disable until ticket #632 is fixed */
   if(MPIDI_Datatype_to_pami(sendtype, &stype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;
   if(MPIDI_Datatype_to_pami(recvtype, &rtype, -1, NULL, &tmp) != MPI_SUCCESS)
      pamidt = 0;

   if(
      (comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] == 
            MPID_COLL_USE_MPICH) ||
       pamidt == 0)
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
         fprintf(stderr,"Using MPICH alltoallv algorithm\n");
      if(!comm_ptr->rank)
         TRACE_ERR("Using MPICH alltoallv\n");
      return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                            recvbuf, recvcounts, recvdispls, recvtype,
                            comm_ptr, mpierrno);
   }
   if(!comm_ptr->rank)
      TRACE_ERR("Using %s for alltoallv protocol\n", pname);

   MPIDI_Datatype_get_info(1, recvtype, rcv_contig, rcvtypelen, rdt, rdt_true_lb);

   pami_xfer_t alltoallv;
   pami_algorithm_t my_alltoallv;
   pami_metadata_t *my_alltoallv_md;
   int queryreq = 0;

   if(comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT] == MPID_COLL_OPTIMIZED)
   {
      TRACE_ERR("Optimized alltoallv was selected\n");
      my_alltoallv = comm_ptr->mpid.opt_protocol[PAMI_XFER_ALLTOALLV_INT][0];
      my_alltoallv_md = &comm_ptr->mpid.opt_protocol_md[PAMI_XFER_ALLTOALLV_INT][0];
      queryreq = comm_ptr->mpid.must_query[PAMI_XFER_ALLTOALLV_INT][0];
   }
   else
   { /* is this purely an else? or do i need to check for some other selectedvar... */
      TRACE_ERR("Alltoallv specified by user\n");
      my_alltoallv = comm_ptr->mpid.user_selected[PAMI_XFER_ALLTOALLV_INT];
      my_alltoallv_md = &comm_ptr->mpid.user_metadata[PAMI_XFER_ALLTOALLV_INT];
      queryreq = comm_ptr->mpid.user_selected_type[PAMI_XFER_ALLTOALLV_INT];
   }
   alltoallv.algorithm = my_alltoallv;
   char *pname = my_alltoallv_md->name;


   alltoallv.cb_done = cb_alltoallv;
   alltoallv.cookie = (void *)&active;
   /* We won't bother with alltoallv since MPI is always going to be ints. */
   if(sendbuf == MPI_IN_PLACE)
   {
      if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL))
         fprintf(stderr,"alltoallv MPI_IN_PLACE buffering\n");
      alltoallv.cmd.xfer_alltoallv_int.stype = rtype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) recvdispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) recvcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = (char *)recvbuf+rdt_true_lb;
   }
   else
   {
      MPIDI_Datatype_get_info(1, sendtype, snd_contig, sndtypelen, sdt, sdt_true_lb);
      alltoallv.cmd.xfer_alltoallv_int.stype = stype;
      alltoallv.cmd.xfer_alltoallv_int.sdispls = (int *) senddispls;
      alltoallv.cmd.xfer_alltoallv_int.stypecounts = (int *) sendcounts;
      alltoallv.cmd.xfer_alltoallv_int.sndbuf = (char *)sendbuf+sdt_true_lb;
   }
   alltoallv.cmd.xfer_alltoallv_int.rcvbuf = (char *)recvbuf+rdt_true_lb;
      
   alltoallv.cmd.xfer_alltoallv_int.rdispls = (int *) recvdispls;
   alltoallv.cmd.xfer_alltoallv_int.rtypecounts = (int *) recvcounts;
   alltoallv.cmd.xfer_alltoallv_int.rtype = rtype;

   if(unlikely(queryreq == MPID_COLL_ALWAYS_QUERY || queryreq == MPID_COLL_CHECK_FN_REQUIRED))
   {
      metadata_result_t result = {0};
      TRACE_ERR("querying alltoallv protocol %s, type was %d\n", pname, queryreq);
      result = my_alltoallv_md->check_fn(&alltoallv);
      TRACE_ERR("bitmask: %#X\n", result.bitmask);
      if(!result.bitmask)
      {
         fprintf(stderr,"Query failed for %s\n", pname);
      }
   }

   if(unlikely(MPIDI_Process.verbose >= MPIDI_VERBOSE_DETAILS_ALL && comm_ptr->rank == 0))
   {
      unsigned long long int threadID;
      MPIU_Thread_id_t tid;
      MPIU_Thread_self(&tid);
      threadID = (unsigned long long int)tid;
      fprintf(stderr,"<%llx> Using protocol %s for alltoallv on %u\n", 
              threadID,
              my_alltoallv_md->name,
              (unsigned) comm_ptr->context_id);
   }

   MPIDI_Context_post(MPIDI_Context[0], &alltoallv_post.state,
                      MPIDI_Pami_post_wrapper, (void *)&alltoallv);

   TRACE_ERR("%d waiting on active %d\n", comm_ptr->rank, active);
   MPID_PROGRESS_WAIT_WHILE(active);


   TRACE_ERR("Leaving alltoallv\n");


   return 0;
}
