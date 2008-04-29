/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoall/mpido_alltoall.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Alltoall = MPIDO_Alltoall

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


static int torus_alltoall(char *sendbuf,
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
//   fprintf(stderr,"torus alltoall\n");
   /* uses the alltoallv protocol */
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
MPIDO_Alltoall(void *sendbuf,
               int sendcount,
               MPI_Datatype sendtype,
               void *recvbuf,
               int recvcount,
               MPI_Datatype recvtype,
               MPID_Comm *comm_ptr)
{
   int numprocs = comm_ptr->local_size;
   int tsndlen, trcvlen, snd_contig, rcv_contig, rc, i;
   MPI_Aint sdt_true_lb, rdt_true_lb;
   MPID_Datatype *dt_null = NULL;

   MPIDI_Datatype_get_info(sendcount, sendtype, snd_contig,
                           tsndlen, dt_null, sdt_true_lb);
   MPIDI_Datatype_get_info(recvcount, recvtype, rcv_contig,
                           trcvlen, dt_null, rdt_true_lb);

   if(sendcount == 0 || recvcount == 0)
      return MPI_SUCCESS;

   /* We only keep one protocol - alltoallv, but the
    * use flag is separate. */
   if(!comm_ptr->dcmf.alltoalls ||
      !snd_contig ||
      !rcv_contig ||
      tsndlen != trcvlen ||
      numprocs < 2 ||
      !MPIDI_CollectiveProtocols.alltoall.usetorus ||
      !(DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
                        &MPIDI_CollectiveProtocols.alltoallv.torus)))
   {
      return MPIR_Alltoall(sendbuf, sendcount, sendtype,
                           recvbuf, recvcount, recvtype,
                           comm_ptr);
   }
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
                                  "MPI_Alltoall",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

  /* ---------------------------------------------- */
  /* Initialize the send buffers and lengths        */
  /* pktInject is the number of packets to inject   */
  /* per advance loop.  The best performance is 2   */
  /* ---------------------------------------------- */
  for (i = 0; i < numprocs; i++)
    {
      comm_ptr->dcmf.sndlen [i] =     tsndlen;
      comm_ptr->dcmf.sdispls[i] = i * tsndlen;
      comm_ptr->dcmf.rcvlen [i] =     trcvlen;
      comm_ptr->dcmf.rdispls[i] = i * trcvlen;
    }

  /* ---------------------------------------------- */
  /* Create a message layer collective message      */
  /* ---------------------------------------------- */

   rc = torus_alltoall((char *)sendbuf + sdt_true_lb,
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
