/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/*
 * Algorithm: BINE permute
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * The permutation is
 * obtained by moving each data block from position 𝑖 to position
 * reverse(v(i)), where reverse denotes the bit-reversal of v(i).
 * By reversing v(i), the transmitted blocks share a fixed number
 * of most significant bits, making them contiguous in memory.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
static inline int MPIR_Reduce_scatter_intra_bine_permute(const void *sendbuf, void *recvbuf,
                                                         const MPI_Aint recvcounts[],
                                                         MPI_Datatype datatype, MPI_Op op,
                                                         MPIR_Comm *comm_ptr, int coll_attr)
{
    int comm_size, rank, mpi_errno = MPI_SUCCESS;
    int mask, inverse_mask, block_first_mask, remapped_rank, partner;
    int send_block_first, send_block_last, recv_block_first, recv_block_last;
    MPI_Aint send_count, recv_count;
    MPI_Aint total_count, extent, true_extent, true_lb;
    MPI_Aint *displs = NULL;
    void *tmp_recvbuf = NULL, *tmp_results = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_CHKLMEM_MALLOC(displs, comm_size * sizeof(MPI_Aint));

    total_count = 0;
    for (int i = 0; i < comm_size; i++) {
        displs[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* allocate temp. buffer to receive incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results because recvbuf may not be big enough */
    MPIR_CHKLMEM_MALLOC(tmp_results, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *) ((char *) tmp_results - true_lb);

    /* Permute memcpy sendbuf into tmp_results */
    for (int i = 0; i < comm_size; i++) {
        remapped_rank = MPII_Bine_remap_rank(comm_size, i);
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy((char *) sendbuf + displs[i] * extent, recvcounts[i],
                                       datatype,
                                       (char *) tmp_results + displs[remapped_rank] * extent,
                                       recvcounts[i], datatype);
        } else {
            mpi_errno = MPIR_Localcopy((char *) recvbuf + displs[i] * extent, recvcounts[i],
                                       datatype,
                                       (char *) tmp_results + displs[remapped_rank] * extent,
                                       recvcounts[i], datatype);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

    mask = 0x1;
    inverse_mask = 0x1 << (MPL_log2(comm_size) - 1);
    block_first_mask = ~(inverse_mask - 1);
    remapped_rank = MPII_Bine_remap_rank(comm_size, rank);
    while (mask < comm_size) {
        if (rank % 2 == 0) {
            partner = MPII_Bine_mod(rank + MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        } else {
            partner = MPII_Bine_mod(rank - MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        }

        /* For sure I need to send my (remapped) partner's data
         * the actual start block however must be aligned to
         * the power of two
         */
        send_block_first = MPII_Bine_remap_rank(comm_size, partner) & block_first_mask;
        send_block_last = send_block_first + inverse_mask - 1;
        send_count = displs[send_block_last] - displs[send_block_first] +
            recvcounts[send_block_last];

        /* Something similar for the block to recv.
         * I receive my block, but aligned to the power of two
         */
        recv_block_first = remapped_rank & block_first_mask;
        recv_block_last = recv_block_first + inverse_mask - 1;
        recv_count = displs[recv_block_last] - displs[recv_block_first] +
            recvcounts[recv_block_last];

        /* Send data from tmp_results. Recv into tmp_recvbuf */
        mpi_errno = MPIC_Sendrecv((char *) tmp_results + displs[send_block_first] * extent,
                                  send_count, datatype, partner, MPIR_REDUCE_SCATTER_TAG,
                                  (char *) tmp_recvbuf + displs[recv_block_first] * extent,
                                  recv_count, datatype, partner, MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                  MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Reduce_local((char *) tmp_recvbuf + displs[recv_block_first] * extent,
                                      (char *) tmp_results + displs[recv_block_first] * extent,
                                      recv_count, datatype, op);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
        inverse_mask >>= 1;
        block_first_mask >>= 1;
    }

    /* Final localcopy */
    mpi_errno = MPIR_Localcopy((char *) tmp_results + displs[remapped_rank] * extent,
                               recvcounts[rank], datatype, recvbuf, recvcounts[rank], datatype);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * Algorithm: BINE send remap
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * This variant is based on skipping the permutation
 * and letting ranks transmit contiguous data as if the
 * permutation had already been applied. As a result, at the end of the collective,
 * each rank would hold the block belonging to another rank. For
 * instance, after the reduce-scatter, rank 1 would hold block 4 (since
 * block 1 was not moved in position 4). A final communication step
 * can then be used to exchange and reorder the data correctly, with
 * each rank r sending its block to rank q = reverse(nu(r)).
 *
 * Cost = (lgp+1).alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
static inline int MPIR_Reduce_scatter_intra_bine_send_remap(const void *sendbuf, void *recvbuf,
                                                            const MPI_Aint recvcounts[],
                                                            MPI_Datatype datatype, MPI_Op op,
                                                            MPIR_Comm *comm_ptr, int coll_attr)
{

    int comm_size, rank, mpi_errno = MPI_SUCCESS;
    int mask, inverse_mask, block_first_mask, remapped_rank;
    int send_block_first, send_block_last, recv_block_first, recv_block_last;
    int partner;
    MPI_Aint send_count, recv_count;
    MPI_Aint total_count, extent, true_lb, true_extent;
    MPI_Aint *displs = NULL;

    void *tmp_recvbuf = NULL, *tmp_results = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_CHKLMEM_MALLOC(displs, comm_size * sizeof(MPI_Aint));

    total_count = 0;
    for (int i = 0; i < comm_size; i++) {
        displs[i] = total_count;
        total_count += recvcounts[i];
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* allocate temp. buffer to receive incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results because recvbuf may not be big enough */
    MPIR_CHKLMEM_MALLOC(tmp_results, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *) ((char *) tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype, tmp_results,
                                   total_count, datatype);
    else
        mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype, tmp_results,
                                   total_count, datatype);
    MPIR_ERR_CHECK(mpi_errno);

    mask = 0x1;
    inverse_mask = 0x1 << (int) (MPII_Bine_log2(comm_size) - 1);
    block_first_mask = ~(inverse_mask - 1);
    remapped_rank = MPII_Bine_remap_rank(comm_size, rank);
    while (mask < comm_size) {
        if (rank % 2 == 0) {
            partner = MPII_Bine_mod(rank + MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        } else {
            partner = MPII_Bine_mod(rank - MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        }

        /* For sure I need to send my (remapped) partner's data
         * the actual start block however must be aligned to
         * the power of two
         */
        send_block_first = MPII_Bine_remap_rank(comm_size, partner) & block_first_mask;
        send_block_last = send_block_first + inverse_mask - 1;
        send_count = displs[send_block_last] - displs[send_block_first] +
            recvcounts[send_block_last];

        /* Something similar for the block to recv.
         * I receive my block, but aligned to the power of two
         */
        recv_block_first = remapped_rank & block_first_mask;
        recv_block_last = recv_block_first + inverse_mask - 1;
        recv_count = displs[recv_block_last] - displs[recv_block_first] +
            recvcounts[recv_block_last];

        /* Send data from tmp_results. Recv into tmp_recvbuf */
        mpi_errno = MPIC_Sendrecv((char *) tmp_results + displs[send_block_first] * extent,
                                  send_count, datatype, partner, MPIR_REDUCE_SCATTER_TAG,
                                  (char *) tmp_recvbuf + displs[recv_block_first] * extent,
                                  recv_count, datatype, partner, MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                  MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        mpi_errno = MPIR_Reduce_local((char *) tmp_recvbuf + displs[recv_block_first] * extent,
                                      (char *) tmp_results + displs[recv_block_first] * extent,
                                      recv_count, datatype, op);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
        inverse_mask >>= 1;
        block_first_mask >>= 1;
    }

    /* Final send
     * Whom I have been remapped to? I.e., who is going to send me my data? Just
     * do a recv from any
     */
    mpi_errno = MPIC_Sendrecv((char *) tmp_results + displs[remapped_rank] * extent,
                              recvcounts[remapped_rank], datatype, remapped_rank,
                              MPIR_REDUCE_SCATTER_TAG, (char *) recvbuf, recvcounts[rank], datatype,
                              MPI_ANY_SOURCE, MPIR_REDUCE_SCATTER_TAG, comm_ptr, MPI_STATUS_IGNORE,
                              coll_attr);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/*
 * Algorithm: BINE block by block
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * Each block is transmitted independently. For small
 * and medium-sized vectors, this approach increases the data transmission overhead.
 * At the same time, it creates more opportunities
 * for computation and communication overlap during reduction
 * phases.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta + n.((p-1)/p).gamma
 */
static inline int MPIR_Reduce_scatter_intra_bine_block_by_block(const void *sendbuf, void *recvbuf,
                                                                const MPI_Aint recvcounts[],
                                                                MPI_Datatype datatype, MPI_Op op,
                                                                MPIR_Comm *comm_ptr, int coll_attr)
{
    int comm_size, rank, mpi_errno = MPI_SUCCESS;
    int mask, inverse_mask, block_first_mask, remapped_rank, partner;
    int send_block_first, send_block_last, recv_block_first, recv_block_last;
    int next_req, w_req;
    MPI_Aint total_count, extent, true_lb, true_extent;
    void *tmp_recvbuf = NULL, *tmp_results = NULL;
    MPI_Aint *displs = NULL, *inverse_remapping = NULL;
    MPIR_Request **reqs = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_CHKLMEM_MALLOC(displs, comm_size * sizeof(MPI_Aint));
    MPIR_CHKLMEM_MALLOC(inverse_remapping, comm_size * sizeof(MPI_Aint));

    total_count = 0;
    for (int i = 0; i < comm_size; i++) {
        displs[i] = total_count;
        total_count += recvcounts[i];
        inverse_remapping[MPII_Bine_remap_rank(comm_size, i)] = i;
    }

    if (total_count == 0) {
        goto fn_exit;
    }

    /* allocate temp. buffer to receive incoming data */
    MPIR_CHKLMEM_MALLOC(tmp_recvbuf, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_recvbuf = (void *) ((char *) tmp_recvbuf - true_lb);

    /* need to allocate another temporary buffer to accumulate
     * results because recvbuf may not be big enough */
    MPIR_CHKLMEM_MALLOC(tmp_results, total_count * (MPL_MAX(true_extent, extent)));
    /* adjust for potential negative lower bound in datatype */
    tmp_results = (void *) ((char *) tmp_results - true_lb);

    /* copy sendbuf into tmp_results */
    if (sendbuf != MPI_IN_PLACE)
        mpi_errno = MPIR_Localcopy(sendbuf, total_count, datatype, tmp_results,
                                   total_count, datatype);
    else
        mpi_errno = MPIR_Localcopy(recvbuf, total_count, datatype, tmp_results,
                                   total_count, datatype);

    MPIR_ERR_CHECK(mpi_errno);

    mask = 0x1;
    inverse_mask = 0x1 << (int) (MPII_Bine_log2(comm_size) - 1);
    block_first_mask = ~(inverse_mask - 1);
    remapped_rank = MPII_Bine_remap_rank(comm_size, rank);

    MPIR_CHKLMEM_MALLOC(reqs, comm_size * sizeof(MPIR_Request *));
    while (mask < comm_size) {
        if (rank % 2 == 0) {
            partner = MPII_Bine_mod(rank + MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        } else {
            partner = MPII_Bine_mod(rank - MPII_Bine_negabinary_to_binary((mask << 1) - 1),
                                    comm_size);
        }

        /* For sure I need to send my (remapped) partner's data
         * the actual start block however must be aligned to
         * the power of two
         */
        send_block_first = MPII_Bine_remap_rank(comm_size, partner) & block_first_mask;
        send_block_last = send_block_first + inverse_mask - 1;
        /* Something similar for the block to recv.
         * I receive my block, but aligned to the power of two
         */
        recv_block_first = remapped_rank & block_first_mask;
        recv_block_last = recv_block_first + inverse_mask - 1;

        next_req = 0;
        for (MPI_Aint block = recv_block_first; block <= recv_block_last; block++) {
            if (mask << 1 >= comm_size) {
                /* Last step, receiving in recvbuf */
                mpi_errno = MPIC_Irecv((char *) recvbuf, recvcounts[inverse_remapping[block]],
                                       datatype, partner, MPIR_REDUCE_SCATTER_TAG, comm_ptr,
                                       &reqs[next_req]);
            } else {
                mpi_errno = MPIC_Irecv((char *) tmp_recvbuf +
                                       displs[inverse_remapping[block]] * extent,
                                       recvcounts[inverse_remapping[block]], datatype, partner,
                                       MPIR_REDUCE_SCATTER_TAG, comm_ptr, &reqs[next_req]);
            }
            MPIR_ERR_CHECK(mpi_errno);
            ++next_req;
        }

        for (MPI_Aint block = send_block_first; block <= send_block_last; block++) {
            mpi_errno = MPIC_Isend((char *) tmp_results + displs[inverse_remapping[block]] * extent,
                                   recvcounts[inverse_remapping[block]], datatype, partner,
                                   MPIR_REDUCE_SCATTER_TAG, comm_ptr, &reqs[next_req], coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
            ++next_req;
        }

        w_req = 0;
        for (MPI_Aint block = recv_block_first; block <= recv_block_last; block++) {
            mpi_errno = MPIC_Wait(reqs[w_req]);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Request_free(reqs[w_req]);
            if (mask << 1 >= comm_size) {
                /* Last step, received in recvbuf, aggregating from tmp_results
                 */
                mpi_errno = MPIR_Reduce_local((char *) tmp_results +
                                              displs[inverse_remapping[block]] * extent,
                                              (char *) recvbuf,
                                              recvcounts[inverse_remapping[block]], datatype, op);
            } else {
                mpi_errno = MPIR_Reduce_local((char *) tmp_recvbuf +
                                              displs[inverse_remapping[block]] * extent,
                                              (char *) tmp_results +
                                              displs[inverse_remapping[block]] * extent,
                                              recvcounts[inverse_remapping[block]], datatype, op);
            }
            MPIR_ERR_CHECK(mpi_errno);
            ++w_req;
        }

        mpi_errno = MPIC_Waitall(next_req - w_req, &reqs[w_req], MPI_STATUSES_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);

        mask <<= 1;
        inverse_mask >>= 1;
        block_first_mask >>= 1;
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Reduce_scatter_intra_bine(const void *sendbuf, void *recvbuf,
                                   const MPI_Aint recvcounts[],
                                   MPI_Datatype datatype, MPI_Op op,
                                   MPIR_Comm *comm_ptr, int bine_type, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    int is_commutative;

    is_commutative = MPIR_Op_is_commutative(op);
    MPIR_Assert(is_commutative);

    /* Here we use the CVAR MPIR_CVAR_REDUCE_SCATTER_BINE_TYPE to select the
     * correct algorithm. If an invalid value is given, then the
     * default algorithm used is MPIR_Reduce_scatter_intra_bine_permute.
     */
    switch (bine_type) {
        case MPIR_BINE_TYPE_PERMUTE:
            mpi_errno =
                MPIR_Reduce_scatter_intra_bine_permute(sendbuf, recvbuf, recvcounts, datatype, op,
                                                       comm_ptr, coll_attr);
            break;
        case MPIR_BINE_TYPE_SEND_REMAP:
            mpi_errno =
                MPIR_Reduce_scatter_intra_bine_send_remap(sendbuf, recvbuf, recvcounts, datatype,
                                                          op, comm_ptr, coll_attr);
            break;
        case MPIR_BINE_TYPE_BLOCK_BY_BLOCK:
            mpi_errno =
                MPIR_Reduce_scatter_intra_bine_block_by_block(sendbuf, recvbuf, recvcounts,
                                                              datatype, op, comm_ptr, coll_attr);
            break;
        default:
            mpi_errno =
                MPIR_Reduce_scatter_intra_bine_permute(sendbuf, recvbuf, recvcounts, datatype, op,
                                                       comm_ptr, coll_attr);
            break;
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
