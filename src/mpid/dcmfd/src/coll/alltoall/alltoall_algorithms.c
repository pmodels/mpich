/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/alltoall/alltoall_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
/**
 * **************************************************************************
 * \brief "Done" callback for collective alltoall message.
 * **************************************************************************
 */

static void
alltoall_cb_done(void * clientdata, DCMF_Error_t *err)
{
  volatile unsigned * work_left = (unsigned *) clientdata;
  * work_left = 0;
  MPID_Progress_signal();
}

int
MPIDO_Alltoall_torus(void * sendbuf,
		     int sendcount,
		     MPI_Datatype sendtype,
		     void * recvbuf,
		     int recvcount,
		     MPI_Datatype recvtype,
		     MPID_Comm * comm)
{
  int rc;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { alltoall_cb_done, (void *) &active };

  unsigned * sndlen = comm->dcmf.sndlen;
  unsigned * sdispls = comm->dcmf.sdispls;
  unsigned * rcvlen = comm->dcmf.rcvlen;
  unsigned * rdispls = comm->dcmf.rdispls;
  unsigned * sndcounters = comm->dcmf.sndcounters;
  unsigned * rcvcounters = comm->dcmf.rcvcounters;

  /* uses the alltoallv protocol */
  rc = DCMF_Alltoallv(&MPIDI_CollectiveProtocols.torus_alltoallv,
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

#endif /* USE_CCMI_COLL */

int
MPIDO_Alltoall_simple(void * send_buff, int send_count,
		      MPI_Datatype send_type, void * recv_buff,
		      int recv_count, MPI_Datatype recv_type,
		      MPID_Comm * comm)
{
  int mpi_errno = MPI_SUCCESS;
  int i, rank, np, nreqs, partner;
  char * send_ptr;
  char * recv_ptr;
  MPI_Aint sndinc;
  MPI_Aint rcvinc;
  MPI_Request * req, * req_ptr;

  rank = comm->rank;
  np = comm->local_size;

  MPIR_Type_extent_impl(send_type, &sndinc);
  MPIR_Type_extent_impl(recv_type, &rcvinc);

  sndinc *= send_count;
  rcvinc *= recv_count;

  /* Allocate arrays of requests. */

  nreqs = 2 * (np - 1);
  req = (MPI_Request *) MPIU_Malloc(nreqs * sizeof(MPI_Request));
  req_ptr = req;

  if (!req)
  {
    if (rank == 0)
      printf("cannot allocate memory\n");
    PMPI_Abort(comm->handle, 0);
  }

  /* simple optimization */
  send_ptr = ((char *) send_buff) + (rank * sndinc);
  recv_ptr = ((char *) recv_buff) + (rank * rcvinc);
  memcpy(recv_ptr, send_ptr, sndinc);

  recv_ptr = (char *) recv_buff;
  send_ptr = (char *) send_buff;

  for (i = 0; i < np; i++)
  {
    partner = (rank + i) % np;
    if (partner == rank) continue;

    mpi_errno = MPIC_Irecv(recv_ptr + (partner * rcvinc), recv_count, recv_type, partner,
                           MPIR_ALLTOALL_TAG, comm->handle, req_ptr++);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    mpi_errno = MPIC_Isend(send_ptr + (partner * sndinc), send_count, send_type, partner,
                           MPIR_ALLTOALL_TAG, comm->handle, req_ptr++);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  }

  mpi_errno = MPIR_Waitall_impl(nreqs, req, MPI_STATUSES_IGNORE);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);

  if (req)
    MPIU_Free(req);

 fn_exit:
  return MPI_SUCCESS;
 fn_fail:
  goto fn_exit;
}

