/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoallw/mpido_alltoallw.c
 * \brief ???
 */

#include "mpido_coll.h"

#pragma weak PMPIDO_Alltoallw = MPIDO_Alltoallw
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

static int torus_alltoallw(char *sendbuf,
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
MPIDO_Alltoallw(void *sendbuf,
                int *sendcounts,
                int *senddispls,
                MPI_Datatype *sendtypes,
                void *recvbuf,
                int *recvcounts,
                int *recvdispls,
                MPI_Datatype *recvtypes,
                MPID_Comm *comm_ptr)
{

   int numprocs = comm_ptr->local_size;
   int *tsndlen, *trcvlen, snd_contig, rcv_contig, rc ,i;
   MPI_Aint sdt_true_lb=0, rdt_true_lb=0;
   MPID_Datatype *dt_null = NULL;

   tsndlen = MPIU_Malloc(numprocs * sizeof(unsigned));
   trcvlen = MPIU_Malloc(numprocs * sizeof(unsigned));
   if(!tsndlen || !trcvlen)
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Alltoallw",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);

   for(i=0;i<numprocs;i++)
   {
      MPIDI_Datatype_get_info(1, sendtypes[i], snd_contig, tsndlen[i],
                           dt_null, sdt_true_lb);
      MPIDI_Datatype_get_info(1, recvtypes[i], rcv_contig, trcvlen[i],
                           dt_null, rdt_true_lb);

      MPID_Ensure_Aint_fits_in_pointer(
         MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf + sdt_true_lb);
      MPID_Ensure_Aint_fits_in_pointer( 
         MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + rdt_true_lb);


      if(!comm_ptr->dcmf.alltoalls ||
         !snd_contig ||
         !rcv_contig ||
         tsndlen[i] != trcvlen[i] ||
         numprocs < 2 ||
         !MPIDI_CollectiveProtocols.alltoallw.usetorus ||
         !(DCMF_Geometry_analyze(&comm_ptr->dcmf.geometry,
               &MPIDI_CollectiveProtocols.alltoallv.torus)))
      {
         if(tsndlen) free(tsndlen);
         if(trcvlen) free(trcvlen);
         return MPIR_Alltoallw(sendbuf, sendcounts, senddispls, sendtypes,
                               recvbuf, recvcounts, recvdispls, recvtypes,
                               comm_ptr);
      }
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
                                  "MPI_Alltoallw",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
   }

  /* ---------------------------------------------- */
  /* Initialize the send buffers and lengths        */
  /* pktInject is the number of packets to inject   */
  /* per advance loop.  The best performance is 2   */
  /* ---------------------------------------------- */
  for (i = 0; i < numprocs; i++)
    {
      comm_ptr->dcmf.sndlen [i] = tsndlen[i] * sendcounts[i];
      comm_ptr->dcmf.sdispls[i] = senddispls[i];

      comm_ptr->dcmf.rcvlen [i] = trcvlen[i] * recvcounts[i];
      comm_ptr->dcmf.rdispls[i] = recvdispls[i];
    }


  /* ---------------------------------------------- */
  /* Create a message layer collective message      */
  /* ---------------------------------------------- */

   rc = torus_alltoallw((char *)sendbuf + sdt_true_lb,
                        comm_ptr->dcmf.sndlen,
                        comm_ptr->dcmf.sdispls,
                        (char *)recvbuf + rdt_true_lb,
                        comm_ptr->dcmf.rcvlen,
                        comm_ptr->dcmf.rdispls,
                        comm_ptr->dcmf.sndcounters,
                        comm_ptr->dcmf.rcvcounters,
                        &comm_ptr->dcmf.geometry);


   if(tsndlen) free(tsndlen);
   if(trcvlen) free(trcvlen);
  return rc;
}
