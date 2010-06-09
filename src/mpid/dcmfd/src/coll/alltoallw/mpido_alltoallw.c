/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoallw/mpido_alltoallw.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Alltoallw = MPIDO_Alltoallw

int
MPIDO_Alltoallw(void *sendbuf,
                int *sendcounts,
                int *senddispls,
                MPI_Datatype *sendtypes,
                void *recvbuf,
                int *recvcounts,
                int *recvdispls,
                MPI_Datatype *recvtypes,
                MPID_Comm *comm)
{
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  int numprocs = comm->local_size;
  int *tsndlen, *trcvlen, snd_contig, rcv_contig, rc ,i;
  MPI_Aint sdt_true_lb=0, rdt_true_lb=0;
  MPID_Datatype *dt_null = NULL;
  char *sbuf = sendbuf, *rbuf = recvbuf;
  
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

    MPIDI_VerifyBuffer(sendbuf, sbuf, sdt_true_lb);
    MPIDI_VerifyBuffer(recvbuf, rbuf, rdt_true_lb);


    if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_ALLTOALLW) ||
        !MPIDO_INFO_ISSET(properties, MPIDO_USE_TORUS_ALLTOALLW) ||
        !snd_contig ||
        !rcv_contig ||
        tsndlen[i] != trcvlen[i])
    {
      if(tsndlen) MPIU_Free(tsndlen);
      if(trcvlen) MPIU_Free(trcvlen);

      comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLTOALLW;
      return MPIR_Alltoallw_intra(sendbuf, sendcounts, senddispls, sendtypes,
                                  recvbuf, recvcounts, recvdispls, recvtypes,
                                  comm);
    }
  }
  
  if (!MPIDO_AllocateAlltoallBuffers(comm))
    return MPIR_Err_create_code(MPI_SUCCESS,
				MPIR_ERR_RECOVERABLE,
				"MPI_Alltoallw",
				__LINE__, MPI_ERR_OTHER, "**nomem", 0);
      
  /* ---------------------------------------------- */
  /* Initialize the send buffers and lengths        */
  /* pktInject is the number of packets to inject   */
  /* per advance loop.  The best performance is 2   */
  /* ---------------------------------------------- */
  for (i = 0; i < numprocs; i++)
  {
    comm->dcmf.sndlen [i] = tsndlen[i] * sendcounts[i];
    comm->dcmf.sdispls[i] = senddispls[i];

    comm->dcmf.rcvlen [i] = trcvlen[i] * recvcounts[i];
    comm->dcmf.rdispls[i] = recvdispls[i];
  }


  /* ---------------------------------------------- */
  /* Create a message layer collective message      */
  /* ---------------------------------------------- */
  comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLTOALLW;

  rc = MPIDO_Alltoallw_torus(sbuf,
			     sendcounts,
			     senddispls,
			     sendtypes,
			     rbuf,
			     recvcounts,
			     recvdispls,
			     recvtypes,
			     comm);
  
  if(tsndlen) MPIU_Free(tsndlen);
  if(trcvlen) MPIU_Free(trcvlen);

  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Alltoallw(void *sendbuf,
                    int *sendcounts,
                    int *senddispls,
                    MPI_Datatype *sendtypes,
                    void *recvbuf,
                    int *recvcounts,
                    int *recvdispls,
                    MPI_Datatype *recvtypes,
                    MPID_Comm *comm)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */
