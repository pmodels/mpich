/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/allgatherv/allgatherv_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
/* \brief Callback for async bcast. MPIDO_Bcast call wouldn't be appropriate
 * here, so we just use call DCMF_AsyncBroadcast directly
 */

static void allgatherv_async_done(void *clientdata, DCMF_Error_t *err)
{
  volatile unsigned *work_left = (unsigned *)clientdata;
  (*work_left)--;
  MPID_Progress_signal();
}

int MPIDO_Allgatherv_bcast_binom_async(void *sendbuf,
				       int sendcount,
				       MPI_Datatype sendtype,
				       void *recvbuf,
				       int *recvcounts,
				       int buffer_sum, 
				       int *displs,
				       MPI_Datatype recvtype,
				       MPI_Aint send_true_lb,
				       MPI_Aint recv_true_lb,
				       size_t send_size,
				       size_t recv_size,
				       MPID_Comm * comm_ptr)
{
  int i, j, max_size, left_over;
  int dt_contig, data_sz;
  MPID_Datatype *dt_ptr;
  MPI_Aint extent, dt_true_lb;
  volatile unsigned active = 0;
   
  int numrequests = MPIDI_CollectiveProtocols.numrequests;
  DCMF_CollectiveRequest_t *requests;
  DCMF_Callback_t callback = {allgatherv_async_done, (void *)&active};
  DCMF_CollectiveProtocol_t * protocol =
    &MPIDI_CollectiveProtocols.async_binomial_bcast;

  void *destbuf;
   
  MPID_Datatype_get_extent_macro(recvtype, extent);
  MPIDI_Datatype_get_info(1,
                          recvtype,
                          dt_contig,
                          data_sz,
                          dt_ptr,
                          dt_true_lb);
      
  MPID_Ensure_Aint_fits_in_pointer (
                                    (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     displs[comm_ptr->rank] * extent));
   
  if(sendbuf != MPI_IN_PLACE)
  {
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   recvbuf + displs[comm_ptr->rank] * extent,
                   recvcounts[comm_ptr->rank],
                   recvtype);
  }
   
  requests = (DCMF_CollectiveRequest_t *)malloc(numrequests *
                                                sizeof(DCMF_CollectiveRequest_t));
   
  max_size = ((int)comm_ptr->local_size/numrequests)*numrequests;
  left_over = comm_ptr->local_size - max_size;
   
  for(i=0;i<max_size ;i+=numrequests)
  {
    active = numrequests;
    for(j=0;j<numrequests;j++)
    {
      destbuf = recvbuf + displs[i+j] * extent;
      if(recvcounts[i+j])
        DCMF_AsyncBroadcast(protocol,
                            &requests[j],
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            &comm_ptr->dcmf.geometry,
                            comm_ptr->vcr[i+j],
                            destbuf,
                            data_sz * recvcounts[i+j]);
      else 
        active--;
    }
    MPID_PROGRESS_WAIT_WHILE(active);
  }

  if(left_over)
  {
    active = left_over;
    for(i=0;i<left_over;i++)
    {
      destbuf = recvbuf + displs[max_size + i] * extent;
      if(recvcounts[max_size+i])
        DCMF_AsyncBroadcast(protocol,
                            &requests[i],
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            &comm_ptr->dcmf.geometry,
                            comm_ptr->vcr[max_size+i],
                            destbuf,
                            data_sz * recvcounts[max_size+i]);
      else
        active--;
    }
    MPID_PROGRESS_WAIT_WHILE(active);
  }

  free(requests);

  return MPI_SUCCESS;
  
}

