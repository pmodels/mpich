/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allgather/allgather_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
static void allgather_async_done(void *clientdata, DCMF_Error_t *err)
{
  volatile unsigned *work_left = (unsigned *)clientdata;
  (*work_left)--;
  MPID_Progress_signal();
  return;
}

/* ****************************************************************** */
/**
 * \brief Use (torus) Asyncbcast_rect() to do a fast Allgather operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - a fast bcast is availible (for max performance)
 *       - The datatype parameters needed added to the function signature
 */
/* ****************************************************************** */

int MPIDO_Allgather_bcast_rect_async(void *sendbuf,
                                     int sendcount,
                                     MPI_Datatype sendtype,
                                     void *recvbuf,
                                     int recvcount,
                                     MPI_Datatype recvtype,
                                     MPI_Aint send_true_lb,
                                     MPI_Aint recv_true_lb,
                                     size_t send_size,
                                     size_t recv_size,
                                     MPID_Comm *comm_ptr)
{
  MPID_Datatype *dt_ptr;
  int i, j, np, max_size, left_over, data_sz, dt_contig;
  MPI_Aint extent, dt_true_lb;
  volatile unsigned active = 0;

  int numrequests = MPIDI_CollectiveProtocols.numrequests;
  DCMF_CollectiveRequest_t *requests;
  DCMF_Callback_t callback = {allgather_async_done, (void *)&active};
  DCMF_CollectiveProtocol_t *protocol =
    &(MPIDI_CollectiveProtocols.async_rectangle_bcast);

  void *destbuf;

  np = comm_ptr->local_size;
   
  MPID_Datatype_get_extent_macro(recvtype, extent);

  MPIDI_Datatype_get_info(recvcount,
                          recvtype,
                          dt_contig,
                          data_sz,
                          dt_ptr,
                          dt_true_lb);
   
  MPID_Ensure_Aint_fits_in_pointer ((MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + 
                                     np * recvcount * extent));

  if(sendbuf != MPI_IN_PLACE)
  {
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   recvbuf + comm_ptr->rank * recvcount * extent,
                   recvcount,
                   recvtype);
  }

  requests = (DCMF_CollectiveRequest_t *)MPIU_Malloc(numrequests *
                                                     sizeof(DCMF_CollectiveRequest_t));

  max_size = ((int) np / numrequests) * numrequests;
  left_over = np - max_size;

  for(i=0;i<max_size;i+=numrequests)
  {
    active = numrequests;
    for(j=0;j<numrequests;j++)
    {
      destbuf = recvbuf + (i+j)*recvcount*extent;
      DCMF_AsyncBroadcast(protocol,
                          &requests[j],
                          callback,
                          DCMF_MATCH_CONSISTENCY,
                          &comm_ptr->dcmf.geometry,
                          comm_ptr->vcr[i+j],
                          destbuf,
                          data_sz);
    }

    MPID_PROGRESS_WAIT_WHILE(active);
  }


  if(left_over)
  {
    active = left_over;
    for(i=0;i<left_over;i++)
    {
      destbuf = recvbuf + (max_size+i)*recvcount*extent;
      DCMF_AsyncBroadcast(protocol,
                          &requests[i],
                          callback,
                          DCMF_MATCH_CONSISTENCY,
                          &comm_ptr->dcmf.geometry,
                          comm_ptr->vcr[max_size+i],
                          destbuf,
                          data_sz);
    }

    MPID_PROGRESS_WAIT_WHILE(active);
  }
         
  MPIU_Free(requests);
         
  return MPI_SUCCESS;

}
         

/* ****************************************************************** */
/**
 * \brief Use (torus) Asyncbcast_binom() to do a fast Allgather operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - a fast bcast is availible (for max performance)
 *       - The datatype parameters needed added to the function signature
 */
/* ****************************************************************** */

