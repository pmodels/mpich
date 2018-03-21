/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "ibcast.h"

#undef FUNCNAME
#define FUNCNAME MPII_Ibcast_sched_test_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Ibcast_sched_test_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recv_size;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;

    MPIR_Get_count_impl(&ibcast_state->status, MPI_BYTE, &recv_size);
    if (ibcast_state->n_bytes != recv_size || ibcast_state->status.MPI_ERROR != MPI_SUCCESS) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**collective_size_mismatch",
                                         "**collective_size_mismatch %d %d", ibcast_state->n_bytes,
                                         recv_size);
    }

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Ibcast_sched_test_curr_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Ibcast_sched_test_curr_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;

    if (ibcast_state->n_bytes != ibcast_state->curr_bytes) {
        mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_RECOVERABLE,
                                         FCNAME, __LINE__, MPI_ERR_OTHER,
                                         "**collective_size_mismatch",
                                         "**collective_size_mismatch %d %d", ibcast_state->n_bytes,
                                         ibcast_state->curr_bytes);
    }

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Ibcast_sched_add_length
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Ibcast_sched_add_length(MPIR_Comm * comm, int tag, void *state)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint recv_size;
    struct MPII_Ibcast_state *ibcast_state = (struct MPII_Ibcast_state *) state;

    MPIR_Get_count_impl(&ibcast_state->status, MPI_BYTE, &recv_size);
    ibcast_state->curr_bytes += recv_size;

    return mpi_errno;
}


/* TODO it would be nice if we could refactor things to minimize
 * duplication between this and MPIR_Iscatter and algorithms.  We
 * can't use the MPIR_Iscatter algorithms as is without inducing an
 * extra copy in the noncontig case. */
/* This is a binomial scatter operation, but it does *not* take
 * typical scatter arguments.  At the moment this function always
 * scatters a buffer of nbytes starting at tmp_buf address. */
#undef FUNCNAME
#define FUNCNAME MPII_Iscatter_for_bcast_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Iscatter_for_bcast_sched(void *tmp_buf, int root, MPIR_Comm * comm_ptr, int nbytes,
                                  MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int scatter_size, curr_size, recv_size, send_size;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* The scatter algorithm divides the buffer into nprocs pieces and
     * scatters them among the processes. Root gets the first piece,
     * root+1 gets the second piece, and so forth. Uses the same
     * binomial tree algorithm as above. Ceiling division is used to
     * compute the size of each piece. This means some processes may
     * not get any data. For example if bufsize = 97 and nprocs = 16,
     * ranks 15 and 16 will get 0 data. On each process, the scattered
     * data is stored at the same offset in the buffer as it is on the
     * root process. */

    scatter_size = (nbytes + comm_size - 1) / comm_size;        /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0;    /* root starts with all the data */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;

            /* compute the exact recv_size to avoid writing this NBC
             * in callback style */
            recv_size = nbytes - (relative_rank * scatter_size);
            if (recv_size < 0)
                recv_size = 0;

            curr_size = recv_size;

            if (recv_size > 0) {
                mpi_errno = MPIR_Sched_recv(((char *) tmp_buf + relative_rank * scatter_size),
                                            recv_size, MPI_BYTE, src, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);
                MPIR_SCHED_BARRIER(s);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB upto (but not including) mask.  Because of the
     * "not including", we start by shifting mask back down one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0) {
                dst = rank + mask;
                if (dst >= comm_size)
                    dst -= comm_size;
                mpi_errno =
                    MPIR_Sched_send(((char *) tmp_buf + scatter_size * (relative_rank + mask)),
                                    send_size, MPI_BYTE, dst, comm_ptr, s);
                if (mpi_errno)
                    MPIR_ERR_POP(mpi_errno);

                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