int MPIDO_Allgatherv_bcast_rect_async(void *sendbuf,
				      int sendcount,
				      MPI_Datatype sendtype,
				      void *recvbuf,
				      int *recvcounts,
				      int buffer_sum, 
				      int *displs,
				      MPI_Datatype recvtype,
				      MPI_Aint send_true_lb,
				      MPI_Aint recv_true_lb,
				      size_t send_size,
				      size_t recv_size,
				      MPID_Comm * comm_ptr)
{

  int i, j, max_size, left_over;
  int dt_contig, data_sz;
  MPID_Datatype *dt_ptr;
  MPI_Aint extent, dt_true_lb;
  volatile unsigned active = 0;
   
  int numrequests = MPIDI_CollectiveProtocols.numrequests;
  DCMF_CollectiveRequest_t *requests;
  DCMF_Callback_t callback = {allgatherv_async_done, (void *)&active};
  DCMF_CollectiveProtocol_t * protocol =
    &MPIDI_CollectiveProtocols.async_rectangle_bcast;

  void *destbuf;
   
  MPID_Datatype_get_extent_macro(recvtype, extent);
  MPIDI_Datatype_get_info(1,
                          recvtype,
                          dt_contig,
                          data_sz,
                          dt_ptr,
                          dt_true_lb);
      
  MPID_Ensure_Aint_fits_in_pointer (
                                    (MPI_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                     displs[comm_ptr->rank] * extent));
   
  if(sendbuf != MPI_IN_PLACE)
  {
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   recvbuf + displs[comm_ptr->rank] * extent,
                   recvcounts[comm_ptr->rank],
                   recvtype);
  }
   
  requests = (DCMF_CollectiveRequest_t *)malloc(numrequests *
                                                sizeof(DCMF_CollectiveRequest_t));
   
  max_size = ((int)comm_ptr->local_size/numrequests)*numrequests;
  left_over = comm_ptr->local_size - max_size;
   
  for(i=0;i<max_size ;i+=numrequests)
  {
    active = numrequests;
    for(j=0;j<numrequests;j++)
    {
      destbuf = recvbuf + displs[i+j] * extent;
      if(recvcounts[i+j])
        DCMF_AsyncBroadcast(protocol,
                            &requests[j],
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            &comm_ptr->dcmf.geometry,
                            comm_ptr->vcr[i+j],
                            destbuf,
                            data_sz * recvcounts[i+j]);
      else 
        active--;
    }
    MPID_PROGRESS_WAIT_WHILE(active);
  }

  if(left_over)
  {
    active = left_over;
    for(i=0;i<left_over;i++)
    {
      destbuf = recvbuf + displs[max_size + i] * extent;
      if(recvcounts[max_size+i])
        DCMF_AsyncBroadcast(protocol,
                            &requests[i],
                            callback,
                            DCMF_MATCH_CONSISTENCY,
                            &comm_ptr->dcmf.geometry,
                            comm_ptr->vcr[max_size+i],
                            destbuf,
                            data_sz * recvcounts[max_size+i]);
      else
        active--;
    }
    MPID_PROGRESS_WAIT_WHILE(active);
  }

  free(requests);

  return MPI_SUCCESS;
  
}

#endif /* USE_CCMI_COLL */

/* ****************************************************************** */
/**
 * \brief Use (tree) MPIDO_Allreduce() to do a fast Allgatherv operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - The recv buffer is continuous
 *       - Tree allreduce is availible (for max performance)
 */
/* ****************************************************************** */
int MPIDO_Allgatherv_allreduce(void *sendbuf,
			       int sendcount,
			       MPI_Datatype sendtype,
			       void *recvbuf,
			       int *recvcounts,
			       int buffer_sum, 
			       int *displs,
			       MPI_Datatype recvtype,
			       MPI_Aint send_true_lb,
			       MPI_Aint recv_true_lb,
			       size_t send_size,
			       size_t recv_size,
			       MPID_Comm * comm_ptr)
{
  int start, rc;
  int length;
  char *startbuf = NULL;
  char *destbuf = NULL;
  
  startbuf = (char *) recvbuf + recv_true_lb;
  destbuf = startbuf + displs[comm_ptr->rank] * recv_size;
  
  start = 0;
  length = displs[comm_ptr->rank] * recv_size;
  memset(startbuf + start, 0, length);
  
  start  = (displs[comm_ptr->rank] + 
	    recvcounts[comm_ptr->rank]) * recv_size;
  length = buffer_sum - (displs[comm_ptr->rank] + 
			 recvcounts[comm_ptr->rank]) * recv_size;
  memset(startbuf + start, 0, length);
  
  if (sendbuf != MPI_IN_PLACE)
  {
    char *outputbuf = (char *) sendbuf + send_true_lb;
    memcpy(destbuf, outputbuf, send_size);
  }
  
  //if (0==comm_ptr->rank) puts("allreduce allgatherv");

  rc = MPIDO_Allreduce(MPI_IN_PLACE,
		       startbuf,
		       buffer_sum/sizeof(int),
		       MPI_INT,
		       MPI_BOR,
		       comm_ptr);
  
  return rc;
}

