/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "collutil.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 2048
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        use the short message algorithm for intercommunicator MPI_Scatter if the
        send buffer size is < this value (in bytes)

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* This is the default implementation of scatter. The algorithm is:
   
   Algorithm: MPI_Scatter

   We use a binomial tree algorithm for both short and
   long messages. At nodes other than leaf nodes we need to allocate
   a temporary buffer to store the incoming message. If the root is
   not rank 0, we reorder the sendbuf in order of relative ranks by 
   copying it into a temporary buffer, so that all the sends from the
   root are contiguous and in the right order. In the heterogeneous
   case, we first pack the buffer by using MPI_Pack and then do the
   scatter. 

   Cost = lgp.alpha + n.((p-1)/p).beta
   where n is the total size of the data to be scattered from the root.

   Possible improvements: 

   End Algorithm: MPI_Scatter
*/


/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Scatter_intra
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Scatter_intra(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                       void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                       MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    MPI_Status status;
    MPI_Aint   extent=0;
    int        rank, comm_size, is_homogeneous, sendtype_size;
    int curr_cnt, relative_rank, nbytes, send_subtree_cnt;
    int mask, recvtype_size=0, src, dst;
    int tmp_buf_size = 0;
    void *tmp_buf=NULL;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if ( ((rank == root) && (sendcount == 0)) ||
         ((rank != root) && (recvcount == 0)) )
        return MPI_SUCCESS;

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

/* Use binomial tree algorithm */
    
    if (rank == root) 
        MPIR_Datatype_get_extent_macro(sendtype, extent);
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;
    
    
    if (is_homogeneous) {
        /* communicator is homogeneous */
        if (rank == root) {
            /* We separate the two cases (root and non-root) because
               in the event of recvbuf=MPI_IN_PLACE on the root,
               recvcount and recvtype are not valid */
            MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
            MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
					     extent*sendcount*comm_size);

            nbytes = sendtype_size * sendcount;
        }
        else {
            MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
            MPIR_Ensure_Aint_fits_in_pointer(extent*recvcount*comm_size);
            nbytes = recvtype_size * recvcount;
        }
        
        curr_cnt = 0;
        
        /* all even nodes other than root need a temporary buffer to
           receive data of max size (nbytes*comm_size)/2 */
        if (relative_rank && !(relative_rank % 2)) {
	    tmp_buf_size = (nbytes*comm_size)/2;
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");
        }
        
        /* if the root is not rank 0, we reorder the sendbuf in order of
           relative ranks and copy it into a temporary buffer, so that
           all the sends from the root are contiguous and in the right
           order. */
        if (rank == root) {
            if (root != 0) {
		tmp_buf_size = nbytes*comm_size;
                MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

                if (recvbuf != MPI_IN_PLACE)
                    mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent*sendcount*rank),
                                   sendcount*(comm_size-rank), sendtype, tmp_buf,
                                   nbytes*(comm_size-rank), MPI_BYTE);
                else
                    mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent*sendcount*(rank+1)),
                                   sendcount*(comm_size-rank-1),
                                   sendtype, (char *)tmp_buf + nbytes, 
                                   nbytes*(comm_size-rank-1), MPI_BYTE);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                mpi_errno = MPIR_Localcopy(sendbuf, sendcount*rank, sendtype,
                               ((char *) tmp_buf + nbytes*(comm_size-rank)),
                               nbytes*rank, MPI_BYTE);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);

                curr_cnt = nbytes*comm_size;
            } 
            else 
                curr_cnt = sendcount*comm_size;
        }
        
        /* root has all the data; others have zero so far */
        
        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                /* The leaf nodes receive directly into recvbuf because
                   they don't have to forward data to anyone. Others
                   receive data into a temporary buffer. */
                if (relative_rank % 2) {
                    mpi_errno = MPIC_Recv(recvbuf, recvcount, recvtype,
                                             src, MPIR_SCATTER_TAG, comm_ptr,
                                             &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                else {
                    mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                             MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                        curr_cnt = 0;
                    } else
                        /* the recv size is larger than what may be sent in
                           some cases. query amount of data actually received */
                        MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
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
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                
                if ((rank == root) && (root == 0))
		{
                    send_subtree_cnt = curr_cnt - sendcount * mask;
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send(((char *)sendbuf +
                                              extent * sendcount * mask),
                                             send_subtree_cnt,
                                             sendtype, dst,
                                             MPIR_SCATTER_TAG, comm_ptr, errflag);
                }
                else
		{
                    /* non-zero root and others */
                    send_subtree_cnt = curr_cnt - nbytes*mask; 
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send(((char *)tmp_buf + nbytes*mask),
                                             send_subtree_cnt,
                                             MPI_BYTE, dst,
                                             MPIR_SCATTER_TAG, comm_ptr, errflag);
                }
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
            /* for root=0, put root's data in recvbuf if not MPI_IN_PLACE */
            mpi_errno = MPIR_Localcopy ( sendbuf, sendcount, sendtype,
                                         recvbuf, recvcount, recvtype );
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
        else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
            /* for non-zero root and non-leaf nodes, copy from tmp_buf
               into recvbuf */ 
            mpi_errno = MPIR_Localcopy ( tmp_buf, nbytes, MPI_BYTE, 
                                         recvbuf, recvcount, recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }

    }
    
#ifdef MPID_HAS_HETERO
    else { /* communicator is heterogeneous */
        int position;
        if (rank == root) {
            MPIR_Pack_size_impl(sendcount*comm_size, sendtype, &tmp_buf_size);

            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

          /* calculate the value of nbytes, the number of bytes in packed
             representation that each process receives. We can't
             accurately calculate that from tmp_buf_size because
             MPI_Pack_size returns an upper bound on the amount of memory
             required. (For example, for a single integer, MPICH-1 returns
             pack_size=12.) Therefore, we actually pack some data into
             tmp_buf and see by how much 'position' is incremented. */

            position = 0;
            mpi_errno = MPIR_Pack_impl(sendbuf, 1, sendtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
            nbytes = position*sendcount;

            curr_cnt = nbytes*comm_size;
            
            if (root == 0) {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(sendbuf, sendcount*comm_size, sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount),
                                               sendcount*(comm_size-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
            else {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount*rank),
                                               sendcount*(comm_size-rank), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcount*(rank+1)),
                                               sendcount*(comm_size-rank-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
                mpi_errno = MPIR_Pack_impl(sendbuf, sendcount*rank, sendtype, tmp_buf,
                                           tmp_buf_size, &position);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
        else {
            MPIR_Pack_size_impl(recvcount*(comm_size/2), recvtype, &tmp_buf_size);
            MPIR_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

            /* calculate nbytes */
            position = 0;
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            nbytes = position*recvcount;

            curr_cnt = 0;
        }
        
        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                mpi_errno = MPIC_Recv(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                         MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    curr_cnt = 0;
                } else
                    /* the recv size is larger than what may be sent in
                       some cases. query amount of data actually received */
                    MPIR_Get_count_impl(&status, MPI_BYTE, &curr_cnt);
                break;
            }
            mask <<= 1;
        }
        
        /* This process is responsible for all processes that have bits
           set from the LSB upto (but not including) mask.  Because of
           the "not including", we start by shifting mask back down
           one. */
        
        mask >>= 1;
        while (mask > 0) {
            if (relative_rank + mask < comm_size) {
                dst = rank + mask;
                if (dst >= comm_size) dst -= comm_size;
                
                send_subtree_cnt = curr_cnt - nbytes * mask; 
                /* mask is also the size of this process's subtree */
                mpi_errno = MPIC_Send(((char *)tmp_buf + nbytes*mask),
                                         send_subtree_cnt, MPI_BYTE, dst,
                                         MPIR_SCATTER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        /* copy local data into recvbuf */
        position = 0;
        if (recvbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                         recvcount, recvtype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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



/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Scatter_inter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Scatter_inter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                       int recvcount, MPI_Datatype recvtype, int root, MPIR_Comm *comm_ptr,
                       MPIR_Errflag_t *errflag)
{
/*  Intercommunicator scatter.
    For short messages, root sends to rank 0 in remote group. rank 0
    does local intracommunicator scatter (binomial tree). 
    Cost: (lgp+1).alpha + n.((p-1)/p).beta + n.beta
   
    For long messages, we use linear scatter to avoid the extra n.beta.
    Cost: p.alpha + n.beta
*/

    int rank, local_size, remote_size, mpi_errno=MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int i, nbytes, sendtype_size, recvtype_size;
    MPI_Status status;
    MPI_Aint extent, true_extent, true_lb = 0;
    void *tmp_buf=NULL;
    MPIR_Comm *newcomm_ptr = NULL;
    MPIR_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }

    remote_size = comm_ptr->remote_size; 
    local_size  = comm_ptr->local_size; 

    if (root == MPI_ROOT) {
        MPIR_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcount * remote_size;
    }
    else {
        /* remote side */
        MPIR_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcount * local_size;
    }

    if (nbytes < MPIR_CVAR_SCATTER_INTER_SHORT_MSG_SIZE) {
        if (root == MPI_ROOT) {
            /* root sends all data to rank 0 on remote group and returns */
            mpi_errno = MPIC_Send(sendbuf, sendcount*remote_size,
                                     sendtype, 0, MPIR_SCATTER_TAG, comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            goto fn_exit;
        }
        else {
            /* remote group. rank 0 receives data from root. need to
               allocate temporary buffer to store this data. */
            
            rank = comm_ptr->rank;
            
            if (rank == 0) {
                MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

                MPIR_Datatype_get_extent_macro(recvtype, extent);
		MPIR_Ensure_Aint_fits_in_pointer(extent*recvcount*local_size);
		MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
						 sendcount*remote_size*extent);

                MPIR_CHKLMEM_MALLOC(tmp_buf, void *, recvcount*local_size*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf");
                
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = MPIC_Recv(tmp_buf, recvcount*local_size,
                                         recvtype, root,
                                         MPIR_SCATTER_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            
            /* Get the local intracommunicator */
            if (!comm_ptr->local_comm)
                MPII_Setup_intercomm_localcomm( comm_ptr );
            
            newcomm_ptr = comm_ptr->local_comm;
            
            /* now do the usual scatter on this intracommunicator */
            mpi_errno = MPID_Scatter(tmp_buf, recvcount, recvtype,
                                          recvbuf, recvcount, recvtype, 0,
                                          newcomm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }
    else {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT) {
            MPIR_Datatype_get_extent_macro(sendtype, extent);
            for (i=0; i<remote_size; i++) {
                mpi_errno = MPIC_Send(((char *)sendbuf+sendcount*i*extent),
                                         sendcount, sendtype, i,
                                         MPIR_SCATTER_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        else {
            mpi_errno = MPIC_Recv(recvbuf,recvcount,recvtype,root,
                                     MPIR_SCATTER_TAG,comm_ptr,&status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

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


#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Scatter
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 int root, MPIR_Comm *comm_ptr, MPIR_Errflag_t *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIC_DEFAULT_Scatter_intra(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, root,
                                       comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */ 
        mpi_errno = MPIC_DEFAULT_Scatter_inter(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype, root,
                                       comm_ptr, errflag);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
  
 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

#undef FCNAME
#undef FUNCNAME
