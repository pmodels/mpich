/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/coll/bcast/bcast_algorithms.c
 * \brief ???
 */

#include "mpido_coll.h"

#ifdef USE_CCMI_COLL
/**
 * **************************************************************************
 * \brief "Done" callback for collective broadcast message.
 * **************************************************************************
 */

static void
bcast_cb_done(void *clientdata, DCMF_Error_t *err)
{
  volatile unsigned *work_left = (unsigned *) clientdata;
  *work_left = 0;
  MPID_Progress_signal();
}


int MPIDO_Bcast_CCMI_tree(void *buffer,
                           int bytes,
                           int root,
                           MPID_Comm *comm)
{
  if (!bytes) return MPI_SUCCESS;
  
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);

  hw_root = comm->vcr[root];

    rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.tree_bcast,
			&request,
			callback,
			DCMF_MATCH_CONSISTENCY,
			geometry,
			hw_root,
			buffer,
			bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}


int MPIDO_Bcast_CCMI_tree_dput(void *buffer,
                           int bytes,
                           int root,
                           MPID_Comm *comm)
{
  if (!bytes) return MPI_SUCCESS;
  
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);

  hw_root = comm->vcr[root];

    rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.tree_dput_bcast,
			&request,
			callback,
			DCMF_MATCH_CONSISTENCY,
			geometry,
			hw_root,
			buffer,
			bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}


int MPIDO_Bcast_tree(void * buffer,
		     int bytes,
		     int root,
		     MPID_Comm * comm)
{
  if (!bytes) return MPI_SUCCESS;
  
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };

  hw_root = comm->vcr[root];

    rc = DCMF_GlobalBcast(&MPIDI_Protocols.globalbcast,
			  (DCMF_Request_t *)&request,
			  callback,
			  DCMF_MATCH_CONSISTENCY,
			  hw_root,
			  buffer,
			  bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Bcast_binom_sync(void * buffer,
			   int bytes,
			   int root,
			   MPID_Comm * comm)
{
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };

  hw_root = comm->vcr[root];

  rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.binomial_bcast,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      hw_root,
                      buffer,
                      bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}


