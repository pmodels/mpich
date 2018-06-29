/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "bcast.h"

/* FIXME it would be nice if we could refactor things to minimize
   duplication between this and other MPIR_Scatter algorithms.  We can't use
   MPIR_Scatter algorithms as is without inducing an extra copy in the noncontig case. */
/* There are additional arguments included here that are unused because we
   always assume that the noncontig case has been packed into a contig case by
   the caller for now.  Once we start handling noncontig data at the upper level
   we can start handling it here.

   At the moment this function always scatters a buffer of nbytes starting at
   tmp_buf address. */
#undef FUNCNAME
#define FUNCNAME MPII_Scatter_for_bcast
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Scatter_for_bcast(void *buffer ATTRIBUTE((unused)),
                           int count ATTRIBUTE((unused)),
                           MPI_Datatype datatype ATTRIBUTE((unused)),
                           int root,
                           MPIR_Comm * comm_ptr,
                           int nbytes, void *tmp_buf, int is_contig, MPIR_Errflag_t * errflag)
{
    MPI_Status status;
    int rank, comm_size, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int scatter_size, recv_size = 0;
    MPI_Aint curr_size, send_size;

    comm_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + comm_size;

    /* use long message algorithm: binomial tree scatter followed by an allgather */

    /* The scatter algorithm divides the buffer into nprocs pieces and
     * scatters them among the processes. Root gets the first piece,
     * root+1 gets the second piece, and so forth. Uses the same binomial
     * tree algorithm as above. Ceiling division
     * is used to compute the size of each piece. This means some
     * processes may not get any data. For example if bufsize = 97 and
     * nprocs = 16, ranks 15 and 16 will get 0 data. On each process, the
     * scattered data is stored at the same offset in the buffer as it is
     * on the root process. */

    scatter_size = (nbytes + comm_size - 1) / comm_size;        /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0;    /* root starts with all the
                                                 * data */

    mask = 0x1;
    while (mask < comm_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += comm_size;
            recv_size = nbytes - relative_rank * scatter_size;
            /* recv_size is larger than what might actually be sent by the
             * sender. We don't need compute the exact value because MPI
             * allows you to post a larger recv. */
            if (recv_size <= 0) {
                curr_size = 0;  /* this process doesn't receive any data
                                 * because of uneven division */
            } else {
                mpi_errno = MPIC_Recv(((char *) tmp_buf +
                                       relative_rank * scatter_size),
                                      recv_size, MPI_BYTE, src,
                                      MPIR_BCAST_TAG, comm_ptr, &status, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                    curr_size = 0;
                } else
                    /* query actual size of data received */
                    MPIR_Get_count_impl(&status, MPI_BYTE, &curr_size);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB upto (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < comm_size) {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0) {
                dst = rank + mask;
                if (dst >= comm_size)
                    dst -= comm_size;
                mpi_errno = MPIC_Send(((char *) tmp_buf +
                                       scatter_size * (relative_rank + mask)),
                                      send_size, MPI_BYTE, dst, MPIR_BCAST_TAG, comm_ptr, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }

                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}
