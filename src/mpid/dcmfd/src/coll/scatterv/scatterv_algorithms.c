/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/scatterv/scatterv_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"


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
                         MPID_Comm *comm_ptr)
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
    tempbuf = MPIU_Malloc(sizeof(char) * sum);
    if (!tempbuf)
      return MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE,
                                  "MPI_Scatterv", __LINE__, MPI_ERR_OTHER,
                                  "**nomem", 0);
  }
  else
    tempbuf = sendbuf;
  
  rc = MPIDO_Bcast(tempbuf, sum, sendtype, root, comm_ptr);

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
                             MPID_Comm * comm_ptr)
{
  int rank = comm_ptr->rank;
  int size = comm_ptr->local_size;

  int *sdispls, *scounts;
  int *rdispls, *rcounts;
  char *sbuf, *rbuf;
  int contig, rbytes, sbytes;

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
                                "MPI_Scatterv",
                                __LINE__, MPI_ERR_OTHER, "**nomem", 0);
  }
  //   memset(rbuf, 0, rbytes * size * sizeof(char));

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
                                  "MPI_Scatterv",
                                  __LINE__, MPI_ERR_OTHER, "**nomem", 0);
    }
    memset(sdispls, 0, size*sizeof(int));
    memset(scounts, 0, size*sizeof(int));
    //      memset(sbuf, 0, rbytes * sizeof(char));
  }

  rdispls = MPIU_Malloc(size * sizeof(int));
  rcounts = MPIU_Malloc(size * sizeof(int));
  if(!rdispls || !rcounts)
  {
    if(rdispls)
      MPIU_Free(rdispls);
    return MPIR_Err_create_code(MPI_SUCCESS,
                                MPIR_ERR_RECOVERABLE,
                                "MPI_Scatterv",
                                __LINE__, MPI_ERR_OTHER, "**nomem", 0);
  }

  memset(rdispls, 0, size*sizeof(unsigned));
  memset(rcounts, 0, size*sizeof(unsigned));

  rcounts[root] = rbytes;

  MPIDO_Alltoallv(sbuf,
                  scounts,
                  sdispls,
                  sendtype,
                  rbuf,
                  rcounts,
                  rdispls,
                  MPI_CHAR,
                  comm_ptr);

  if(rank == root && recvbuf == MPI_IN_PLACE)
  {
    MPIU_Free(rbuf);
    MPIU_Free(rdispls);
    MPIU_Free(rcounts);
    return MPI_SUCCESS;
  }
  else
  {
    //      memcpy(recvbuf, rbuf+(root*rbytes), rbytes);
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

  return MPI_SUCCESS;
}