int MPIDO_Bcast_rect_dput(void * buffer,
			  int bytes,
			  int root,
			  MPID_Comm * comm)
{
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  hw_root = comm->vcr[root];

  rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.rectangle_bcast_dput,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      hw_root,
                      buffer,
                      bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Bcast_rect_singleth(void * buffer,
                              int bytes,
                              int root,
                              MPID_Comm * comm)
{
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  hw_root = comm->vcr[root];

  rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.rectangle_bcast_singleth,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      hw_root,
                      buffer,
                      bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Bcast_binom_singleth(void * buffer,
                               int bytes,
                               int root,
                               MPID_Comm * comm)
{
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  hw_root = comm->vcr[root];

  rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.binomial_bcast_singleth,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      hw_root,
                      buffer,
                      bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Bcast_rect_sync(void * buffer,
			  int bytes,
			  int root,
			  MPID_Comm * comm)
{
  int rc, hw_root;
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };
  hw_root = comm->vcr[root];

  rc = DCMF_Broadcast(&MPIDI_CollectiveProtocols.rectangle_bcast,
                      &request,
                      callback,
                      DCMF_MATCH_CONSISTENCY,
                      geometry,
                      hw_root,
                      buffer,
                      bytes);
  MPID_PROGRESS_WAIT_WHILE(active);
  return rc;
}

int MPIDO_Bcast_binom_async(void * buffer,
			    int bytes,
			    int root,
			    MPID_Comm * comm)
{
  int rc;
  unsigned int hw_root;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = {bcast_cb_done, (void *)&active };

  hw_root = comm->vcr[root];

  rc = DCMF_AsyncBroadcast(&MPIDI_CollectiveProtocols.async_binomial_bcast,
			   &request,
			   callback,
			   DCMF_MATCH_CONSISTENCY,
			   geometry,
			   hw_root,
			   buffer,
			   bytes);

  MPID_PROGRESS_WAIT_WHILE(active);

  return rc;
}

int MPIDO_Bcast_rect_async(void * buffer,
			   int bytes,
			   int root,
			   MPID_Comm * comm)
{
  int rc;
  unsigned int hw_root;
  DCMF_Geometry_t * geometry = &(comm->dcmf.geometry);
  DCMF_CollectiveRequest_t request;
  volatile unsigned active = 1;
  DCMF_Callback_t callback = { bcast_cb_done, (void *) &active };

  hw_root = comm->vcr[root];
  rc=DCMF_AsyncBroadcast(&MPIDI_CollectiveProtocols.async_rectangle_bcast,
                         &request,
                         callback,
                         DCMF_MATCH_CONSISTENCY,
                         geometry,
                         hw_root,
                         buffer,
                         bytes);

  MPID_PROGRESS_WAIT_WHILE(active);

  return rc;
}

#endif

int
MPIDO_Bcast_scatter_gather(void * buffer,
                           int bytes,
                           int root,
                           MPID_Comm * comm)
{
  MPI_Status status;

  int i, j, k, src, dst, rank, num_procs, send_offset, recv_offset;
  int mask, relative_rank, curr_size, recv_size, send_size;
  int scatter_size, tree_root, relative_dst, dst_tree_root;
  int my_tree_root, offset, tmp_mask, num_procs_completed;

  rank = comm->rank;
  num_procs = comm->local_size;

  scatter_size = (bytes + num_procs - 1) / num_procs;
  curr_size = (rank == root) ? bytes : 0;
  relative_rank = (rank >= root) ? rank - root : rank - root + num_procs;

  mask = 0x1;
  while (mask < num_procs)
  {
    if (relative_rank & mask)
    {
      src = rank - mask;
      if (src < 0) src += num_procs;
      recv_size = bytes - relative_rank * scatter_size;
      if (recv_size <= 0)
        curr_size = 0;
      else
      {
        MPIC_Recv(buffer + relative_rank * scatter_size, recv_size,
                  MPI_BYTE, src, MPIR_BCAST_TAG, comm->handle, &status);
        MPIR_Get_count_impl(&status, MPI_BYTE, &curr_size);
      }
      break;
    }
    mask <<= 1;
  }


  mask >>= 1;
  while (mask > 0)
  {
    if (relative_rank + mask < num_procs)
    {
      send_size = curr_size - scatter_size * mask;

      if (send_size > 0)
      {
        dst = rank + mask;
        if (dst >= num_procs) dst -= num_procs;
        MPIC_Send (buffer + scatter_size * (relative_rank + mask),
                   send_size, MPI_BYTE, dst, MPIR_BCAST_TAG,
                   comm->handle);

        curr_size -= send_size;
      }
    }
    mask >>= 1;
  }



  mask = 0x1;
  i = 0;
  while (mask < num_procs)
  {
    relative_dst = relative_rank ^ mask;

    dst = (relative_dst + root) % num_procs;

    /* find offset into send and recv buffers.
       zero out the least significant "i" bits of relative_rank and
       relative_dst to find root of src and dst
       subtrees. Use ranks of roots as index to send from
       and recv into  buffer */

    dst_tree_root = relative_dst >> i;
    dst_tree_root <<= i;

    my_tree_root = relative_rank >> i;
    my_tree_root <<= i;

    send_offset = my_tree_root * scatter_size;
    recv_offset = dst_tree_root * scatter_size;

    if (relative_dst < num_procs)
    {
      MPIC_Sendrecv(buffer + send_offset, curr_size, MPI_BYTE, dst,
                    MPIR_BCAST_TAG,
                    buffer + recv_offset, scatter_size * mask, MPI_BYTE,
                    dst, MPIR_BCAST_TAG, comm->handle, &status);
      MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
      curr_size += recv_size;
    }

    /* if some processes in this process's subtree in this step
       did not have any destination process to communicate with
       because of non-power-of-two, we need to send them the
       data that they would normally have received from those
       processes. That is, the haves in this subtree must send to
       the havenots. We use a logarithmic recursive-halfing algorithm
       for this. */

    if (dst_tree_root + mask > num_procs)
    {
      num_procs_completed = num_procs - my_tree_root - mask;
      /* num_procs_completed is the number of processes in this
         subtree that have all the data. Send data to others
         in a tree fashion. First find root of current tree
         that is being divided into two. k is the number of
         least-significant bits in this process's rank that
         must be zeroed out to find the rank of the root */
      j = mask;
      k = 0;
      while (j)
      {
        j >>= 1;
        k++;
      }
      k--;

      offset = scatter_size * (my_tree_root + mask);
      tmp_mask = mask >> 1;

      while (tmp_mask)
      {
        relative_dst = relative_rank ^ tmp_mask;
        dst = (relative_dst + root) % num_procs;

        tree_root = relative_rank >> k;
        tree_root <<= k;

        /* send only if this proc has data and destination
           doesn't have data. */

        if ((relative_dst > relative_rank)
            && (relative_rank < tree_root + num_procs_completed)
            && (relative_dst >= tree_root + num_procs_completed))
        {
          MPIC_Send(buffer+offset, recv_size, MPI_BYTE, dst,
                    MPIR_BCAST_TAG, comm->handle);

          /* recv_size was set in the previous
             receive. that's the amount of data to be
             sent now. */
        }
        /* recv only if this proc. doesn't have data and sender
           has data */
        else if ((relative_dst < relative_rank)
                 && (relative_dst < tree_root + num_procs_completed)
                 && (relative_rank >= tree_root + num_procs_completed))
        {

          MPIC_Recv(buffer + offset, scatter_size *num_procs_completed,
                    MPI_BYTE, dst, MPIR_BCAST_TAG, comm->handle,
                    &status);

          /* num_procs_completed is also equal to the no. of processes
             whose data we don't have */
          MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
          curr_size += recv_size;
        }
        tmp_mask >>= 1;
        k--;
      }
    }
    mask <<= 1;
    i++;
  }

  return MPI_SUCCESS;
}

int
MPIDO_Bcast_flattree(void * buffer,
		     int bytes,
		     int root,
		     MPID_Comm * comm)
{
  int mpi_errno = MPI_SUCCESS;
  MPI_Request * req_ptr;
  MPI_Request * reqs;
  MPI_Status status;

  int i, rank, num_procs;

  rank = comm->rank;
  num_procs = comm->local_size;

  if (rank != root)
  {
    mpi_errno = MPIC_Recv(buffer, bytes, MPI_CHAR, root, MPIR_BCAST_TAG,
                          comm->handle, &status);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    goto fn_exit;
  }

  reqs = (MPI_Request *) MPIU_Malloc((num_procs - 1) * sizeof(MPI_Request));
  MPID_assert(reqs);

  req_ptr = reqs;

  for (i = 0; i < num_procs; i++)
  {
    if (i == rank)
      continue;
    mpi_errno = MPIC_Isend(buffer, bytes, MPI_CHAR, i, MPIR_BCAST_TAG,
                           comm->handle, req_ptr++);
    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
  }

  mpi_errno = MPIR_Waitall_impl(num_procs - 1, reqs, MPI_STATUSES_IGNORE);
  if (mpi_errno) MPIU_ERR_POP(mpi_errno);

  MPIU_Free(reqs);
  
 fn_exit:
  return mpi_errno;
 fn_fail:
  goto fn_exit;
}



