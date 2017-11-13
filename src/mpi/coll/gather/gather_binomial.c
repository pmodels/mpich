/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_GATHER_VSMALL_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 1024
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        use a temporary buffer for intracommunicator MPI_Gather if the send
        buffer size is < this value (in bytes)
        (See also: MPIR_CVAR_GATHER_INTER_SHORT_MSG_SIZE)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#undef FUNCNAME
#define FUNCNAME MPIR_Gather_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Gather_binomial(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                      int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                      MPIR_Errflag_t *errflag)
{
    int        comm_size, rank;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int relative_rank, is_homogeneous;
    int mask, src, dst, relative_src;
    MPI_Aint curr_cnt=0, nbytes, sendtype_size, recvtype_size;
    int recvblks;
    int missing;
    MPI_Aint tmp_buf_size;
    void *tmp_buf=NULL;
    MPI_Status status;
    MPI_Aint   extent=0;            /* Datatype extent */
    int blocks[2];
    int displs[2];
    MPI_Aint struct_displs[2];
    MPI_Datatype types[2], tmp_type;
    int copy_offset = 0, copy_blks = 0;
    MPIR_CHKLMEM_DECL(1);

#ifdef MPID_HAS_HETERO
    int position, recv_size;
#endif

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if ( ((rank == root) && (recvcount == 0)) ||
         ((rank != root) && (sendcount == 0)) )
        return MPI_SUCCESS;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

    /* Use binomial tree algorithm. */
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    if (rank == root) 
    {
        MPIR_Datatype_get_extent_macro(recvtype, extent);
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf+
					 (extent*recvcount*comm_size));
    }

    if (is_homogeneous)
    {

        /* communicator is homogeneous. no need to pack buffer. */

        if (rank == root)
	{
	    MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
            nbytes = recvtype_size * recvcount;
        }
        else
	{
	    MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
            nbytes = sendtype_size * sendcount;
        }

	/* Find the number of missing nodes in my sub-tree compared to
	 * a balanced tree */
	for (mask = 1; mask < comm_size; mask <<= 1);
	--mask;
	while (relative_rank & mask) mask >>= 1;
	missing = (relative_rank | mask) - comm_size + 1;
	if (missing < 0) missing = 0;
	tmp_buf_size = (mask - missing);

	/* If the message is smaller than the threshold, we will copy
	 * our message in there too */
	if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) tmp_buf_size++;

	tmp_buf_size *= nbytes;

	/* For zero-ranked root, we don't need any temporary buffer */
	if ((rank == root) && (!root || (nbytes >= MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)))
	    tmp_buf_size = 0;

	if (tmp_buf_size) {
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);
	}

        if (rank == root)
	{
	    if (sendbuf != MPI_IN_PLACE)
	    {
		mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
					   ((char *) recvbuf + extent*recvcount*rank), recvcount, recvtype);
		if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
	    }
        }
	else if (tmp_buf_size && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE))
	{
            /* copy from sendbuf into tmp_buf */
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       tmp_buf, nbytes, MPI_BYTE);
	    if (mpi_errno) { MPIR_ERR_POP(mpi_errno); }
        }
	curr_cnt = nbytes;
        
        mask = 0x1;
        while (mask < comm_size)
	{
            if ((mask & relative_rank) == 0)
	    {
                src = relative_rank | mask;
                if (src < comm_size)
		{
                    src = (src + root) % comm_size;

		    if (rank == root)
		    {
			recvblks = mask;
			if ((2 * recvblks) > comm_size)
			    recvblks = comm_size - recvblks;

			if ((rank + mask + recvblks == comm_size) ||
			    (((rank + mask) % comm_size) <
			     ((rank + mask + recvblks) % comm_size))) {
			    /* If the data contiguously fits into the
			     * receive buffer, place it directly. This
			     * should cover the case where the root is
			     * rank 0. */
			    mpi_errno = MPIC_Recv(((char *)recvbuf +
                                                      (((rank + mask) % comm_size)*(MPI_Aint)recvcount*extent)),
                                                     (MPI_Aint)recvblks * recvcount, recvtype, src,
                                                     MPIR_GATHER_TAG, comm_ptr,
                                                     &status, errflag);
                            if (mpi_errno) {
                                /* for communication errors, just record the error but continue */
                                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                            }
			}
			else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
			    /* small transfer size case. cast ok */
			    MPIR_Assert(recvblks*nbytes == (int)(recvblks*nbytes));
			    mpi_errno = MPIC_Recv(tmp_buf, (int)(recvblks * nbytes),
					    MPI_BYTE, src, MPIR_GATHER_TAG,
					    comm_ptr, &status, errflag);
                            if (mpi_errno) {
                                /* for communication errors, just record the error but continue */
                                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                            }
			    copy_offset = rank + mask;
			    copy_blks = recvblks;
			}
			else {
			    blocks[0] = recvcount * (comm_size - root - mask);
			    displs[0] = recvcount * (root + mask);
			    blocks[1] = (recvcount * recvblks) - blocks[0];
			    displs[1] = 0;
			    
			    mpi_errno = MPIR_Type_indexed_impl(2, blocks, displs, recvtype, &tmp_type);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                            
			    mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
			    
			    mpi_errno = MPIC_Recv(recvbuf, 1, tmp_type, src,
                                                     MPIR_GATHER_TAG, comm_ptr, &status, errflag);
                            if (mpi_errno) {
                                /* for communication errors, just record the error but continue */
                                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                            }

			    MPIR_Type_free_impl(&tmp_type);
			}
		    }
                    else /* Intermediate nodes store in temporary buffer */
		    {
			MPI_Aint offset;

			/* Estimate the amount of data that is going to come in */
			recvblks = mask;
			relative_src = ((src - root) < 0) ? (src - root + comm_size) : (src - root);
			if (relative_src + mask > comm_size)
			    recvblks -= (relative_src + mask - comm_size);

			if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE)
			    offset = mask * nbytes;
			else
			    offset = (mask - 1) * nbytes;
			mpi_errno = MPIC_Recv(((char *)tmp_buf + offset),
                                                 recvblks * nbytes, MPI_BYTE, src,
                                                 MPIR_GATHER_TAG, comm_ptr,
                                                 &status, errflag);
                        if (mpi_errno) {
                            /* for communication errors, just record the error but continue */
                            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        }
			curr_cnt += (recvblks * nbytes);
                    }
                }
            }
            else
	    {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;

		if (!tmp_buf_size)
		{
                    /* leaf nodes send directly from sendbuf */
                    mpi_errno = MPIC_Send(sendbuf, sendcount, sendtype, dst,
                                             MPIR_GATHER_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                else if (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) {
		    mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                             MPIR_GATHER_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
		}
		else {
		    blocks[0] = sendcount;
		    struct_displs[0] = MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf;
		    types[0] = sendtype;
		    /* check for overflow.  work around int limits if needed*/
		    if (curr_cnt - nbytes != (int)(curr_cnt - nbytes)) {
			    blocks[1] = 1;
			    MPIR_Type_contiguous_x_impl(curr_cnt - nbytes,
					    MPI_BYTE, &(types[1]));
		    } else {
			    MPIR_Assign_trunc(blocks[1],  curr_cnt - nbytes, int);
			    types[1] = MPI_BYTE;
		    }
		    struct_displs[1] = MPIR_VOID_PTR_CAST_TO_MPI_AINT tmp_buf;
		    mpi_errno = MPIR_Type_create_struct_impl(2, blocks, struct_displs, types, &tmp_type);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                    
		    mpi_errno = MPIR_Type_commit_impl(&tmp_type);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

		    mpi_errno = MPIC_Send(MPI_BOTTOM, 1, tmp_type, dst,
                                             MPIR_GATHER_TAG, comm_ptr, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
		    MPIR_Type_free_impl(&tmp_type);
		    if (types[1] != MPI_BYTE)
			    MPIR_Type_free_impl(&types[1]);
		}

                break;
            }
            mask <<= 1;
        }

        if ((rank == root) && root && (nbytes < MPIR_CVAR_GATHER_VSMALL_MSG_SIZE) && copy_blks)
	{
            /* reorder and copy from tmp_buf into recvbuf */
	    mpi_errno = MPIR_Localcopy(tmp_buf,
                                       nbytes * (comm_size - copy_offset), MPI_BYTE,  
                                       ((char *) recvbuf + extent * recvcount * copy_offset),
                                       recvcount * (comm_size - copy_offset), recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
	    mpi_errno = MPIR_Localcopy((char *) tmp_buf + nbytes * (comm_size - copy_offset),
                                       nbytes * (copy_blks - comm_size + copy_offset), MPI_BYTE,  
                                       recvbuf,
                                       recvcount * (copy_blks - comm_size + copy_offset), recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

    }
    
#ifdef MPID_HAS_HETERO
    else
    { /* communicator is heterogeneous. pack data into tmp_buf. */
        if (rank == root)
            MPIR_Pack_size_impl(recvcount*comm_size, recvtype, &tmp_buf_size);
        else
            MPIR_Pack_size_impl(sendcount*(comm_size/2), sendtype, &tmp_buf_size);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        position = 0;
        if (sendbuf != MPI_IN_PLACE)
	{
            mpi_errno = MPIR_Pack_impl(sendbuf, sendcount, sendtype, tmp_buf,
                                       tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position;
        }
        else
	{
            /* do a dummy pack just to calculate nbytes */
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf,
                                       tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position*recvcount;
        }
        
        curr_cnt = nbytes;
        
        mask = 0x1;
        while (mask < comm_size)
	{
            if ((mask & relative_rank) == 0)
	    {
                src = relative_rank | mask;
                if (src < comm_size)
		{
                    src = (src + root) % comm_size;
                    mpi_errno = MPIC_Recv(((char *)tmp_buf + curr_cnt),
                                             tmp_buf_size-curr_cnt, MPI_BYTE, src,
                                             MPIR_GATHER_TAG, comm_ptr,
                                             &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        recv_size = 0;
                    } else
                        /* the recv size is larger than what may be sent in
                           some cases. query amount of data actually received */
                        MPIR_Get_count_impl(&status, MPI_BYTE, &recv_size);
                    curr_cnt += recv_size;
                }
            }
            else
	    {
                dst = relative_rank ^ mask;
                dst = (dst + root) % comm_size;
                mpi_errno = MPIC_Send(tmp_buf, curr_cnt, MPI_BYTE, dst,
                                         MPIR_GATHER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                break;
            }
            mask <<= 1;
        }
        
        if (rank == root)
	{
            /* reorder and copy from tmp_buf into recvbuf */
            if (sendbuf != MPI_IN_PLACE)
	    {
                position = 0;
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                             ((char *) recvbuf + extent*recvcount*rank),
                                             recvcount*(comm_size-rank), recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else
	    {
                position = nbytes;
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position,
                                             ((char *) recvbuf + extent*recvcount*(rank+1)),
                                             recvcount*(comm_size-rank-1), recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            if (root != 0) {
                mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                             recvcount*rank, recvtype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        
    }
#endif /* MPID_HAS_HETERO */

 fn_exit:
    MPIR_CHKLMEM_FREEALL();
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
