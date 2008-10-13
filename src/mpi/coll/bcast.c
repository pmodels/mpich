/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Bcast */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Bcast = PMPI_Bcast
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Bcast  MPI_Bcast
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Bcast as PMPI_Bcast
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Bcast
#define MPI_Bcast PMPI_Bcast

/* This is the default implementation of broadcast. The algorithm is:
   
   Algorithm: MPI_Bcast

   For short messages, we use a binomial tree algorithm. 
   Cost = lgp.alpha + n.lgp.beta

   For long messages, we do a scatter followed by an allgather. 
   We first scatter the buffer using a binomial tree algorithm. This costs
   lgp.alpha + n.((p-1)/p).beta
   If the datatype is contiguous and the communicator is homogeneous,
   we treat the data as bytes and divide (scatter) it among processes
   by using ceiling division. For the noncontiguous or heterogeneous
   cases, we first pack the data into a temporary buffer by using
   MPI_Pack, scatter it as bytes, and unpack it after the allgather.

   For the allgather, we use a recursive doubling algorithm for 
   medium-size messages and power-of-two number of processes. This
   takes lgp steps. In each step pairs of processes exchange all the
   data they have (we take care of non-power-of-two situations). This
   costs approximately lgp.alpha + n.((p-1)/p).beta. (Approximately
   because it may be slightly more in the non-power-of-two case, but
   it's still a logarithmic algorithm.) Therefore, for long messages
   Total Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta

   Note that this algorithm has twice the latency as the tree algorithm
   we use for short messages, but requires lower bandwidth: 2.n.beta
   versus n.lgp.beta. Therefore, for long messages and when lgp > 2,
   this algorithm will perform better.

   For long messages and for medium-size messages and non-power-of-two 
   processes, we use a ring algorithm for the allgather, which 
   takes p-1 steps, because it performs better than recursive doubling.
   Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta

   Possible improvements: 
   For clusters of SMPs, we may want to do something differently to
   take advantage of shared memory on each node.

   End Algorithm: MPI_Bcast
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* begin:nested */

/* not declared static because it is called in intercomm. allgatherv */
int MPIR_Bcast ( 
	void *buffer, 
	int count, 
	MPI_Datatype datatype, 
	int root, 
	MPID_Comm *comm_ptr )
{
  MPI_Status status;
  int        rank, comm_size, src, dst;
  int        relative_rank, mask;
  int        mpi_errno = MPI_SUCCESS;
  int scatter_size, nbytes=0, curr_size, recv_size = 0, send_size;
  int type_size, j, k, i, tmp_mask, is_contig, is_homogeneous;
  int relative_dst, dst_tree_root, my_tree_root, send_offset;
  int recv_offset, tree_root, nprocs_completed, offset, position;
  int *recvcnts, *displs, left, right, jnext, pof2, comm_size_is_pof2;
  void *tmp_buf=NULL;
  MPI_Comm comm;
  MPID_Datatype *dtp;
  MPI_Aint true_extent, true_lb;
  MPID_MPI_STATE_DECL(MPID_STATE_MPIR_BCAST);
  
  MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_BCAST);

  if (count == 0) goto fn_exit;

  comm = comm_ptr->handle;
  comm_size = comm_ptr->local_size;
  rank = comm_ptr->rank;
  
  /* If there is only one process, return */
  if (comm_size == 1) goto fn_exit;

  if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
      is_contig = 1;
  else {
      MPID_Datatype_get_ptr(datatype, dtp); 
      is_contig = dtp->is_contig;
  }

  is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
  if (comm_ptr->is_hetero)
      is_homogeneous = 0;