/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Bcast() to do a fast Allgatherv operation
 *
 * \note This function requires one of these (for max performance):
 *       - Tree broadcast
 *       - Rect broadcast
 *       ? Binomial broadcast
 */
/* ****************************************************************** */
int MPIDO_Allgatherv_bcast(void *sendbuf,
			   int sendcount,
			   MPI_Datatype sendtype,
			   void *recvbuf,
			   int *recvcounts,
			   int buffer_sum, 
			   int *displs,
			   MPI_Datatype recvtype,
			   MPI_Aint send_true_lb,
			   MPI_Aint recv_true_lb,
			   size_t send_size,
			   size_t recv_size,
			   MPID_Comm * comm_ptr)
{
  int i, rc=MPI_ERR_INTERN;
  MPI_Aint extent;
  MPID_Datatype_get_extent_macro(recvtype, extent);

  if (sendbuf != MPI_IN_PLACE)
  {
    void *destbuffer = recvbuf + displs[comm_ptr->rank] * extent;
    MPIR_Localcopy(sendbuf,
                   sendcount,
                   sendtype,
                   destbuffer,
                   recvcounts[comm_ptr->rank],
                   recvtype);
  }
  
  for (i = 0; i < comm_ptr->local_size; i++)
  {
    void *destbuffer = recvbuf + displs[i] * extent;
    rc = MPIDO_Bcast(destbuffer,
                     recvcounts[i],
                     recvtype,
                     i,
                     comm_ptr);
  }
  //if (0==comm_ptr->rank) puts("bcast allgatherv");

  return rc;
}

/* ****************************************************************** */
/**
 * \brief Use (tree/rect) MPIDO_Alltoall() to do a fast Allgatherv operation
 *
 * \note This function requires that:
 *       - The send/recv data types are contiguous
 *       - DMA alltoallv is availible (for max performance)
 */
/* ****************************************************************** */

int MPIDO_Allgatherv_alltoall(void *sendbuf,
			      int sendcount,
			      MPI_Datatype sendtype,
			      void *recvbuf,
			      int *recvcounts,
			      int buffer_sum, 
			      int *displs,
			      MPI_Datatype recvtype,
			      MPI_Aint send_true_lb,
			      MPI_Aint recv_true_lb,
			      size_t send_size,
			      size_t recv_size,
			      MPID_Comm * comm_ptr)
{
  size_t total_send_size;
  char *startbuf;
  char *destbuf;
  int i, rc;
  int my_recvcounts = -1;
  void *a2a_sendbuf = NULL;
  int a2a_sendcounts[comm_ptr->local_size];
  int a2a_senddispls[comm_ptr->local_size];
  
  total_send_size = recvcounts[comm_ptr->rank] * recv_size;
  for (i = 0; i < comm_ptr->local_size; ++i)
  {
    a2a_sendcounts[i] = total_send_size;
    a2a_senddispls[i] = 0;
  }
  if (sendbuf != MPI_IN_PLACE)
  {
    a2a_sendbuf = sendbuf + send_true_lb;
  }
  else
  {
    startbuf = (char *) recvbuf + recv_true_lb;
    destbuf = startbuf + displs[comm_ptr->rank] * recv_size;
    a2a_sendbuf = destbuf;
    a2a_sendcounts[comm_ptr->rank] = 0;
    my_recvcounts = recvcounts[comm_ptr->rank];
    recvcounts[comm_ptr->rank] = 0;
  }
  
  //if (0==comm_ptr->rank) puts("all2all allgatherv");
  rc = MPIDO_Alltoallv(a2a_sendbuf,
		       a2a_sendcounts,
		       a2a_senddispls,
		       MPI_CHAR,
		       recvbuf,
		       recvcounts,
		       displs,
		       recvtype,
		       comm_ptr);
  if (sendbuf == MPI_IN_PLACE)
    recvcounts[comm_ptr->rank] = my_recvcounts;

  return rc;
}