int MPIDO_Allgather_bcast_binom_async(void *sendbuf,
                                      int sendcount,
                                      MPI_Datatype sendtype,
                                      void *recvbuf,
                                      int recvcount,
                                      MPI_Datatype recvtype,
                                      MPI_Aint send_true_lb,
                                      MPI_Aint recv_true_lb,
                                      size_t send_size,
                                      size_t recv_size,
                                      MPID_Comm *comm_ptr)
{
  MPID_Datatype *dt_ptr;
  int i, j, np, max_size, left_over, data_sz, dt_contig;
  MPI_Aint extent, dt_true_lb;
  volatile unsigned active = 0;

  int numrequests = MPIDI_CollectiveProtocols.numrequests;
  DCMF_CollectiveRequest_t *requests;
  DCMF_Callback_t callback = {allgather_async_done, (void *)&active};
  DCMF_CollectiveProtocol_t *protocol =
    &(MPIDI_CollectiveProtocols.async_binomial_bcast);

  void *destbuf;

  np = comm_ptr->local_size;
  MPID_Datatype_get_extent_macro(recvtype, extent);

  MPIDI_Datatype_get_info(recvcount,
                          recvtype,
                          dt_contig,
                          data_sz,
                          dt_ptr,
                          dt_true_lb);

  MPID_Ensure_Aint_fits_in_pointer ((MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + 
                                     np * recvcount * extent));

  if(sendbuf != MPI_IN_PLACE)
  {
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   recvbuf + comm_ptr->rank * recvcount * extent,
                   recvcount,
                   recvtype);
  }

  requests = (DCMF_CollectiveRequest_t *)MPIU_Malloc(numrequests *
                                                     sizeof(DCMF_CollectiveRequest_t));

  max_size = ((int)np/numrequests)*numrequests;
  left_over = np - max_size;

  for(i=0;i<max_size;i+=numrequests)
  {
    active = numrequests;
    for(j=0;j<numrequests;j++)
    {
      destbuf = recvbuf + (i+j)*recvcount*extent;
      DCMF_AsyncBroadcast(protocol,
                          &requests[j],
                          callback,
                          DCMF_MATCH_CONSISTENCY,
                          &comm_ptr->dcmf.geometry,
                          comm_ptr->vcr[i+j],
                          destbuf,
                          data_sz);
    }

    MPID_PROGRESS_WAIT_WHILE(active);
  }


  if(left_over)
  {
    active = left_over;
    for(i=0;i<left_over;i++)
    {
      destbuf = recvbuf + (max_size+i)*recvcount*extent;
      DCMF_AsyncBroadcast(protocol,
                          &requests[i],
                          callback,
                          DCMF_MATCH_CONSISTENCY,
                          &comm_ptr->dcmf.geometry,
                          comm_ptr->vcr[max_size+i],
                          destbuf,
                          data_sz);
    }

    MPID_PROGRESS_WAIT_WHILE(active);
  }
         
  MPIU_Free(requests);
         
  return MPI_SUCCESS;

}

#endif /* USE_CCMI_COLL */


/* ****************************************************************** */
/**
 * \brief Use (tree) MPIDO_Allreduce() to do a fast Allgather operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - Tree allreduce is availible (for max performance)
 *       - The datatype parameters needed added to the function signature
 */
/* ****************************************************************** */
int MPIDO_Allgather_allreduce(void *sendbuf,
			      int sendcount,
			      MPI_Datatype sendtype,
			      void *recvbuf,
			      int recvcount,
			      MPI_Datatype recvtype,
			      MPI_Aint send_true_lb,
			      MPI_Aint recv_true_lb,
			      size_t send_size,
			      size_t recv_size,
			      MPID_Comm * comm)
  
