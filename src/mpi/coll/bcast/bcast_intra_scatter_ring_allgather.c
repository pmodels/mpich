/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bcast.h"

/* Algorithm: Broadcast based on a scatter followed by an allgather.
 *
 * We first scatter the buffer using a binomial tree algorithm. This costs
 * lgp.alpha + n.((p-1)/p).beta
 * If the datatype is contiguous and the communicator is homogeneous,
 * we treat the data as bytes and divide (scatter) it among processes
 * by using ceiling division. For the noncontiguous or heterogeneous
 * cases, we first pack the data into a temporary buffer by using
 * MPI_Pack, scatter it as bytes, and unpack it after the allgather.
 *
 * We use a ring algorithm for the allgather, which takes p-1 steps.
 * This may perform better than recursive doubling for long messages and
 * medium-sized non-power-of-two messages.
 * Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Bcast_intra_scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Bcast_intra_scatter_ring_allgather(
    void *buffer, 
    int count, 
    MPI_Datatype datatype, 
    int root, 
    MPIR_Comm *comm_ptr,
    MPIR_Errflag_t *errflag)
{
    int rank, comm_size;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int scatter_size;
    int j, i, is_contig, is_homogeneous;
    MPI_Aint nbytes, type_size, position;
    int left, right, jnext;
    int curr_size = 0;
    void *tmp_buf;
    int recvd_size;
    MPI_Status status;
    MPI_Aint true_extent, true_lb;
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

    if (is_contig && is_homogeneous)
    {
        /* contiguous and homogeneous. no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    }
    else
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

    scatter_size = (nbytes + comm_size - 1)/comm_size; /* ceiling division */

    mpi_errno = MPII_Scatter_for_bcast(buffer, count, datatype, root, comm_ptr,
                                  nbytes, tmp_buf, is_contig, is_homogeneous, errflag);
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
        MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */ 

    /* Calculate how much data we already have */
    curr_size = MPL_MIN(scatter_size,
                         nbytes - ((rank - root + comm_size) % comm_size) * scatter_size);
    if (curr_size < 0)
        curr_size = 0;

    left  = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;
    j     = rank;
    jnext = left;
    for (i=1; i<comm_size; i++)
    {
        int left_count, right_count, left_disp, right_disp, rel_j, rel_jnext;

        rel_j     = (j     - root + comm_size) % comm_size;
        rel_jnext = (jnext - root + comm_size) % comm_size;
        left_count = MPL_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPL_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIC_Sendrecv((char *)tmp_buf + right_disp, right_count,
                                     MPI_BYTE, right, MPIR_BCAST_TAG,
                                     (char *)tmp_buf + left_disp, left_count,
                                     MPI_BYTE, left, MPIR_BCAST_TAG,
                                     comm_ptr, &status, errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag = MPIR_ERR_GET_CLASS(mpi_errno);
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        curr_size += recvd_size;
        j     = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }

    /* check that we received as much as we expected */
    /* recvd_size may not be accurate for packed heterogeneous data */
    if (is_homogeneous && curr_size != nbytes) {
        if (*errflag == MPIR_ERR_NONE) *errflag = MPIR_ERR_OTHER;
        MPIR_ERR_SET2(mpi_errno, MPI_ERR_OTHER,
		      "**collective_size_mismatch",
		      "**collective_size_mismatch %d %d", curr_size, nbytes );
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
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