#endif

  /* MPI_Type_size() might not give the accurate size of the packed
   * datatype for heterogeneous systems (because of padding, encoding,
   * etc). On the other hand, MPI_Pack_size() can become very
   * expensive, depending on the implementation, especially for
   * heterogeneous systems. We want to use MPI_Type_size() wherever
   * possible, and MPI_Pack_size() in other places.
   */
  if (is_homogeneous) {
      MPID_Datatype_get_size_macro(datatype, type_size);
  }
  else {
      mpi_errno = NMPI_Pack_size(1, datatype, comm, &type_size);
      if (mpi_errno != MPI_SUCCESS) {
	  MPIU_ERR_POP(mpi_errno);
      }
  }
  nbytes = type_size * count;

  if (!is_contig || !is_homogeneous)
  {
      tmp_buf = MPIU_Malloc(nbytes);
      /* --BEGIN ERROR HANDLING-- */
      if (!tmp_buf)
      {
          mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
					    "**nomem %d", type_size );
          goto fn_fail;
      }
      /* --END ERROR HANDLING-- */

      /* TODO: Pipeline the packing and communication */
      position = 0;
      if (rank == root)
	  NMPI_Pack(buffer, count, datatype, tmp_buf, nbytes,
		    &position, comm);
  }

  relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

  /* check if multiple threads are calling this collective function */
  MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

  if ((nbytes < MPIR_BCAST_SHORT_MSG) || (comm_size < MPIR_BCAST_MIN_PROCS))
  {

      /* Use short message algorithm, namely, binomial tree */

      /* Algorithm:
         This uses a fairly basic recursive subdivision algorithm.
         The root sends to the process comm_size/2 away; the receiver becomes
         a root for a subtree and applies the same process. 

         So that the new root can easily identify the size of its
         subtree, the (subtree) roots are all powers of two (relative
         to the root) If m = the first power of 2 such that 2^m >= the
         size of the communicator, then the subtree at root at 2^(m-k)
         has size 2^k (with special handling for subtrees that aren't
         a power of two in size).
     
         Do subdivision.  There are two phases:
         1. Wait for arrival of data.  Because of the power of two nature
         of the subtree roots, the source of this message is alwyas the
         process whose relative rank has the least significant 1 bit CLEARED.
         That is, process 4 (100) receives from process 0, process 7 (111) 
         from process 6 (110), etc.   
         2. Forward to my subtree
         
         Note that the process that is the tree root is handled automatically
         by this code, since it has no bits set.  */

      mask = 0x1;
      while (mask < comm_size)
      {
          if (relative_rank & mask)
	  {
              src = rank - mask; 
              if (src < 0) src += comm_size;
	      if (!is_contig || !is_homogeneous)
		  mpi_errno = MPIC_Recv(tmp_buf,nbytes,MPI_BYTE,src,
					MPIR_BCAST_TAG,comm,MPI_STATUS_IGNORE);
	      else
		  mpi_errno = MPIC_Recv(buffer,count,datatype,src,
					MPIR_BCAST_TAG,comm,MPI_STATUS_IGNORE);
              if (mpi_errno != MPI_SUCCESS) {
		  MPIU_ERR_POP(mpi_errno);
	      }
              break;
          }
          mask <<= 1;
      }

      /* This process is responsible for all processes that have bits
         set from the LSB upto (but not including) mask.  Because of
         the "not including", we start by shifting mask back down one.
         
         We can easily change to a different algorithm at any power of two
         by changing the test (mask > 1) to (mask > block_size) 

         One such version would use non-blocking operations for the last 2-4
         steps (this also bounds the number of MPI_Requests that would
         be needed).  */

      mask >>= 1;
      while (mask > 0)
      {
          if (relative_rank + mask < comm_size)
	  {
              dst = rank + mask;
              if (dst >= comm_size) dst -= comm_size;
	      if (!is_contig || !is_homogeneous)
		  mpi_errno = MPIC_Send(tmp_buf,nbytes,MPI_BYTE,dst,
					MPIR_BCAST_TAG,comm);
	      else
		  mpi_errno = MPIC_Send(buffer,count,datatype,dst,
					MPIR_BCAST_TAG,comm); 
              if (mpi_errno != MPI_SUCCESS) {
		  MPIU_ERR_POP(mpi_errno);
	      }
          }
          mask >>= 1;
      }
  }

  else
  { 
      /* use long message algorithm: binomial tree scatter followed by an allgather */

      /* The scatter algorithm divides the buffer into nprocs pieces and
         scatters them among the processes. Root gets the first piece,
         root+1 gets the second piece, and so forth. Uses the same binomial
         tree algorithm as above. Ceiling division
         is used to compute the size of each piece. This means some
         processes may not get any data. For example if bufsize = 97 and
         nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
         scattered data is stored at the same offset in the buffer as it is
         on the root process. */ 

      if (is_contig && is_homogeneous)
      {
          /* contiguous and homogeneous. no need to pack. */
          mpi_errno = NMPI_Type_get_true_extent(datatype, &true_lb,
                                                &true_extent);
          if (mpi_errno) {
	      MPIU_ERR_POP(mpi_errno);
	  }
          tmp_buf = (char *) buffer + true_lb;
      }

      scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */
      curr_size = (rank == root) ? nbytes : 0; /* root starts with all the
                                                  data */

      mask = 0x1;
      while (mask < comm_size)
      {
          if (relative_rank & mask)
	  {
              src = rank - mask; 
              if (src < 0) src += comm_size;
              recv_size = nbytes - relative_rank*scatter_size;
              /* recv_size is larger than what might actually be sent by the
                 sender. We don't need compute the exact value because MPI
                 allows you to post a larger recv.*/ 
              if (recv_size <= 0)
	      {
                  curr_size = 0; /* this process doesn't receive any data
                                    because of uneven division */
	      }
              else
	      {
                  mpi_errno = MPIC_Recv(((char *)tmp_buf +
                                         relative_rank*scatter_size),
                                        recv_size, MPI_BYTE, src,
                                        MPIR_BCAST_TAG, comm, &status);
                  if (mpi_errno != MPI_SUCCESS) {
		      MPIU_ERR_POP(mpi_errno);
		  }

                  /* query actual size of data received */
                  NMPI_Get_count(&status, MPI_BYTE, &curr_size);
              }
              break;
          }
          mask <<= 1;
      }

      /* This process is responsible for all processes that have bits
         set from the LSB upto (but not including) mask.  Because of
         the "not including", we start by shifting mask back down
         one. */

      mask >>= 1;
      while (mask > 0)
      {
          if (relative_rank + mask < comm_size)
	  {
              send_size = curr_size - scatter_size * mask; 
              /* mask is also the size of this process's subtree */

              if (send_size > 0)
	      {
                  dst = rank + mask;
                  if (dst >= comm_size) dst -= comm_size;
                  mpi_errno = MPIC_Send (((char *)tmp_buf +
                                         scatter_size*(relative_rank+mask)),
                                        send_size, MPI_BYTE, dst,
                                        MPIR_BCAST_TAG, comm);
                  if (mpi_errno != MPI_SUCCESS) {
		      MPIU_ERR_POP(mpi_errno);
		  }
                  curr_size -= send_size;
              }
          }
          mask >>= 1;
      }

      /* Scatter complete. Now do an allgather .  */ 

      /* check if comm_size is a power of two */
      pof2 = 1;
      while (pof2 < comm_size)
          pof2 *= 2;
      if (pof2 == comm_size) 
          comm_size_is_pof2 = 1;
      else
          comm_size_is_pof2 = 0;

      if ((nbytes < MPIR_BCAST_LONG_MSG) && (comm_size_is_pof2))
      {
          /* medium size allgather and pof2 comm_size. use recurive doubling. */

          mask = 0x1;
          i = 0;
          while (mask < comm_size)
	  {
              relative_dst = relative_rank ^ mask;
              
              dst = (relative_dst + root) % comm_size; 
              
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
              
              if (relative_dst < comm_size)
	      {
                  mpi_errno = MPIC_Sendrecv(((char *)tmp_buf + send_offset),
                                            curr_size, MPI_BYTE, dst, MPIR_BCAST_TAG, 
                                            ((char *)tmp_buf + recv_offset),
                                            (nbytes-recv_offset < 0 ? 0 : nbytes-recv_offset), 
					    MPI_BYTE, dst, MPIR_BCAST_TAG, comm, &status);
                  if (mpi_errno != MPI_SUCCESS) {
		      MPIU_ERR_POP(mpi_errno);
		  }
                  NMPI_Get_count(&status, MPI_BYTE, &recv_size);
                  curr_size += recv_size;
              }
              
              /* if some processes in this process's subtree in this step
                 did not have any destination process to communicate with
                 because of non-power-of-two, we need to send them the
                 data that they would normally have received from those
                 processes. That is, the haves in this subtree must send to
                 the havenots. We use a logarithmic recursive-halfing algorithm
                 for this. */
              
              /* This part of the code will not currently be
                 executed because we are not using recursive
                 doubling for non power of two. Mark it as experimental
                 so that it doesn't show up as red in the coverage tests. */  

	      /* --BEGIN EXPERIMENTAL-- */
              if (dst_tree_root + mask > comm_size)
	      {
                  nprocs_completed = comm_size - my_tree_root - mask;
                  /* nprocs_completed is the number of processes in this
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
                      dst = (relative_dst + root) % comm_size; 
                      
                      tree_root = relative_rank >> k;
                      tree_root <<= k;
                      
                      /* send only if this proc has data and destination
                         doesn't have data. */
                      
                      /* if (rank == 3) { 
                         printf("rank %d, dst %d, root %d, nprocs_completed %d\n", relative_rank, relative_dst, tree_root, nprocs_completed);
                         fflush(stdout);
                         }*/
                      
                      if ((relative_dst > relative_rank) && 
                          (relative_rank < tree_root + nprocs_completed)
                          && (relative_dst >= tree_root + nprocs_completed))
		      {
                          
                          /* printf("Rank %d, send to %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                             fflush(stdout); */
                          mpi_errno = MPIC_Send(((char *)tmp_buf + offset),
                                                recv_size, MPI_BYTE, dst,
                                                MPIR_BCAST_TAG, comm); 
                          /* recv_size was set in the previous
                             receive. that's the amount of data to be
                             sent now. */
                          if (mpi_errno != MPI_SUCCESS) {
			      MPIU_ERR_POP(mpi_errno);
			  }
                      }
                      /* recv only if this proc. doesn't have data and sender
                         has data */
                      else if ((relative_dst < relative_rank) && 
                               (relative_dst < tree_root + nprocs_completed) &&
                               (relative_rank >= tree_root + nprocs_completed))
		      {
                          /* printf("Rank %d waiting to recv from rank %d\n",
                             relative_rank, dst); */
                          mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                                nbytes - offset, 
                                                MPI_BYTE, dst, MPIR_BCAST_TAG,
                                                comm, &status); 
                          /* nprocs_completed is also equal to the no. of processes
                             whose data we don't have */
                          if (mpi_errno != MPI_SUCCESS) {
			      MPIU_ERR_POP(mpi_errno);
			  }
                          NMPI_Get_count(&status, MPI_BYTE, &recv_size);
                          curr_size += recv_size;
                          /* printf("Rank %d, recv from %d, offset %d, size %d\n", rank, dst, offset, recv_size);
                             fflush(stdout);*/
                      }
                      tmp_mask >>= 1;
                      k--;
                  }
              }
              /* --END EXPERIMENTAL-- */
              
              mask <<= 1;
              i++;
          }
      } 
      else
      {
          /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */ 

          recvcnts = MPIU_Malloc(comm_size*sizeof(int));
	  /* --BEGIN ERROR HANDLING-- */
          if (!recvcnts)
	  {
              mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
						"**nomem %d", comm_size * sizeof(int));
              goto fn_fail;
          }
	  /* --END ERROR HANDLING-- */
          displs = MPIU_Malloc(comm_size*sizeof(int));
	  /* --BEGIN ERROR HANDLING-- */
          if (!displs)
	  {
              mpi_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem",
						"**nomem %d", comm_size * sizeof(int));
              goto fn_fail;
          }
	  /* --END ERROR HANDLING-- */
          
          for (i=0; i<comm_size; i++)
	  {
              recvcnts[i] = nbytes - i*scatter_size;
              if (recvcnts[i] > scatter_size)
                  recvcnts[i] = scatter_size;
              if (recvcnts[i] < 0)
                  recvcnts[i] = 0;
          }
          
          displs[0] = 0;
          for (i=1; i<comm_size; i++)
              displs[i] = displs[i-1] + recvcnts[i-1];
          
          left  = (comm_size + rank - 1) % comm_size;
          right = (rank + 1) % comm_size;
          
          j     = rank;
          jnext = left;
          for (i=1; i<comm_size; i++)
	  {
              mpi_errno = 
                  MPIC_Sendrecv((char *)tmp_buf +
                                displs[(j-root+comm_size)%comm_size],  
                                recvcnts[(j-root+comm_size)%comm_size],
                                MPI_BYTE, right, MPIR_BCAST_TAG, 
                                (char *)tmp_buf +
                                displs[(jnext-root+comm_size)%comm_size], 
                                recvcnts[(jnext-root+comm_size)%comm_size],  
                                MPI_BYTE, left,   
                                MPIR_BCAST_TAG, comm, MPI_STATUS_IGNORE);
              if (mpi_errno != MPI_SUCCESS) {
		  MPIU_ERR_POP(mpi_errno);
	      }
              j	    = jnext;
              jnext = (comm_size + jnext - 1) % comm_size;
          }
          
          MPIU_Free(recvcnts);
          MPIU_Free(displs);
      }
  }

  if (!is_contig || !is_homogeneous)
  {
      if (rank != root)
      {
	  position = 0;
	  NMPI_Unpack(tmp_buf, nbytes, &position, buffer, count,
		      datatype, comm);
      }
      MPIU_Free(tmp_buf);
  }

 fn_exit:
  /* check if multiple threads are calling this collective function */
  MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );

  MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_BCAST);

  return mpi_errno;
 fn_fail:
  goto fn_exit;
}
/* end:nested */

