/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatter/scatter_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

int MPIDO_Scatter_bcast(void * sendbuf,
			int sendcount,
			MPI_Datatype sendtype,
			void *recvbuf,
			int recvcount,
			MPI_Datatype recvtype,
			int root,
			MPID_Comm * comm)
{
  /* Pretty simple - bcast a temp buffer and copy our little chunk out */
  int contig, nbytes, rc;
  int rank = comm->rank;
  int size = comm->local_size;
  char *tempbuf = NULL;

  MPID_Datatype * dt_ptr;
  MPI_Aint true_lb = 0; 
  
  if(rank == root)
  {
    MPIDI_Datatype_get_info(sendcount,
                            sendtype,
                            contig,
                            nbytes,
                            dt_ptr,
                            true_lb);
    tempbuf = sendbuf;
  }
  else
  {
    MPIDI_Datatype_get_info(recvcount,
                            recvtype,
                            contig,
                            nbytes,
                            dt_ptr,
                            true_lb);

    tempbuf = MPIU_Malloc(nbytes * size);
    if(!tempbuf)
    {
      return MPIR_Err_create_code(MPI_SUCCESS,
                                  MPIR_ERR_RECOVERABLE,
                                  "MPI_Scatter",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
    }
  }
  
  rc = MPIDO_Bcast(tempbuf, nbytes*size, MPI_CHAR, root, comm);
  
  if(rank == root && recvbuf == MPI_IN_PLACE)
    return MPI_SUCCESS;
  else
    memcpy(recvbuf, tempbuf+(rank*nbytes), nbytes);
  
  if (rank!=root)
    MPIU_Free(tempbuf);

  return rc;
}
