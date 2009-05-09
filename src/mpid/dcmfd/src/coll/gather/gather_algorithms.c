/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/gather/gather_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

int MPIDO_Gather_reduce(void * sendbuf,
			int sendcount,
			MPI_Datatype sendtype,
			void *recvbuf,
			int recvcount,
			MPI_Datatype recvtype,
			int root,
			MPID_Comm * comm)
{
  MPID_Datatype * data_ptr;
  MPI_Aint true_lb;
  int rank = comm->rank;
  int size = comm->local_size;
  int rc, sbytes, rbytes, contig;
  char *tempbuf = NULL;
  char *inplacetemp = NULL;
  
  MPIDI_Datatype_get_info(sendcount, sendtype, contig,
			  sbytes, data_ptr, true_lb);

  MPIDI_Datatype_get_info(recvcount, recvtype, contig,
			  rbytes, data_ptr, true_lb);
  
  if(rank == root)
  {
    tempbuf = recvbuf;
    /* zero the buffer if we aren't in place */
    if(sendbuf != MPI_IN_PLACE)
    {
      memset(tempbuf, 0, sbytes * size * sizeof(char));
      memcpy(tempbuf+(rank * sbytes), sendbuf, sbytes);
    }
    /* copy our contribution temporarily, zero the buffer, put it back */
    /* this will likely suck */
    else
    {
      inplacetemp = MPIU_Malloc(rbytes * sizeof(char));
      memcpy(inplacetemp, recvbuf+(rank * rbytes), rbytes);
      memset(tempbuf, 0, sbytes * size * sizeof(char));
      memcpy(tempbuf+(rank * rbytes), inplacetemp, rbytes);
      MPIU_Free(inplacetemp);
    }
  }
  /* everyone might need to speciifcally allocate a tempbuf, or 
   * we might need to make sure we don't end up at mpich in the
   * mpido_() functions - seems to be a problem?
   */
   
  /* If we aren't root, malloc tempbuf and zero it, 
   * then copy our contribution to the right spot in the big buffer */
  else
  {
    tempbuf = MPIU_Malloc(sbytes * size * sizeof(char));
    if(!tempbuf)
      return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, 
                                  "MPI_Gather", __LINE__, MPI_ERR_OTHER,
                                  "**nomem", 0);
     
    memset(tempbuf, 0, sbytes * size * sizeof(char));
    memcpy(tempbuf+(rank*sbytes), sendbuf, sbytes);
  }
   
  rc = MPIDO_Reduce(MPI_IN_PLACE, 
                    tempbuf, 
                    (sbytes * size)/4, 
                    MPI_INT, 
                    MPI_BOR, 
                    root, 
                    comm);
   
  if(rank != root)
    MPIU_Free(tempbuf);
   
  return rc;
}
