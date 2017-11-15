/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "coll_util.h"

/* A binomial tree broadcast algorithm.  Good for short messages, 
   Cost = lgp.alpha + n.lgp.beta */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_binomial
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_binomial(
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    int        rank, comm_size, src, dst;
    int        relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int nbytes=0;
    int recvd_size;
    MPI_Status status;
    int is_contig, is_homogeneous;
    MPI_Aint type_size;
    MPI_Aint position;
    void *tmp_buf=NULL;
    MPIR_CHKLMEM_DECL(1);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1) goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
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
    if (is_homogeneous)
        MPIR_Datatype_get_size_macro(datatype, type_size);
    else
	/* --BEGIN HETEROGENEOUS-- */
        MPIR_Pack_size_impl(1, datatype, &type_size);
        /* --END HETEROGENEOUS-- */

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit; /* nothing to do */

    if (!is_contig || !is_homogeneous)
    {
        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        /* TODO: Pipeline the packing and communication */
        position = 0;
        if (rank == root) {
            mpi_errno = MPIR_Pack_impl(buffer, count, datatype, tmp_buf, nbytes,
                                       &position);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        }
    }

    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

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
                                         MPIR_BCAST_TAG,comm_ptr, &status, errflag);
            else
                mpi_errno = MPIC_Recv(buffer,count,datatype,src,
                                         MPIR_BCAST_TAG,comm_ptr, &status, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }

            /* check that we received as much as we expected */
            MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
            /* recvd_size may not be accurate for packed heterogeneous data */
            if (is_homogeneous && recvd_size != nbytes) {
                if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
                MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
		      "**collective_size_mismatch",
		      "**collective_size_mismatch %d %d", recvd_size, nbytes );
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
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
                                         MPIR_BCAST_TAG,comm_ptr, errflag);
            else
                mpi_errno = MPIC_Send(buffer,count,datatype,dst,
                                         MPIR_BCAST_TAG,comm_ptr, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }
        mask >>= 1;
    }

    if (!is_contig || !is_homogeneous)
    {
        if (rank != root)
        {
            position = 0;
            mpi_errno = MPIR_Unpack_impl(tmp_buf, nbytes, &position, buffer,
                                         count, datatype);
            if (mpi_errno) MPIR_ERR_POP(mpi_errno);
            
        }
    }

fn_exit:
    MPIR_CHKLMEM_FREEALL();
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