/* A simple utility function to that calls the comm_ptr->coll_fns->Bcast
   override if it exists or else it calls MPIR_Bcast with the same arguments.
   This function just makes the high-level broadcast logic easier to read while
   still accomodating coll_fns-style overrides.  It also reduces future errors
   by eliminating the duplication of Bcast arguments. 

   This routine is used in other files as well (barrier.c, allreduce.c)
*/
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_or_coll_fn
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Bcast_or_coll_fn(void *buffer, 
			  int count, 
			  MPI_Datatype datatype, 
			  int root, 
			  MPID_Comm *comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Bcast != NULL)
    {
        /* --BEGIN USEREXTENSION-- */
        mpi_errno = comm_ptr->node_roots_comm->coll_fns->Bcast(buffer, count,
                                                               datatype, root, comm_ptr);
        /* --END USEREXTENSION-- */
    }
    else {
        mpi_errno = MPIR_Bcast(buffer, count, datatype, root, comm_ptr);
    }

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/* begin:nested */
/* Not PMPI_LOCAL because it is called in intercomm allgather */
int MPIR_Bcast_inter ( 
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPID_Comm *comm_ptr )
{
/*  Intercommunicator broadcast.
    Root sends to rank 0 in remote group. Remote group does local
    intracommunicator broadcast.
*/
    int rank, mpi_errno;
    MPI_Status status;
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;
    MPID_MPI_STATE_DECL(MPID_STATE_MPIR_BCAST_INTER);

    MPID_MPI_FUNC_ENTER(MPID_STATE_MPIR_BCAST_INTER);

    comm = comm_ptr->handle;

    if (root == MPI_PROC_NULL)
    {
        /* local processes other than root do nothing */
        mpi_errno = MPI_SUCCESS;
    }
    else if (root == MPI_ROOT)
    {
        /* root sends to rank 0 on remote group and returns */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
        mpi_errno =  MPIC_Send(buffer, count, datatype, 0,
                               MPIR_BCAST_TAG, comm); 
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, 
					     FCNAME, __LINE__, MPI_ERR_OTHER, 
					     "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
        MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    }
    else
    {
        /* remote group. rank 0 on remote group receives from root */
        
        rank = comm_ptr->rank;
        
        if (rank == 0)
	{
            mpi_errno = MPIC_Recv(buffer, count, datatype, root,
                                  MPIR_BCAST_TAG, comm, &status);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, 
						 FCNAME, __LINE__, 
						 MPI_ERR_OTHER, "**fail", 0);
		return mpi_errno;
	    }
	    /* --END ERROR HANDLING-- */
        }
        
        /* Get the local intracommunicator */
        if (!comm_ptr->local_comm)
            MPIR_Setup_intercomm_localcomm( comm_ptr );

        newcomm_ptr = comm_ptr->local_comm;

        /* now do the usual broadcast on this intracommunicator
           with rank 0 as root. */
        mpi_errno = MPIR_Bcast(buffer, count, datatype, 0, newcomm_ptr);
	/* --BEGIN ERROR HANDLING-- */
	if (mpi_errno != MPI_SUCCESS)
	{
	    mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
	}
	/* --END ERROR HANDLING-- */
    }

    MPID_MPI_FUNC_EXIT(MPID_STATE_MPIR_BCAST_INTER);
    
    return mpi_errno;
}
/* end:nested */
#endif /* MPICH_MPI_FROM_PMPI */

