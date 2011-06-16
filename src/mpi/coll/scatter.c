/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* -- Begin Profiling Symbol Block for routine MPI_Scatter */
#if defined(HAVE_PRAGMA_WEAK)
#pragma weak MPI_Scatter = PMPI_Scatter
#elif defined(HAVE_PRAGMA_HP_SEC_DEF)
#pragma _HP_SECONDARY_DEF PMPI_Scatter  MPI_Scatter
#elif defined(HAVE_PRAGMA_CRI_DUP)
#pragma _CRI duplicate MPI_Scatter as PMPI_Scatter
#endif
/* -- End Profiling Symbol Block */

/* Define MPICH_MPI_FROM_PMPI if weak symbols are not supported to build
   the MPI routines */
#ifndef MPICH_MPI_FROM_PMPI
#undef MPI_Scatter
#define MPI_Scatter PMPI_Scatter

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
#define FUNCNAME MPIR_Scatter_intra
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Scatter_intra ( 
	void *sendbuf, 
	int sendcnt, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr,
        int *errflag )
{
    MPI_Status status;
    MPI_Aint   extent=0;
    int        rank, comm_size, is_homogeneous, sendtype_size;
    int curr_cnt, relative_rank, nbytes, send_subtree_cnt;
    int mask, recvtype_size=0, src, dst, position;
    int tmp_buf_size = 0;
    void *tmp_buf=NULL;
    int        mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Comm comm;
    MPIU_CHKLMEM_DECL(4);
    
    comm = comm_ptr->handle;
    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    if ( ((rank == root) && (sendcnt == 0)) ||
         ((rank != root) && (recvcnt == 0)) )
        return MPI_SUCCESS;

    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );

    is_homogeneous = 1;
#ifdef MPID_HAS_HETERO
    if (comm_ptr->is_hetero)
        is_homogeneous = 0;
#endif