{
  int rc, rank;
  char *startbuf = NULL;
  char *destbuf = NULL;

  rank = comm->rank;

  startbuf   = (char *) recvbuf + recv_true_lb;
  destbuf    = startbuf + rank * send_size;

  memset(startbuf, 0, rank * send_size);
  memset(destbuf + send_size, 0, recv_size - (rank + 1) * send_size);

  if (sendbuf != MPI_IN_PLACE)
  {
    char *outputbuf = (char *) sendbuf + send_true_lb;
    memcpy(destbuf, outputbuf, send_size);
  }

  rc = MPIDO_Allreduce(MPI_IN_PLACE,
		       startbuf,
		       recv_size/sizeof(int),
		       MPI_INT,
		       MPI_BOR,
		       comm);

  return rc;
}


/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Bcast() to do a fast Allgather operation
 *
 * \note This function requires one of these (for max performance):
 *       - Tree broadcast
 */
/* ****************************************************************** */
int MPIDO_Allgather_bcast(void *sendbuf,
			  int sendcount,
			  MPI_Datatype sendtype,
			  void *recvbuf,
			  int recvcount,
			  MPI_Datatype recvtype,
			  MPI_Aint send_true_lb,
			  MPI_Aint recv_true_lb,
			  size_t send_size,
			  size_t recv_size,
			  MPID_Comm * comm)
{
  int i, np, rc = 0;
  MPI_Aint extent;

  np = comm ->local_size;
  MPID_Datatype_get_extent_macro(recvtype, extent);

  MPID_Ensure_Aint_fits_in_pointer ((MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf + 
				     np * recvcount * extent));
  if (sendbuf != MPI_IN_PLACE)
  {
    void *destbuf = recvbuf + comm->rank * recvcount * extent;
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   destbuf,
                   recvcount,
                   recvtype);
  }

/* this code should either abort on first error or somehow aggregate 
 * error codes, esp since it calls internal routines */
  for (i = 0; i < np; i++)
  {
    void *destbuf = recvbuf + i * recvcount * extent;
    rc = MPIDO_Bcast(destbuf,
                     recvcount,
                     recvtype,
                     i,
                     comm);
  }

  return rc;
}

/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Alltoall() to do a fast Allgather operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - DMA alltoallv is availible (for max performance)
 *       - The datatype parameters needed added to the function signature
 */
/* ****************************************************************** */
int MPIDO_Allgather_alltoall(void *sendbuf,
			     int sendcount,
			     MPI_Datatype sendtype,
			     void *recvbuf,
			     int recvcount,
			     MPI_Datatype recvtype,
			     MPI_Aint send_true_lb,
			     MPI_Aint recv_true_lb,
			     size_t send_size,
			     size_t recv_size,
			     MPID_Comm * comm)
{
  int i, rc;
  void *a2a_sendbuf = NULL;
  char *destbuf=NULL;
  char *startbuf=NULL;

  int a2a_sendcounts[comm->local_size];
  int a2a_senddispls[comm->local_size];
  int a2a_recvcounts[comm->local_size];
  int a2a_recvdispls[comm->local_size];

  for (i = 0; i < comm->local_size; ++i)
  {
    a2a_sendcounts[i] = send_size;
    a2a_senddispls[i] = 0;
    a2a_recvcounts[i] = recvcount;
    a2a_recvdispls[i] = recvcount * i;
  }
  if (sendbuf != MPI_IN_PLACE)
  {
    a2a_sendbuf = sendbuf + send_true_lb;
  }
  else
  {
    startbuf = (char *) recvbuf + recv_true_lb;
    destbuf = startbuf + comm->rank * send_size;
    a2a_sendbuf = destbuf;
    a2a_sendcounts[comm->rank] = 0;

    a2a_recvcounts[comm->rank] = 0;
  }

  rc = MPIDO_Alltoallv(a2a_sendbuf,
		       a2a_sendcounts,
		       a2a_senddispls,
		       MPI_CHAR,
		       recvbuf,
		       a2a_recvcounts,
		       a2a_recvdispls,
		       recvtype,
		       comm);

  return rc;
}