#undef FUNCNAME
#define FUNCNAME MPI_Bcast
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)

/*@
MPI_Bcast - Broadcasts a message from the process with rank "root" to
            all other processes of the communicator

Input/Output Parameter:
. buffer - starting address of buffer (choice) 

Input Parameters:
+ count - number of entries in buffer (integer) 
. datatype - data type of buffer (handle) 
. root - rank of broadcast root (integer) 
- comm - communicator (handle) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
.N MPI_ERR_ROOT
@*/
int MPI_Bcast( void *buffer, int count, MPI_Datatype datatype, int root, 
               MPI_Comm comm )
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_BCAST);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_BCAST);

    /* Validate parameters, especially handles needing to be converted */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
	    MPIR_ERRTEST_COMM(comm, mpi_errno);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	}
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* Convert MPI object handles to object pointers */
    MPID_Comm_get_ptr( comm, comm_ptr );

    /* Validate parameters and objects (post conversion) */
#   ifdef HAVE_ERROR_CHECKING
    {
        MPID_BEGIN_ERROR_CHECKS;
        {
            MPID_Datatype *datatype_ptr = NULL;
	    
            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
	    MPIR_ERRTEST_COUNT(count, mpi_errno);
	    MPIR_ERRTEST_DATATYPE(datatype, "datatype", mpi_errno);
	    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);
	    }
	    else {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);
	    }
	    
            if (HANDLE_GET_KIND(datatype) != HANDLE_KIND_BUILTIN) {
                MPID_Datatype_get_ptr(datatype, datatype_ptr);
                MPID_Datatype_valid_ptr( datatype_ptr, mpi_errno );
                MPID_Datatype_committed_ptr( datatype_ptr, mpi_errno );
            }

            MPIR_ERRTEST_BUF_INPLACE(buffer, count, mpi_errno);
            MPIR_ERRTEST_USERBUFFER(buffer,count,datatype,mpi_errno);
            
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Bcast != NULL)
    {
	/* --BEGIN USEREXTENSION-- */
	mpi_errno = comm_ptr->coll_fns->Bcast(buffer, count,
                                              datatype, root, comm_ptr);
	/* --END USEREXTENSION-- */
    }
    else
    {
	MPIU_THREADPRIV_DECL;
	MPIU_THREADPRIV_GET;

	MPIR_Nest_incr();
        if (comm_ptr->comm_kind == MPID_INTRACOMM)
	{
            /* intracommunicator */
#if USE_SMP_COLLECTIVES
            if (MPIR_Comm_is_node_aware(comm_ptr)) {
                /* perform the intranode broadcast on the root's node */
                if (comm_ptr->node_comm != NULL &&
                    MPIU_Get_intranode_rank(comm_ptr, root) > 0) /* is not the node root (0) */ 
                {                                                /* and is on our node (!-1) */
                    mpi_errno = MPIR_Bcast_or_coll_fn(buffer, count, datatype,
                                                      MPIU_Get_intranode_rank(comm_ptr, root),
                                                      comm_ptr->node_comm);
                    if (mpi_errno) {
                        MPIR_Nest_decr();
                        goto fn_fail;
                    }
                }

                /* perform the internode broadcast */
                if (comm_ptr->node_roots_comm != NULL) {
                    mpi_errno = MPIR_Bcast_or_coll_fn(buffer, count, datatype,
                                                      MPIU_Get_internode_rank(comm_ptr, root),
                                                      comm_ptr->node_roots_comm);
                    if (mpi_errno) {
                        MPIR_Nest_decr();
                        goto fn_fail;
                    }
                }

                /* perform the intranode broadcast on all except for the root's node */
                if (comm_ptr->node_comm != NULL &&
                    MPIU_Get_intranode_rank(comm_ptr, root) <= 0) /* 0 if root was local root too */
                {                                                 /* -1 if different node than root */
                    mpi_errno = MPIR_Bcast_or_coll_fn(buffer, count, datatype, 0, comm_ptr->node_comm);
                }
            }
            else {
                mpi_errno = MPIR_Bcast( buffer, count, datatype, root, comm_ptr );
            }
#else
            mpi_errno = MPIR_Bcast( buffer, count, datatype, root, comm_ptr );
#endif
	}
        else
	{
            /* intercommunicator */
            mpi_errno = MPIR_Bcast_inter( buffer, count, datatype, root, comm_ptr );
        }
	MPIR_Nest_decr();
    }

    if (mpi_errno != MPI_SUCCESS) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_BCAST);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, 
	    "**mpi_bcast",
	    "**mpi_bcast %p %d %D %d %C", buffer, count, datatype, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
