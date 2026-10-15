/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/*
 * Algorithm: BINE Bandwidth
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * For large vectors,
 * we perform a BINE reduce-scatter followed by a BINE allgather.
 *
 * Cost = 2.lgp.alpha + 2.n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
int MPIR_Allreduce_intra_bine_bdw(const void *sendbuf, void *recvbuf,
                                  MPI_Aint count, MPI_Datatype datatype,
                                  MPI_Op op, MPIR_Comm *comm_ptr,
                                  int coll_attr) {
    int comm_size, rank, dest, steps, step, mpi_errno = MPI_SUCCESS;
    int adjsize, extra_ranks, is_power_of_two;
    int new_rank, loop_flag = 0;
    int *r_count = NULL, *s_count = NULL, *r_index = NULL, *s_index = NULL;
    int vdest;
    MPI_Aint w_size;
    int vrank;
    char *tmp_send = NULL, *tmp_recv = NULL;
    char *tmp_buf = NULL;
    MPI_Aint true_lb = 0, extent, true_extent;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /* Special case for comm_size == 1 */
    if (comm_size == 1) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count,
                                       datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
        goto fn_exit;
    }

    MPIR_Assert(MPIR_Op_is_commutative(op));

    /* Determine nearest power of two less than or equal to comm_size
     * and return an error if comm_size is 0
     */
    steps = MPII_Bine_hibit(comm_size, (int)(sizeof(comm_size) * CHAR_BIT) - 1);
    MPIR_ERR_CHKANDJUMP(steps == -1, mpi_errno, MPI_ERR_ARG, "**arg");
    adjsize = 1 << steps; /* Largest power of two <= comm_size */

    /* Number of nodes that exceed the largest power of two less than or equal
     * to comm_size
     */
    extra_ranks = comm_size - adjsize;
    is_power_of_two = (comm_size & (comm_size - 1)) == 0;

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    /* Allocate temporary buffer for send/recv and reduce operations */
    MPIR_CHKLMEM_MALLOC(tmp_buf, count * (MPL_MAX(extent, true_extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_buf = (void *) ((char *) tmp_buf - true_lb);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* First part of computation to get a 2^n number of nodes.
     * What happens is that first #extra_rank even nodes sends their
     * data to the successive node and do not partecipate in the general
     * collective call operation.
     * All the nodes that do not stop their computation will receive an alias
     * called new_node, used to calculate their correct destination wrt this
     * new "cut" topology.
     */
    new_rank = rank;
    loop_flag = 0;
    if (rank < (2 * extra_ranks)) {
        if ((rank % 2) == 0) { /* even */
            mpi_errno = MPIC_Send(recvbuf, count, datatype, (rank + 1),
                                  MPIR_ALLREDUCE_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            loop_flag = 1;
        } else { /* odd */
            mpi_errno =
                MPIC_Recv(tmp_buf, count, datatype, (rank - 1),
                          MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno = MPIR_Reduce_local(tmp_buf, recvbuf, count, datatype, op);
            MPIR_ERR_CHECK(mpi_errno);
            new_rank = rank >> 1;
        }
    } else {
        new_rank = rank - extra_ranks;
    }

    /* Here the actual allreduce starts */
    MPIR_CHKLMEM_MALLOC(r_index, sizeof(*r_index) * steps);
    MPIR_CHKLMEM_MALLOC(s_index, sizeof(*s_index) * steps);
    MPIR_CHKLMEM_MALLOC(r_count, sizeof(*r_count) * steps);
    MPIR_CHKLMEM_MALLOC(s_count, sizeof(*s_count) * steps);

    /* Only the remaining ranks will do the following part */
    if (!loop_flag) {
        /* Reduce-Scatter phase */
        w_size = count;
        s_index[0] = r_index[0] = 0;
        vrank = MPII_Bine_remap_rank(adjsize, new_rank);

        for (step = 0; step < steps; step++) {
            vdest = MPII_Bine_pi(new_rank, step, adjsize);

            dest = is_power_of_two         ? vdest
                   : (vdest < extra_ranks) ? (vdest << 1) + 1
                                           : vdest + extra_ranks;
            /* TODO: dest or vdest as param? */
            vdest = MPII_Bine_remap_rank(adjsize, vdest);

            if (vrank < vdest) {
                r_count[step] = w_size >> 1;
                s_count[step] = w_size - r_count[step];
                s_index[step] = r_index[step] + r_count[step];
            } else {
                s_count[step] = w_size >> 1;
                r_count[step] = w_size - s_count[step];
                r_index[step] = s_index[step] + s_count[step];
            }

            tmp_send = (char *)recvbuf + s_index[step] * extent;

            mpi_errno = MPIC_Sendrecv(
                tmp_send, s_count[step], datatype, dest, MPIR_ALLREDUCE_TAG,
                tmp_buf, r_count[step], datatype, dest, MPIR_ALLREDUCE_TAG,
                comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);

            tmp_recv = (char *)recvbuf + r_index[step] * extent;

            mpi_errno = MPIR_Reduce_local(tmp_buf, tmp_recv, r_count[step],
                                          datatype, op);
            MPIR_ERR_CHECK(mpi_errno);

            if (step + 1 < steps) {
                r_index[step + 1] = r_index[step];
                s_index[step + 1] = r_index[step];
                w_size = r_count[step];
            }
        }

        /* Allgather phase */
        for (step = steps - 1; step >= 0; step--) {
            vdest = MPII_Bine_pi(new_rank, step, adjsize);

            dest = is_power_of_two         ? vdest
                   : (vdest < extra_ranks) ? (vdest << 1) + 1
                                           : vdest + extra_ranks;

            tmp_send = (char *)recvbuf + r_index[step] * extent;
            tmp_recv = (char *)recvbuf + s_index[step] * extent;

            mpi_errno = MPIC_Sendrecv(
                tmp_send, r_count[step], datatype, dest, MPIR_ALLREDUCE_TAG,
                tmp_recv, s_count[step], datatype, dest, MPIR_ALLREDUCE_TAG,
                comm_ptr, MPI_STATUS_IGNORE, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* Final results is sent to nodes that are not included in general
     * computation (general computation loop requires 2^n nodes).
     */
    if (rank < (2 * extra_ranks)) {
        if (rank % 2) { /* odd */
            mpi_errno = MPIC_Send(recvbuf, count, datatype, (rank - 1),
                                  MPIR_ALLREDUCE_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        } else { /* even */
            mpi_errno =
                MPIC_Recv(recvbuf, count, datatype, (rank + 1),
                          MPIR_ALLREDUCE_TAG, comm_ptr, MPI_STATUS_IGNORE);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}