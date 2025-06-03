/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
int MPII_Scatter_for_bcast(void *buffer ATTRIBUTE((unused)),
                           MPI_Aint count ATTRIBUTE((unused)),
                           MPI_Datatype datatype ATTRIBUTE((unused)),
                           int root,
                           MPIR_Comm * comm_ptr,
                           MPI_Aint nbytes, void *tmp_buf, int is_contig, MPIR_Errflag_t errflag)
{
    MPI_Status status;
    int rank, group_size, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint scatter_size, recv_size = 0;
    MPI_Aint curr_size, send_size;

    group_size = comm_ptr->local_size;
    rank = comm_ptr->rank;
    relative_rank = (rank >= root) ? rank - root : rank - root + group_size;

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

    scatter_size = (nbytes + group_size - 1) / group_size;        /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0;    /* root starts with all the
                                                 * data */

    mask = 0x1;
    while (mask < group_size) {
        if (relative_rank & mask) {
            src = rank - mask;
            if (src < 0)
                src += group_size;
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
                                      recv_size, MPIR_BYTE_INTERNAL, src, MPIR_BCAST_TAG, comm_ptr,
                                      &status);
                MPIR_ERR_CHECK(mpi_errno);
                /* query actual size of data received */
                MPIR_Get_count_impl(&status, MPIR_BYTE_INTERNAL, &curr_size);
            }
            break;
        }
        mask <<= 1;
    }

    /* This process is responsible for all processes that have bits
     * set from the LSB up to (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < group_size) {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0) {
                dst = rank + mask;
                if (dst >= group_size)
                    dst -= group_size;
                mpi_errno = MPIC_Send(((char *) tmp_buf +
                                       scatter_size * (relative_rank + mask)),
                                      send_size, MPIR_BYTE_INTERNAL, dst, MPIR_BCAST_TAG, comm_ptr,
                                      errflag);
                MPIR_ERR_CHECK(mpi_errno);

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
int MPII_Scatter_for_bcast_group(void *buffer ATTRIBUTE((unused)), 
                            MPI_Aint count ATTRIBUTE((unused)), 
                            MPI_Datatype datatype ATTRIBUTE((unused)),
                           int root, MPIR_Comm * comm_ptr, int* group, int group_size, MPI_Aint nbytes, void *tmp_buf,
                           int is_contig, MPIR_Errflag_t errflag) 
{
    MPI_Status status;
    int rank, src, dst;
    int relative_rank, mask;
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint scatter_size, recv_size = 0;
    MPI_Aint curr_size, send_size;
    
    rank = comm_ptr->rank;
    int group_rank, group_root;
    bool found_rank_in_group;

    found_rank_in_group = find_local_rank_linear(group, group_size, rank, root, &group_rank, &group_root);

    
    relative_rank = (group_rank >= group_root) ? group_rank - group_root : group_rank - group_root + group_size;

    scatter_size = (nbytes + group_size - 1) / group_size;        /* ceiling division */
    curr_size = (rank == root) ? nbytes : 0;    /* root starts with all the
                                                 * data */

    mask = 0x1;
    while (mask < group_size) {
        if (relative_rank & mask) {
            src = group_rank - mask;
            if (src < 0)
                src += group_size;
            recv_size = nbytes - relative_rank * scatter_size;
            
            if (recv_size <= 0) {
                curr_size = 0;  
            } else {
                mpi_errno = MPIC_Recv(((char *) tmp_buf +
                                       relative_rank * scatter_size),
                                      recv_size, MPI_BYTE, group[src], MPIR_BCAST_TAG, comm_ptr, &status);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);
                if (mpi_errno) {
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
     * set from the LSB up to (but not including) mask.  Because of
     * the "not including", we start by shifting mask back down
     * one. */

    mask >>= 1;
    while (mask > 0) {
        if (relative_rank + mask < group_size) {
            send_size = curr_size - scatter_size * mask;
            /* mask is also the size of this process's subtree */

            if (send_size > 0) {
                dst = group_rank + mask;
                if (dst >= group_size)
                    dst -= group_size;
                mpi_errno = MPIC_Send(((char *) tmp_buf +
                                       scatter_size * (relative_rank + mask)),
                                      send_size, MPI_BYTE, group[dst], MPIR_BCAST_TAG, comm_ptr, errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag, mpi_errno_ret);

                curr_size -= send_size;
            }
        }
        mask >>= 1;
    }

    return mpi_errno_ret;
}

/* A linear search function to find the local rank of a rank within a subgroup. 
    This also retrieves the local rank of the root process for that group. Returns 0 on failure and 1 on success. */
bool find_local_rank_linear(int* group, int group_size, int rank, int root, int* group_rank, int* group_root) {
    
    if (!group) return 0;

    bool found_rank = 0;
    bool found_root = 0;

    for (int i = 0; i < group_size; i++) {
        if (group[i] == rank) {
            *group_rank = i;
            found_rank = 1; 
        }
        if (group[i] == root) {
            *group_root = i;
            found_root = 1;
        }
    }

    return found_rank && found_root;
}
