/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

#include <math.h>

/*
 * Algorithm: Recursive Multiplying
 *
 * This algorithm is a generalization of the recursive doubling algorithm,
 * and it has three stages. In the first stage, ranks above the nearest
 * power of k less than or equal to comm_size collapse their data to the
 * lower ranks. The main stage proceeds with power-of-k ranks. In the main
 * stage, ranks exchange data within groups of size k in rounds with
 * increasing distance (k, k^2, ...). Lastly, those in the main stage
 * disperse the result back to the excluded ranks. Setting k according
 * to the network hierarchy (e.g., the number of NICs in a node) can
 * improve performance.
 */


int MPIR_Allreduce_intra_recursive_multiplying(const void *sendbuf,
                                               void *recvbuf,
                                               MPI_Aint count,
                                               MPI_Datatype datatype,
                                               MPI_Op op,
                                               MPIR_Comm * comm_ptr,
                                               int coll_group, const int k, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    /* Ensure the op is commutative */

    int comm_size, rank, virt_rank;
    MPIR_COLL_RANK_SIZE(comm_ptr, coll_group, rank, comm_size);
    virt_rank = rank;

    /* get nearest power-of-two less than or equal to comm_size */
    int power = (int) (log(comm_size) / log(k));
    int pofk = (int) lround(pow(k, power));

    MPIR_CHKLMEM_DECL(2);
    void *tmp_buf;

    /*Allocate for nb requests */
    MPIR_Request **reqs;
    int num_reqs = 0;
    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **, (2 * (k - 1) * sizeof(MPIR_Request *)), mpi_errno,
                        "reqs", MPL_MEM_BUFFER);

    /* need to allocate temporary buffer to store incoming data */
    MPI_Aint true_extent, true_lb, extent;
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPI_Aint single_node_data_size = extent * count - (extent - true_extent);

    MPIR_CHKLMEM_MALLOC(tmp_buf, void *, (k - 1) * count * single_node_data_size, mpi_errno,
                        "temporary buffer", MPL_MEM_BUFFER);

    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);


    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* PRE-STAGE: ranks over pofk collapse their data into the others
     * using reverse round-robin */
    if (pofk < comm_size) {
        /* Non-pofk only works with communicative ops */
        MPIR_Assert(MPIR_Op_is_commutative(op));

        if (rank >= pofk) {
            /* This process is outside the main algorithm so send data */
            int pre_dst = rank % pofk;
            /* This is follower so send data */
            mpi_errno = MPIC_Send(recvbuf, count, datatype,
                                  pre_dst, MPIR_ALLREDUCE_TAG, comm_ptr, coll_group, errflag);
            MPIR_ERR_CHECK(mpi_errno);
            /* Set virtual rank so this rank is not used in main stage */
            virt_rank = -1;
        } else {
            /* Receive data from all those greater than pofk */
            for (int pre_src = (rank % pofk) + pofk; pre_src < comm_size; pre_src += pofk) {
                mpi_errno = MPIC_Irecv(((char *) tmp_buf) + num_reqs * count * extent, count,
                                       datatype, pre_src, MPIR_ALLREDUCE_TAG, comm_ptr, coll_group,
                                       &reqs[num_reqs]);
                MPIR_ERR_CHECK(mpi_errno);
                num_reqs++;
            }

            /* Wait for asynchronous operations to complete */
            MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);

            /* Reduce locally */
            for (int i = 0; i < num_reqs; i++) {
                if (i == (num_reqs - 1)) {
                    mpi_errno = MPIR_Reduce_local(((char *) tmp_buf) + i * count * extent,
                                                  recvbuf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                } else {
                    mpi_errno = MPIR_Reduce_local(((char *) tmp_buf) + i * count * extent,
                                                  ((char *) tmp_buf) + (i + 1) * count * extent,
                                                  count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                }
            }
        }
    }

    /*MAIN-STAGE: Ranks exchange data with groups size k over increasing
     * distances */
    if (virt_rank != -1) {
        /*Do exchanges */
        num_reqs = 0;
        int exchanges = 0;
        int distance = 1;
        int next_distance = k;
        while (distance < pofk) {
            /* Asynchronous sends */

            int starting_rank = rank / next_distance * next_distance;
            int rank_offset = starting_rank + rank % distance;
            for (int dst = rank_offset; dst < starting_rank + next_distance; dst += distance) {
                if (dst != rank) {
                    mpi_errno = MPIC_Isend(recvbuf, count, datatype, dst, MPIR_ALLREDUCE_TAG,
                                           comm_ptr, coll_group, &reqs[num_reqs++], errflag);
                    MPIR_ERR_CHECK(mpi_errno);
                    mpi_errno = MPIC_Irecv(((char *) tmp_buf) + exchanges * count * extent,
                                           count, datatype, dst, MPIR_ALLREDUCE_TAG,
                                           comm_ptr, coll_group, &reqs[num_reqs++]);
                    MPIR_ERR_CHECK(mpi_errno);
                    exchanges++;
                }
            }

            /* Wait for asynchronous operations to complete */
            MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);
            num_reqs = 0;
            exchanges = 0;

            /* Perform reduction on the received values */
            int recvbuf_last = 0;
            for (int dst = rank_offset; dst < starting_rank + next_distance - distance;
                 dst += distance) {
                void *dst_buf = ((char *) tmp_buf) + exchanges * count * extent;
                if (dst == rank - distance) {
                    mpi_errno = MPIR_Reduce_local(dst_buf, recvbuf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                    recvbuf_last = 1;
                    exchanges++;
                } else if (dst == rank) {
                    mpi_errno = MPIR_Reduce_local(recvbuf, dst_buf, count, datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                    recvbuf_last = 0;
                } else {
                    mpi_errno =
                        MPIR_Reduce_local(dst_buf, (char *) dst_buf + count * extent, count,
                                          datatype, op);
                    MPIR_ERR_CHECK(mpi_errno);
                    recvbuf_last = 0;
                    exchanges++;
                }
            }

            if (!recvbuf_last) {
                mpi_errno = MPIR_Localcopy((char *) tmp_buf + exchanges * count * extent,
                                           count, datatype, recvbuf, count, datatype);
                MPIR_ERR_CHECK(mpi_errno);
            }

            distance = next_distance;
            next_distance *= k;
            exchanges = 0;
        }
    }

    /* POST-STAGE: Send result to ranks outside main algorithm */
    if (pofk < comm_size) {
        num_reqs = 0;
        if (rank >= pofk) {
            int post_src = rank % pofk;
            /* This process is outside the core algorithm, so receive data */
            mpi_errno = MPIC_Recv(recvbuf, count,
                                  datatype, post_src,
                                  MPIR_ALLREDUCE_TAG, comm_ptr, coll_group, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
        } else {
            /* This is process is in the algorithm, so send data */
            for (int post_dst = (rank % pofk) + pofk; post_dst < comm_size; post_dst += pofk) {
                mpi_errno = MPIC_Isend(recvbuf, count, datatype, post_dst, MPIR_ALLREDUCE_TAG,
                                       comm_ptr, coll_group, &reqs[num_reqs++], errflag);
                MPIR_ERR_CHECK(mpi_errno);
            }

            MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);
        }
    }
  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
