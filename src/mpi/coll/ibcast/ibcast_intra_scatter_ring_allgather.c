/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "ibcast.h"

/*
 * Broadcast based on a scatter followed by an allgather.

 * We first scatter the buffer using a binomial tree algorithm. This
 * costs lgp.alpha + n.((p-1)/p).beta
 *
 * If the datatype is contiguous, we treat the data as bytes and
 * divide (scatter) it among processes by using ceiling division.
 * For the noncontiguous cases, we first pack the data into a
 * temporary buffer by using MPI_Pack, scatter it as bytes, and
 * unpack it after the allgather.
 *
 * We use a ring algorithm for the allgather, which takes p-1 steps.
 * This may perform better than recursive doubling for long messages
 * and medium-sized non-power-of-two messages.
 *
 * Total Cost = (lgp+p-1).alpha + 2.n.((p-1)/p).beta
 */
#undef FUNCNAME
#define FUNCNAME MPIR_Ibcast_sched_intra_scatter_ring_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ibcast_sched_intra_scatter_ring_allgather(void *buffer, int count, MPI_Datatype datatype,
                                                   int root, MPIR_Comm * comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int comm_size, rank;
    int is_contig, type_size, nbytes;
    int scatter_size, curr_size;
    int i, j, jnext, left, right;
    MPI_Aint true_extent, true_lb;
    void *tmp_buf = NULL;

    struct MPII_Ibcast_state *ibcast_state;
    MPIR_SCHED_CHKPMEM_DECL(4);

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;

    /* If there is only one process, return */
    if (comm_size == 1)
        goto fn_exit;

    if (HANDLE_GET_KIND(datatype) == HANDLE_KIND_BUILTIN)
        is_contig = 1;
    else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    MPIR_SCHED_CHKPMEM_MALLOC(ibcast_state, struct MPII_Ibcast_state *,
                              sizeof(struct MPII_Ibcast_state), mpi_errno, "MPI_Status",
                              MPL_MEM_BUFFER);
    MPIR_Datatype_get_size_macro(datatype, type_size);
    nbytes = type_size * count;
    ibcast_state->n_bytes = nbytes;
    ibcast_state->curr_bytes = 0;
    if (is_contig) {
        /* contiguous, no need to pack. */
        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

        tmp_buf = (char *) buffer + true_lb;
    } else {
        MPIR_SCHED_CHKPMEM_MALLOC(tmp_buf, void *, nbytes, mpi_errno, "tmp_buf", MPL_MEM_BUFFER);

        if (rank == root) {
            mpi_errno = MPIR_Sched_copy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, s);
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);
            MPIR_SCHED_BARRIER(s);
        }
    }

    mpi_errno = MPII_Iscatter_for_bcast_sched(tmp_buf, root, comm_ptr, nbytes, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* this is the block size used for the scatter operation */
    scatter_size = (nbytes + comm_size - 1) / comm_size;        /* ceiling division */

    /* curr_size is the amount of data that this process now has stored in
     * buffer at byte offset (rank*scatter_size) */
    curr_size = MPL_MIN(scatter_size, (nbytes - (rank * scatter_size)));
    if (curr_size < 0)
        curr_size = 0;
    /* curr_size bytes already inplace */
    ibcast_state->curr_bytes = curr_size;

    /* long-message allgather or medium-size but non-power-of-two. use ring algorithm. */

    left = (comm_size + rank - 1) % comm_size;
    right = (rank + 1) % comm_size;

    j = rank;
    jnext = left;
    for (i = 1; i < comm_size; i++) {
        int left_count, right_count, left_disp, right_disp, rel_j, rel_jnext;

        rel_j = (j - root + comm_size) % comm_size;
        rel_jnext = (jnext - root + comm_size) % comm_size;
        left_count = MPL_MIN(scatter_size, (nbytes - rel_jnext * scatter_size));
        if (left_count < 0)
            left_count = 0;
        left_disp = rel_jnext * scatter_size;
        right_count = MPL_MIN(scatter_size, (nbytes - rel_j * scatter_size));
        if (right_count < 0)
            right_count = 0;
        right_disp = rel_j * scatter_size;

        mpi_errno = MPIR_Sched_send(((char *) tmp_buf + right_disp),
                                    right_count, MPI_BYTE, right, comm_ptr, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        /* sendrecv, no barrier here */
        mpi_errno = MPIR_Sched_recv_status(((char *) tmp_buf + left_disp),
                                           left_count, MPI_BYTE, left, comm_ptr,
                                           &ibcast_state->status, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);
        mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_add_length, ibcast_state, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
        MPIR_SCHED_BARRIER(s);

        j = jnext;
        jnext = (comm_size + jnext - 1) % comm_size;
    }
    mpi_errno = MPIR_Sched_cb(&MPII_Ibcast_sched_test_curr_length, ibcast_state, s);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!is_contig && rank != root) {
        mpi_errno = MPIR_Sched_copy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, s);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_CHKPMEM_COMMIT(s);
  fn_exit:
    return mpi_errno;
  fn_fail:
    MPIR_SCHED_CHKPMEM_REAP(s);
    goto fn_exit;
}
