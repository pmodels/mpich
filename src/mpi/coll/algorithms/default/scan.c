/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* This is the default implementation of scan. The algorithm is:
   
   Algorithm: MPI_Scan

   We use a lgp recursive doubling algorithm. The basic algorithm is
   given below. (You can replace "+" with any other scan operator.)
   The result is stored in recvbuf.

 .vb
   recvbuf = sendbuf;
   partial_scan = sendbuf;
   mask = 0x1;
   while (mask < size) {
      dst = rank^mask;
      if (dst < size) {
         send partial_scan to dst;
         recv from dst into tmp_buf;
         if (rank > dst) {
            partial_scan = tmp_buf + partial_scan;
            recvbuf = tmp_buf + recvbuf;
         }
         else {
            if (op is commutative)
               partial_scan = tmp_buf + partial_scan;
            else {
               tmp_buf = partial_scan + tmp_buf;
               partial_scan = tmp_buf;
            }
         }
      }
      mask <<= 1;
   }  
 .ve

   End Algorithm: MPI_Scan
*/


#undef FUNCNAME
#define FUNCNAME MPIC_DEFAULT_Scan_generic
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int MPIC_DEFAULT_Scan_generic ( 
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    MPI_Status status;
    int        rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int mask, dst, is_commutative; 
    MPI_Aint true_extent, true_lb, extent;
    void *partial_scan, *tmp_buf;
    MPIR_Op *op_ptr;
    MPIR_CHKLMEM_DECL(2);
    
    if (count == 0) return MPI_SUCCESS;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* set op_errno to 0. stored in perthread structure */
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        per_thread->op_errno = 0;
    }

    if (HANDLE_GET_KIND(op) == HANDLE_KIND_BUILTIN) {
        is_commutative = 1;
    }
    else {
        MPIR_Op_get_ptr(op, op_ptr);
        if (op_ptr->kind == MPIR_OP_KIND__USER_NONCOMMUTE)
            is_commutative = 0;
        else
            is_commutative = 1;
    }
    
    /* need to allocate temporary buffer to store partial scan*/
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_CHKLMEM_MALLOC(partial_scan, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "partial_scan");

    /* This eventually gets malloc()ed as a temp buffer, not added to
     * any user buffers */
    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    /* adjust for potential negative lower bound in datatype */
    partial_scan = (void *)((char*)partial_scan - true_lb);
    
    /* need to allocate temporary buffer to store incoming data*/
    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count*(MPL_MAX(extent,true_extent)), mpi_errno, "tmp_buf");
    
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *)((char*)tmp_buf - true_lb);
    
    /* Since this is an inclusive scan, copy local contribution into
       recvbuf. */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   partial_scan, count, datatype);
    else 
        mpi_errno = MPIR_Localcopy(recvbuf, count, datatype,
                                   partial_scan, count, datatype);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    
    mask = 0x1;
    while (mask < comm_size) {
        dst = rank ^ mask;
        if (dst < comm_size) {
            /* Send partial_scan to dst. Recv into tmp_buf */
            mpi_errno = MPIC_Sendrecv(partial_scan, count, datatype,
                                         dst, MPIR_SCAN_TAG, tmp_buf,
                                         count, datatype, dst,
                                         MPIR_SCAN_TAG, comm_ptr,
                                         &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            
            if (rank > dst) {
		mpi_errno = MPIR_Reduce_local_impl( 
			   tmp_buf, partial_scan, count, datatype, op);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		mpi_errno = MPIR_Reduce_local_impl( 
			   tmp_buf, recvbuf, count, datatype, op);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                if (is_commutative) {
		    mpi_errno = MPIR_Reduce_local_impl( 
			       tmp_buf, partial_scan, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		}
                else {
		    mpi_errno = MPIR_Reduce_local_impl( 
			       partial_scan, tmp_buf, count, datatype, op);
                    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
		    mpi_errno = MPIR_Localcopy(tmp_buf, count, datatype,
					       partial_scan,
					       count, datatype);
		    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
                }
            }
        }
        mask <<= 1;
    }
    
    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno) {
            mpi_errno = per_thread->op_errno;
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
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
#define FUNCNAME MPIC_DEFAULT_Scan
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIC_DEFAULT_Scan(
    const void *sendbuf,
    void *recvbuf,
    int count,
    MPI_Datatype datatype,
    MPI_Op op,
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_CHKLMEM_DECL(3);
    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL, *localfulldata = NULL, *prefulldata = NULL;
    MPI_Aint  true_lb, true_extent, extent; 
    int noneed = 1; /* noneed=1 means no need to bcast tempbuf and 
                       reduce tempbuf & recvbuf */

    /* In order to use the SMP-aware algorithm, the "op" can be
       either commutative or non-commutative, but we require a
       communicator in which all the nodes contain processes with
       consecutive ranks. */

    if (!MPII_Comm_is_node_consecutive(comm_ptr)) {
        /* We can't use the SMP-aware algorithm, use the generic one */
        return MPIC_DEFAULT_Scan_generic(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }
    
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_Ensure_Aint_fits_in_pointer(count * MPL_MAX(extent, true_extent));

    MPIR_CHKLMEM_MALLOC(tempbuf, void *, count*(MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer");
    tempbuf = (void *)((char*)tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_CHKLMEM_MALLOC(prefulldata, void *, count*(MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan");
        prefulldata = (void *)((char*)prefulldata - true_lb);

        if (comm_ptr->node_comm != NULL) {
            MPIR_CHKLMEM_MALLOC(localfulldata, void *, count*(MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan");
            localfulldata = (void *)((char*)localfulldata - true_lb);
        }
    }
  
    /* perform intranode scan to get temporary result in recvbuf. if there is only 
       one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPID_Scan(sendbuf, recvbuf, count, datatype, 
                                   op, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (sendbuf != MPI_IN_PLACE)
    {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype,
                                   recvbuf, count, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    /* get result from local node's last processor which 
       contains the reduce result of the whole node. Name it as
       localfulldata. For example, localfulldata from node 1 contains
       reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL)
    {
        mpi_errno = MPIC_Recv(localfulldata, count, datatype,
                                 comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG, 
                                 comm_ptr->node_comm, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (comm_ptr->node_roots_comm == NULL && 
             comm_ptr->node_comm != NULL && 
             MPIR_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1)
    {
        mpi_errno = MPIC_Send(recvbuf, count, datatype,
                                 0, MPIR_SCAN_TAG, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }
    else if (comm_ptr->node_roots_comm != NULL)
    {
        localfulldata = recvbuf;
    }

    /* do scan on localfulldata to prefulldata. for example, 
       prefulldata on rank 4 contains reduce result of ranks 
       1,2,3,4,5,6. it will be sent to rank 7 which is master 
       process of node 3. */
    if (comm_ptr->node_roots_comm != NULL)
    {
        mpi_errno = MPID_Scan(localfulldata, prefulldata, count, datatype,
                                   op, comm_ptr->node_roots_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        if (MPIR_Get_internode_rank(comm_ptr, rank) !=
            comm_ptr->node_roots_comm->local_size-1)
        {
            mpi_errno = MPIC_Send(prefulldata, count, datatype,
                                     MPIR_Get_internode_rank(comm_ptr, rank) + 1,
                                     MPIR_SCAN_TAG, comm_ptr->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        if (MPIR_Get_internode_rank(comm_ptr, rank) != 0)
        {
            mpi_errno = MPIC_Recv(tempbuf, count, datatype,
                                     MPIR_Get_internode_rank(comm_ptr, rank) - 1,
                                     MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status, errflag);
            noneed = 0;
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
    }

    /* now tempbuf contains all the data needed to get the correct 
       scan result. for example, to node 3, it will have reduce result 
       of rank 1,2,3,4,5,6 in tempbuf. 
       then we should broadcast this result in the local node, and
       reduce it with recvbuf to get final result if nessesary. */

    if (comm_ptr->node_comm != NULL) {
        mpi_errno = MPID_Bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
            mpi_errno = MPID_Bcast(tempbuf, count, datatype, 0, 
					comm_ptr->node_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

	mpi_errno = MPIR_Reduce_local_impl( tempbuf, recvbuf, 
					    count, datatype, op );
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
