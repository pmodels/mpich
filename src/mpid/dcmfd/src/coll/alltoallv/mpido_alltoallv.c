/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoallv/mpido_alltoallv.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Alltoallv = MPIDO_Alltoallv
/**
 * **************************************************************************
 * \brief "Done" callback for collective alltoall message.
 * **************************************************************************
 */

static void
cb_done(void *clientdata)
{
  volatile unsigned *work_left = (unsigned *) clientdata;
  *work_left = 0;
   MPID_Progress_signal();
  return;
}

static int torus_alltoallv(char *sendbuf,
                           unsigned *sndlen,
                           unsigned *sdispls,
                           char *recvbuf,
                           unsigned *rcvlen,
                           unsigned *rdispls,
                           unsigned *sndcounters,
                           unsigned *rcvcounters,
                           DCMF_Geometry_t *geometry)
{
   int rc;
   DCMF_CollectiveRequest_t request;
   volatile unsigned active = 1;
   DCMF_Callback_t callback = { cb_done, (void *) &active };
//   fprintf(stderr,"torus alltoallv\n");

   rc = DCMF_Alltoallv(&MPIDI_CollectiveProtocols.alltoallv.torus,
                       &request,
                       callback,
                       DCMF_MATCH_CONSISTENCY,
                       geometry,
                       sendbuf,
                       sndlen,
                       sdispls,
                       recvbuf,
                       rcvlen,
                       rdispls,
                       sndcounters,
                       rcvcounters);

   MPID_PROGRESS_WAIT_WHILE(active);
   return rc;
}


int
MPIDO_Alltoallv(void *sendbuf,
                int *sendcounts,
                int *senddispls,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *recvdispls,
                MPI_Datatype recvtype,
                MPID_Comm *comm_ptr)
{

   int numprocs = comm_ptr->local_size;
   int tsndlen, trcvlen, snd_contig, rcv_contig, rc ,i;
   MPI_Aint sdt_true_lb, rdt_true_lb;
   MPID_Datatype *dt_null = NULL;

   MPIDI_Datatype_get_info(1, sendtype, snd_contig, tsndlen,
                           dt_null, sdt_true_lb);
   MPIDI_Datatype_get_info(1, recvtype, rcv_contig, trcvlen,
                           dt_null, rdt_true_lb);

   if(!comm_ptr->dcmf.alltoalls ||
      !snd_contig ||
      !rcv_contig ||
      tsndlen != trcvlen ||
      numprocs < 2 ||
      !MPIDI_CollectiveProtocols.alltoallv.usetorus ||
      !(DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
               &MPIDI_CollectiveProtocols.alltoallv.torus)))
   {
      return MPIR_Alltoallv(sendbuf, sendcounts, senddispls, sendtype,
                            recvbuf, recvcounts, recvdispls, recvtype,
                            comm_ptr);
   }

  /* ---------------------------------------------- */
  /* Allocate all data needed by alltoallv algorithm */
  /* ---------------------------------------------- */
   if(!comm_ptr->dcmf.sndlen)
      comm_ptr->dcmf.sndlen = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!comm_ptr->dcmf.rcvlen)
      comm_ptr->dcmf.rcvlen = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!comm_ptr->dcmf.sdispls)
      comm_ptr->dcmf.sdispls = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!comm_ptr->dcmf.rdispls)
      comm_ptr->dcmf.rdispls = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!comm_ptr->dcmf.sndcounters)
      comm_ptr->dcmf.sndcounters = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!comm_ptr->dcmf.rcvcounters)
      comm_ptr->dcmf.rcvcounters = MPIU_Malloc(numprocs * sizeof(unsigned));

   if(!comm_ptr->dcmf.sndlen || !comm_ptr->dcmf.rcvlen ||
      !comm_ptr->dcmf.sdispls || !comm_ptr->dcmf.rdispls ||
      !comm_ptr->dcmf.sndcounters || !comm_ptr->dcmf.rcvcounters)
   {
      if(comm_ptr->dcmf.sndlen) MPIU_Free(comm_ptr->dcmf.sndlen);
      if(comm_ptr->dcmf.rcvlen) MPIU_Free(comm_ptr->dcmf.rcvlen);
      if(comm_ptr->dcmf.sdispls) MPIU_Free(comm_ptr->dcmf.sdispls);
      if(comm_ptr->dcmf.rdispls) MPIU_Free(comm_ptr->dcmf.rdispls);
      if(comm_ptr->dcmf.sndcounters) MPIU_Free(comm_ptr->dcmf.sndcounters);
      if(comm_ptr->dcmf.rcvcounters) MPIU_Free(comm_ptr->dcmf.rcvcounters);
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Alltoallv",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

  /* ---------------------------------------------- */
  /* Initialize the send buffers and lengths        */
  /* pktInject is the number of packets to inject   */
  /* per advance loop.  The best performance is 2   */
  /* ---------------------------------------------- */
  for (i = 0; i < numprocs; i++)
    {
      comm_ptr->dcmf.sndlen [i] = tsndlen * sendcounts[i];
      comm_ptr->dcmf.sdispls[i] = tsndlen * senddispls[i];
      comm_ptr->dcmf.rcvlen [i] = trcvlen * recvcounts[i];
      comm_ptr->dcmf.rdispls[i] = trcvlen * recvdispls[i];
    }


  /* ---------------------------------------------- */
  /* Create a message layer collective message      */
  /* ---------------------------------------------- */

   rc = torus_alltoallv((char *)sendbuf + sdt_true_lb,
                        comm_ptr->dcmf.sndlen,
                        comm_ptr->dcmf.sdispls,
                        (char *)recvbuf + rdt_true_lb,
                        comm_ptr->dcmf.rcvlen,
                        comm_ptr->dcmf.rdispls,
                        comm_ptr->dcmf.sndcounters,
                        comm_ptr->dcmf.rcvcounters,
                        &comm_ptr->dcmf.geometry);


  return rc;
}
