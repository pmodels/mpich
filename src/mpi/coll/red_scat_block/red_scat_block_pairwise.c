/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */


/* This implementation of MPI_Reduce_scatter_block was obtained by taking
   the implementation of MPI_Reduce_scatter from red_scat.c and replacing 
   recvcnts[i] with recvcount everywhere. */


#include "mpiimpl.h"
#include "../collutil.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Reduce_scatter_block_pairwise
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)

/* not declared static because a machine-specific function may call this one in some cases */
int MPIR_Reduce_scatter_block_pairwise ( 
    const void *sendbuf, 
    void *recvbuf, 
    int recvcount, 
    MPI_Datatype datatype, 
    MPI_Op op, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag )
{
    int   rank, comm_size, i;
    MPI_Aint extent, true_extent, true_lb; 
    int  *disps;
    void *tmp_recvbuf;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int src, dst;
    int is_commutative;
    MPIR_Op *op_ptr;
    MPIR_CHKLMEM_DECL(5);

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

    if (recvcount == 0) {
        goto fn_exit;
    }

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    
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

    MPIR_CHKLMEM_MALLOC(disps, int *, comm_size * sizeof(int), mpi_errno, "disps", MPL_MEM_BUFFER);

    for (i=0; i<comm_size; i++) {
        disps[i] = i*recvcount;
    }

    /* commutative and long message, or noncommutative and long message.
       use (p-1) pairwise exchanges */ 
    
    if (sendbuf != MPI_IN_PLACE) {
        /* copy local data into recvbuf */
        mpi_errno = MPIR_Localcopy(((char *)sendbuf+disps[rank]*extent),
                                   recvcount, datatype, recvbuf,
                                   recvcount, datatype);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
    /* allocate temporary buffer to store incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, void *, recvcount*(MPL_MAX(true_extent,extent))+1, mpi_errno, "tmp_recvbuf", MPL_MEM_BUFFER);
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *)((char*)tmp_recvbuf - true_lb);
    
    for (i=1; i<comm_size; i++) {
        src = (rank - i + comm_size) % comm_size;
        dst = (rank + i) % comm_size;
        
        /* send the data that dst needs. recv data that this process
           needs from src into tmp_recvbuf */
        if (sendbuf != MPI_IN_PLACE) 
            mpi_errno = MPIC_Sendrecv(((char *)sendbuf+disps[dst]*extent),
                                         recvcount, datatype, dst,
                                         MPIR_REDUCE_SCATTER_BLOCK_TAG, tmp_recvbuf,
                                         recvcount, datatype, src,
                                         MPIR_REDUCE_SCATTER_BLOCK_TAG, comm_ptr,
                                         MPI_STATUS_IGNORE, errflag);
        else
            mpi_errno = MPIC_Sendrecv(((char *)recvbuf+disps[dst]*extent),
                                         recvcount, datatype, dst,
                                         MPIR_REDUCE_SCATTER_BLOCK_TAG, tmp_recvbuf,
                                         recvcount, datatype, src,
                                         MPIR_REDUCE_SCATTER_BLOCK_TAG, comm_ptr,
                                         MPI_STATUS_IGNORE, errflag);
        
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        
        if (is_commutative || (src < rank)) {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Reduce_local( tmp_recvbuf, recvbuf, 
                                                 recvcount, datatype, op); 
            }
            else {
                mpi_errno = MPIR_Reduce_local( 
                      tmp_recvbuf, ((char *)recvbuf+disps[rank]*extent), 
                      recvcount, datatype, op ); 
                /* we can't store the result at the beginning of
                   recvbuf right here because there is useful data
                   there that other process/processes need. at the
                   end, we will copy back the result to the
                   beginning of recvbuf. */
            }
        }
        else {
            if (sendbuf != MPI_IN_PLACE) {
                mpi_errno = MPIR_Reduce_local(recvbuf, tmp_recvbuf, 
                                                   recvcount, datatype,op); 
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcount, 
                                           datatype, recvbuf,
                                           recvcount, datatype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
            else {
                mpi_errno = MPIR_Reduce_local( 
                           ((char *)recvbuf+disps[rank]*extent),
                           tmp_recvbuf, recvcount, datatype, op);
                /* copy result back into recvbuf */
                mpi_errno = MPIR_Localcopy(tmp_recvbuf, recvcount, 
                                           datatype, 
                                           ((char *)recvbuf +
                                            disps[rank]*extent), 
                                           recvcount, datatype);
                if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            }
        }
    }
    
    /* if MPI_IN_PLACE, move output data to the beginning of
       recvbuf. already done for rank 0. */
    if ((sendbuf == MPI_IN_PLACE) && (rank != 0)) {
        mpi_errno = MPIR_Localcopy(((char *)recvbuf +
                                    disps[rank]*extent),  
                                   recvcount, datatype, 
                                   recvbuf, 
                                   recvcount, datatype); 
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    
fn_exit:
    MPIR_CHKLMEM_FREEALL();

    {
        MPIR_Per_thread_t *per_thread = NULL;
        int err = 0;

        MPID_THREADPRIV_KEY_GET_ADDR(MPIR_ThreadInfo.isThreaded, MPIR_Per_thread_key,
                                     MPIR_Per_thread, per_thread, &err);
        MPIR_Assert(err == 0);
        if (per_thread->op_errno)
            mpi_errno = per_thread->op_errno;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
