/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoallv/mpido_alltoallv.c
 * \brief ???
 */

#include "mpido_coll.h"
#include "mpidi_coll_prototypes.h"


#ifdef USE_CCMI_COLL

#pragma weak PMPIDO_Alltoallv = MPIDO_Alltoallv

int
MPIDO_Alltoallv(void *sendbuf,
                int *sendcounts,
                int *senddispls,
                MPI_Datatype sendtype,
                void *recvbuf,
                int *recvcounts,
                int *recvdispls,
                MPI_Datatype recvtype,
                MPID_Comm *comm)
{
  MPIDO_Embedded_Info_Set * properties = &(comm->dcmf.properties);
  char *sbuf = sendbuf, *rbuf = recvbuf;
  int numprocs = comm->local_size;
  int tsndlen, trcvlen, snd_contig, rcv_contig, rc ,i;
  MPI_Aint sdt_true_lb, rdt_true_lb;
  MPID_Datatype *dt_null = NULL;
  
  MPIDI_Datatype_get_info(1, sendtype, snd_contig, tsndlen,
			  dt_null, sdt_true_lb);
  MPIDI_Datatype_get_info(1, recvtype, rcv_contig, trcvlen,
			  dt_null, rdt_true_lb);

  MPIDI_VerifyBuffer(sendbuf, sbuf, sdt_true_lb);
  MPIDI_VerifyBuffer(recvbuf, rbuf, rdt_true_lb);
  
  
  if (MPIDO_INFO_ISSET(properties, MPIDO_USE_MPICH_ALLTOALLV) ||
      !MPIDO_INFO_ISSET(properties, MPIDO_USE_TORUS_ALLTOALLV) ||
      !snd_contig ||
      !rcv_contig ||
      tsndlen != trcvlen)
  {
    comm->dcmf.last_algorithm = MPIDO_USE_MPICH_ALLTOALLV;
    return MPIR_Alltoallv_intra(sendbuf, sendcounts, senddispls, sendtype,
                                recvbuf, recvcounts, recvdispls, recvtype,
                                comm);
  }
  if (!MPIDO_AllocateAlltoallBuffers(comm))
    return MPIR_Err_create_code(MPI_SUCCESS,
				MPIR_ERR_RECOVERABLE,
				"MPI_Alltoallv",
				__LINE__, MPI_ERR_OTHER, "**nomem", 0);

  /* ---------------------------------------------- */
  /* Initialize the send buffers and lengths        */
  /* pktInject is the number of packets to inject   */
  /* per advance loop.  The best performance is 2   */
  /* ---------------------------------------------- */
  for (i = 0; i < numprocs; i++)
  {
    comm->dcmf.sndlen [i] = tsndlen * sendcounts[i];
    comm->dcmf.sdispls[i] = tsndlen * senddispls[i];
    comm->dcmf.rcvlen [i] = trcvlen * recvcounts[i];
    comm->dcmf.rdispls[i] = trcvlen * recvdispls[i];
  }


  /* ---------------------------------------------- */
  /* Create a message layer collective message      */
  /* ---------------------------------------------- */
  comm->dcmf.last_algorithm = MPIDO_USE_TORUS_ALLTOALLV;
  rc = MPIDO_Alltoallv_torus(sbuf,
                             sendcounts,
			     senddispls,
			     sendtype,
			     rbuf,
			     recvcounts,
			     recvdispls,
			     recvtype,
			     comm);
  return rc;
}

#else /* !USE_CCMI_COLL */

int MPIDO_Alltoallv(void *sendbuf,
                    int *sendcounts,
                    int *senddispls,
                    MPI_Datatype sendtype,
                    void *recvbuf,
                    int *recvcounts,
                    int *recvdispls,
                    MPI_Datatype recvtype,
                    MPID_Comm *comm)
{
  MPID_abort();
}
#endif /* !USE_CCMI_COLL */