/* Use binomial tree algorithm */
    
    if (rank == root) 
        MPID_Datatype_get_extent_macro(sendtype, extent);
    
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;
    
    
    if (is_homogeneous) {
        /* communicator is homogeneous */
        if (rank == root) {
            /* We separate the two cases (root and non-root) because
               in the event of recvbuf=MPI_IN_PLACE on the root,
               recvcnt and recvtype are not valid */
            MPID_Datatype_get_size_macro(sendtype, sendtype_size);
            MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
					     extent*sendcnt*comm_size);

            nbytes = sendtype_size * sendcnt;
        }
        else {
            MPID_Datatype_get_size_macro(recvtype, recvtype_size);
            MPID_Ensure_Aint_fits_in_pointer(extent*recvcnt*comm_size);
            nbytes = recvtype_size * recvcnt;
        }
        
        curr_cnt = 0;
        
        /* all even nodes other than root need a temporary buffer to
           receive data of max size (nbytes*comm_size)/2 */
        if (relative_rank && !(relative_rank % 2)) {
	    tmp_buf_size = (nbytes*comm_size)/2;
            MPIU_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");
        }
        
        /* if the root is not rank 0, we reorder the sendbuf in order of
           relative ranks and copy it into a temporary buffer, so that
           all the sends from the root are contiguous and in the right
           order. */
        if (rank == root) {
            if (root != 0) {
		tmp_buf_size = nbytes*comm_size;
                MPIU_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

                position = 0;

                if (recvbuf != MPI_IN_PLACE)
                    mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent*sendcnt*rank),
                                   sendcnt*(comm_size-rank), sendtype, tmp_buf,
                                   nbytes*(comm_size-rank), MPI_BYTE);
                else
                    mpi_errno = MPIR_Localcopy(((char *) sendbuf + extent*sendcnt*(rank+1)),
                                   sendcnt*(comm_size-rank-1),
                                   sendtype, (char *)tmp_buf + nbytes, 
                                   nbytes*(comm_size-rank-1), MPI_BYTE);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                mpi_errno = MPIR_Localcopy(sendbuf, sendcnt*rank, sendtype, 
                               ((char *) tmp_buf + nbytes*(comm_size-rank)),
                               nbytes*rank, MPI_BYTE);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);

                curr_cnt = nbytes*comm_size;
            } 
            else 
                curr_cnt = sendcnt*comm_size;
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
                    mpi_errno = MPIC_Recv_ft(recvbuf, recvcnt, recvtype,
                                             src, MPIR_SCATTER_TAG, comm, 
                                             &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                    }
                }
                else {
                    mpi_errno = MPIC_Recv_ft(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                             MPIR_SCATTER_TAG, comm, &status, errflag);
                    if (mpi_errno) {
                        /* for communication errors, just record the error but continue */
                        *errflag = TRUE;
                        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                        MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
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
                    send_subtree_cnt = curr_cnt - sendcnt * mask; 
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send_ft(((char *)sendbuf + 
                                              extent * sendcnt * mask),
                                             send_subtree_cnt,
                                             sendtype, dst,
                                             MPIR_SCATTER_TAG, comm, errflag);
                }
                else
		{
                    /* non-zero root and others */
                    send_subtree_cnt = curr_cnt - nbytes*mask; 
                    /* mask is also the size of this process's subtree */
                    mpi_errno = MPIC_Send_ft(((char *)tmp_buf + nbytes*mask),
                                             send_subtree_cnt,
                                             MPI_BYTE, dst,
                                             MPIR_SCATTER_TAG, comm, errflag);
                }
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = TRUE;
                    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                    MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        if ((rank == root) && (root == 0) && (recvbuf != MPI_IN_PLACE)) {
            /* for root=0, put root's data in recvbuf if not MPI_IN_PLACE */
            mpi_errno = MPIR_Localcopy ( sendbuf, sendcnt, sendtype, 
                                         recvbuf, recvcnt, recvtype );
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
        else if (!(relative_rank % 2) && (recvbuf != MPI_IN_PLACE)) {
            /* for non-zero root and non-leaf nodes, copy from tmp_buf
               into recvbuf */ 
            mpi_errno = MPIR_Localcopy ( tmp_buf, nbytes, MPI_BYTE, 
                                         recvbuf, recvcnt, recvtype);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }

    }
    
#ifdef MPID_HAS_HETERO
    else { /* communicator is heterogeneous */
        if (rank == root) {
            MPIR_Pack_size_impl(sendcnt*comm_size, sendtype, &tmp_buf_size);

            MPIU_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

          /* calculate the value of nbytes, the number of bytes in packed
             representation that each process receives. We can't
             accurately calculate that from tmp_buf_size because
             MPI_Pack_size returns an upper bound on the amount of memory
             required. (For example, for a single integer, MPICH-1 returns
             pack_size=12.) Therefore, we actually pack some data into
             tmp_buf and see by how much 'position' is incremented. */

            position = 0;
            mpi_errno = MPIR_Pack_impl(sendbuf, 1, sendtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            
            nbytes = position*sendcnt;

            curr_cnt = nbytes*comm_size;
            
            if (root == 0) {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(sendbuf, sendcnt*comm_size, sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcnt), 
                                               sendcnt*(comm_size-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
            }
            else {
                if (recvbuf != MPI_IN_PLACE) {
                    position = 0;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcnt*rank),
                                               sendcnt*(comm_size-rank), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
                else {
                    position = nbytes;
                    mpi_errno = MPIR_Pack_impl(((char *) sendbuf + extent*sendcnt*(rank+1)),
                                               sendcnt*(comm_size-rank-1), sendtype, tmp_buf,
                                               tmp_buf_size, &position);
                    if (mpi_errno) MPIU_ERR_POP(mpi_errno);
                }
                mpi_errno = MPIR_Pack_impl(sendbuf, sendcnt*rank, sendtype, tmp_buf,
                                           tmp_buf_size, &position);
                if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            }
        }
        else {
            MPIR_Pack_size_impl(recvcnt*(comm_size/2), recvtype, &tmp_buf_size);
            MPIU_CHKLMEM_MALLOC(tmp_buf, void *, tmp_buf_size, mpi_errno, "tmp_buf");

            /* calculate nbytes */
            position = 0;
            mpi_errno = MPIR_Pack_impl(recvbuf, 1, recvtype, tmp_buf, tmp_buf_size, &position);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
            nbytes = position*recvcnt;

            curr_cnt = 0;
        }
        
        mask = 0x1;
        while (mask < comm_size) {
            if (relative_rank & mask) {
                src = rank - mask; 
                if (src < 0) src += comm_size;
                
                mpi_errno = MPIC_Recv_ft(tmp_buf, tmp_buf_size, MPI_BYTE, src,
                                         MPIR_SCATTER_TAG, comm, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = TRUE;
                    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                    MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
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
                mpi_errno = MPIC_Send_ft(((char *)tmp_buf + nbytes*mask),
                                         send_subtree_cnt, MPI_BYTE, dst,
                                         MPIR_SCATTER_TAG, comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = TRUE;
                    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                    MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
                curr_cnt -= send_subtree_cnt;
            }
            mask >>= 1;
        }
        
        /* copy local data into recvbuf */
        position = 0;
        if (recvbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Unpack_impl(tmp_buf, tmp_buf_size, &position, recvbuf,
                                         recvcnt, recvtype);
            if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        }
    }
#endif /* MPID_HAS_HETERO */
    
 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}



/* not declared static because a machine-specific function may call this one in some cases */
#undef FUNCNAME
#define FUNCNAME MPIR_Scatter_inter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Scatter_inter ( 
	void *sendbuf, 
	int sendcnt, 
	MPI_Datatype sendtype, 
	void *recvbuf, 
	int recvcnt, 
	MPI_Datatype recvtype, 
	int root, 
	MPID_Comm *comm_ptr,
        int *errflag )
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
    MPID_Comm *newcomm_ptr = NULL;
    MPI_Comm comm;
    MPIU_CHKLMEM_DECL(1);

    if (root == MPI_PROC_NULL) {
        /* local processes other than root do nothing */
        return MPI_SUCCESS;
    }
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_ENTER( comm_ptr );
    
    comm        = comm_ptr->handle;
    remote_size = comm_ptr->remote_size; 
    local_size  = comm_ptr->local_size; 

    if (root == MPI_ROOT) {
        MPID_Datatype_get_size_macro(sendtype, sendtype_size);
        nbytes = sendtype_size * sendcnt * remote_size;
    }
    else {
        /* remote side */
        MPID_Datatype_get_size_macro(recvtype, recvtype_size);
        nbytes = recvtype_size * recvcnt * local_size;
    }

    if (nbytes < MPIR_PARAM_SCATTER_INTER_SHORT_MSG_SIZE) {
        if (root == MPI_ROOT) {
            /* root sends all data to rank 0 on remote group and returns */
            mpi_errno = MPIC_Send_ft(sendbuf, sendcnt*remote_size,
                                     sendtype, 0, MPIR_SCATTER_TAG, comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = TRUE;
                MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            goto fn_exit;
        }
        else {
            /* remote group. rank 0 receives data from root. need to
               allocate temporary buffer to store this data. */
            
            rank = comm_ptr->rank;
            
            if (rank == 0) {
                MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);

                MPID_Datatype_get_extent_macro(recvtype, extent);
		MPID_Ensure_Aint_fits_in_pointer(extent*recvcnt*local_size);
		MPID_Ensure_Aint_fits_in_pointer(MPI_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
						 sendcnt*remote_size*extent);

                MPIU_CHKLMEM_MALLOC(tmp_buf, void *, recvcnt*local_size*(MPIR_MAX(extent,true_extent)), mpi_errno, "tmp_buf");
                
                /* adjust for potential negative lower bound in datatype */
                tmp_buf = (void *)((char*)tmp_buf - true_lb);

                mpi_errno = MPIC_Recv_ft(tmp_buf, recvcnt*local_size,
                                         recvtype, root,
                                         MPIR_SCATTER_TAG, comm, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = TRUE;
                    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                    MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
            
            /* Get the local intracommunicator */
            if (!comm_ptr->local_comm)
                MPIR_Setup_intercomm_localcomm( comm_ptr );
            
            newcomm_ptr = comm_ptr->local_comm;
            
            /* now do the usual scatter on this intracommunicator */
            mpi_errno = MPIR_Scatter_impl(tmp_buf, recvcnt, recvtype,
                                          recvbuf, recvcnt, recvtype, 0,
                                          newcomm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = TRUE;
                MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }
    else {
        /* long message. use linear algorithm. */
        if (root == MPI_ROOT) {
            MPID_Datatype_get_extent_macro(sendtype, extent);
            for (i=0; i<remote_size; i++) {
                mpi_errno = MPIC_Send_ft(((char *)sendbuf+sendcnt*i*extent), 
                                         sendcnt, sendtype, i,
                                         MPIR_SCATTER_TAG, comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag = TRUE;
                    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                    MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        }
        else {
            mpi_errno = MPIC_Recv_ft(recvbuf,recvcnt,recvtype,root,
                                     MPIR_SCATTER_TAG,comm,&status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = TRUE;
                MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**fail");
                MPIU_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

 fn_exit:
    MPIU_CHKLMEM_FREEALL();
    /* check if multiple threads are calling this collective function */
    MPIDU_ERR_CHECK_MULTIPLE_THREADS_EXIT( comm_ptr );
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag)
        MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**coll_fail");
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}


/* MPIR_Scatter performs an scatter using point-to-point messages.
   This is intended to be used by device-specific implementations of
   scatter.  In all other cases MPIR_Scatter_impl should be used. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scatter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                 void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                 int root, MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;
        
    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
        /* intracommunicator */
        mpi_errno = MPIR_Scatter_intra(sendbuf, sendcnt, sendtype,
                                       recvbuf, recvcnt, recvtype, root,
                                       comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        /* intercommunicator */ 
        mpi_errno = MPIR_Scatter_inter(sendbuf, sendcnt, sendtype,
                                       recvbuf, recvcnt, recvtype, root,
                                       comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
  
 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}

/* MPIR_Scatter_impl should be called by any internal component that
   would otherwise call MPI_Scatter.  This differs from MPIR_Scatter
   in that this will call the coll_fns version if it exists.  This
   function replaces NMPI_Scatter. */
#undef FUNCNAME
#define FUNCNAME MPIR_Scatter_impl
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_Scatter_impl(void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                      void *recvbuf, int recvcnt, MPI_Datatype recvtype,
                      int root, MPID_Comm *comm_ptr, int *errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->coll_fns != NULL && comm_ptr->coll_fns->Scatter != NULL) {
	mpi_errno = comm_ptr->coll_fns->Scatter(sendbuf, sendcnt, sendtype,
                                                recvbuf, recvcnt, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    } else {
        mpi_errno = MPIR_Scatter(sendbuf, sendcnt, sendtype,
                                 recvbuf, recvcnt, recvtype, root, comm_ptr, errflag);
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}
#endif

#undef FUNCNAME
#define FUNCNAME MPI_Scatter
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
/*@

MPI_Scatter - Sends data from one process to all other processes in a 
communicator

Input Parameters:
+ sendbuf - address of send buffer (choice, significant 
only at 'root') 
. sendcount - number of elements sent to each process 
(integer, significant only at 'root') 
. sendtype - data type of send buffer elements (significant only at 'root') 
(handle) 
. recvcount - number of elements in receive buffer (integer) 
. recvtype - data type of receive buffer elements (handle) 
. root - rank of sending process (integer) 
- comm - communicator (handle) 

Output Parameter:
. recvbuf - address of receive buffer (choice) 

.N ThreadSafe

.N Fortran

.N Errors
.N MPI_SUCCESS
.N MPI_ERR_COMM
.N MPI_ERR_COUNT
.N MPI_ERR_TYPE
.N MPI_ERR_BUFFER
@*/
int MPI_Scatter(void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
		void *recvbuf, int recvcnt, MPI_Datatype recvtype, int root, 
		MPI_Comm comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPID_Comm *comm_ptr = NULL;
    int errflag = FALSE;
    MPID_MPI_STATE_DECL(MPID_STATE_MPI_SCATTER);

    MPIR_ERRTEST_INITIALIZED_ORDIE();
    
    MPIU_THREAD_CS_ENTER(ALLFUNC,);
    MPID_MPI_COLL_FUNC_ENTER(MPID_STATE_MPI_SCATTER);

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
	    MPID_Datatype *sendtype_ptr=NULL, *recvtype_ptr=NULL;
	    int rank;

            MPID_Comm_valid_ptr( comm_ptr, mpi_errno );
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;

	    if (comm_ptr->comm_kind == MPID_INTRACOMM) {
		MPIR_ERRTEST_INTRA_ROOT(comm_ptr, root, mpi_errno);

                rank = comm_ptr->rank;
                if (rank == root) {
                    MPIR_ERRTEST_COUNT(sendcnt, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcnt,sendtype,mpi_errno);
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcnt, mpi_errno);

                    /* catch common aliasing cases */
                    if (recvbuf != MPI_IN_PLACE && sendtype == recvtype && sendcnt == recvcnt && recvcnt != 0)
                        MPIR_ERRTEST_ALIAS_COLL(sendbuf,recvbuf,mpi_errno);
                }
                else 
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcnt, mpi_errno);

                if (recvbuf != MPI_IN_PLACE) {
                    MPIR_ERRTEST_COUNT(recvcnt, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcnt,recvtype,mpi_errno);
                }
            }

            if (comm_ptr->comm_kind == MPID_INTERCOMM) {
		MPIR_ERRTEST_INTER_ROOT(comm_ptr, root, mpi_errno);

                if (root == MPI_ROOT) {
                    MPIR_ERRTEST_COUNT(sendcnt, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(sendtype, "sendtype", mpi_errno);
                    if (HANDLE_GET_KIND(sendtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(sendtype, sendtype_ptr);
                        MPID_Datatype_valid_ptr( sendtype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( sendtype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_SENDBUF_INPLACE(sendbuf, sendcnt, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(sendbuf,sendcnt,sendtype,mpi_errno);
                }
                else if (root != MPI_PROC_NULL) {
                    MPIR_ERRTEST_COUNT(recvcnt, mpi_errno);
                    MPIR_ERRTEST_DATATYPE(recvtype, "recvtype", mpi_errno);
                    if (HANDLE_GET_KIND(recvtype) != HANDLE_KIND_BUILTIN) {
                        MPID_Datatype_get_ptr(recvtype, recvtype_ptr);
                        MPID_Datatype_valid_ptr( recvtype_ptr, mpi_errno );
                        MPID_Datatype_committed_ptr( recvtype_ptr, mpi_errno );
                    }
                    MPIR_ERRTEST_RECVBUF_INPLACE(recvbuf, recvcnt, mpi_errno);
                    MPIR_ERRTEST_USERBUFFER(recvbuf,recvcnt,recvtype,mpi_errno);                    
                }
	    }
    
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
        }
        MPID_END_ERROR_CHECKS;
    }
#   endif /* HAVE_ERROR_CHECKING */

    /* ... body of routine ...  */

    mpi_errno = MPIR_Scatter_impl(sendbuf, sendcnt, sendtype,
                                  recvbuf, recvcnt, recvtype, root,
                                  comm_ptr, &errflag);
    if (mpi_errno) goto fn_fail;

    /* ... end of body of routine ... */
    
  fn_exit:
    MPID_MPI_COLL_FUNC_EXIT(MPID_STATE_MPI_SCATTER);
    MPIU_THREAD_CS_EXIT(ALLFUNC,);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
#   ifdef HAVE_ERROR_CHECKING
    {
	mpi_errno = MPIR_Err_create_code(
	    mpi_errno, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**mpi_scatter",
	    "**mpi_scatter %p %d %D %p %d %D %d %C", sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
    }
#   endif
    mpi_errno = MPIR_Err_return_comm( comm_ptr, FCNAME, mpi_errno );
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}
