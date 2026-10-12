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
 * For an allgather, we have a symmetric scenario to reduce-scatter:
 * the permutation is done at the end, after all the ranks have
 * received the entire buffer. The received data is permuted by
 * moving each data block from position i to position reverse(v(i)),
 * where reverse denotes the bit-reversal of v(i). This ensures
 * contiguous memory access during the preceding gather steps.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 */
static inline int MPIR_Allgather_intra_bine_permute(const void *sendbuf, MPI_Aint sendcount,
                                                    MPI_Datatype sendtype, void *recvbuf,
                                                    MPI_Aint recvcount, MPI_Datatype recvtype,
                                                    MPIR_Comm *comm_ptr, int coll_attr)
{

    int rank, comm_size, steps, mpi_errno = MPI_SUCCESS, remote, data_exchange;
    int *permutation = NULL;
    int tmpcount;
    MPI_Aint rext, rsize;
    char *tmprecv = NULL;
    void *tmpbuf = NULL;
    int is_contig = 0, nbytes;
    MPI_Aint true_lb, true_extent;
    MPI_Datatype tmptype;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    steps = MPL_log2(comm_size);
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(recvtype, rext);
    MPIR_Datatype_get_size_macro(recvtype, rsize);

    if (HANDLE_IS_BUILTIN(recvtype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(recvtype, &is_contig);
    }

    nbytes = rsize * recvcount;

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);
        tmpbuf = MPIR_get_contig_ptr(recvbuf, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmpbuf, nbytes * comm_size);
    }

    if (MPI_IN_PLACE != sendbuf) {
        mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                   tmpbuf, nbytes, MPIR_BYTE_INTERNAL);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (!is_contig || rank != 0) {
        /* if true_lb == 0, then tmpsend == tmprecv */
        mpi_errno = MPIR_Localcopy((char *) recvbuf + (MPI_Aint) rank * (MPI_Aint) recvcount * rext,
                                   recvcount, recvtype,
                                   tmpbuf, nbytes, MPIR_BYTE_INTERNAL);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_CHKLMEM_MALLOC(permutation, comm_size * sizeof(int));

    memset(permutation, -1, comm_size * sizeof(int));
    *(permutation + rank) = 0;

    data_exchange = 1;
    for (int step = steps - 1; step >= 0; step--) {
        remote = MPII_Bine_pi(rank, step, comm_size);

        MPII_Bine_get_permutation(rank, step, steps, comm_size, permutation, data_exchange);

        /* if recvtype isn't contiguous, we send/recv
         * the packed data using MPIR_BYTE_INTERNAL.
         * Otherwise, the original recvtype is used directly.
         */
        if (!is_contig) {
            tmprecv = (char *) tmpbuf + (MPI_Aint) data_exchange * (MPI_Aint) nbytes;
            tmpcount = data_exchange * nbytes;
            tmptype = MPIR_BYTE_INTERNAL;
        } else {
            tmprecv = (char *) tmpbuf + (MPI_Aint) data_exchange * (MPI_Aint) recvcount * rext;
            tmpcount = data_exchange * recvcount;
            tmptype = recvtype;
        }

        mpi_errno =
            MPIC_Sendrecv(tmpbuf, tmpcount, tmptype, remote,
                          MPIR_ALLGATHER_TAG, tmprecv, tmpcount,
                          tmptype, remote, MPIR_ALLGATHER_TAG, comm_ptr,
                          MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);
        data_exchange <<= 1;
    }

    if (!is_contig) {
        mpi_errno =
            MPII_Bine_reorder_blocks(tmpbuf, nbytes, MPIR_BYTE_INTERNAL, permutation,
                                    comm_size);
        MPIR_ERR_CHECK(mpi_errno);
        mpi_errno = MPIR_Localcopy(tmpbuf, nbytes * comm_size, MPIR_BYTE_INTERNAL,
                                   recvbuf, recvcount * comm_size, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        mpi_errno =
            MPII_Bine_reorder_blocks(recvbuf, recvcount, recvtype, permutation,
                                     comm_size);
        MPIR_ERR_CHECK(mpi_errno);
    }

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
 * and medium-sized vectors, this approach increases the data transmission overhead,
 * as multiple small messages are sent without the benefit of computation/communication
 * overlap (since Allgather has no reduction phase).
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 */
static inline int
MPIR_Allgather_intra_bine_block_by_block(const void *sendbuf,
                                         MPI_Aint sendcount,
                                         MPI_Datatype sendtype,
                                         void *recvbuf,
                                         MPI_Aint recvcount,
                                         MPI_Datatype recvtype,
                                         MPIR_Comm *comm_ptr, int coll_attr)
{
    int rank, comm_size, steps, mpi_errno = MPI_SUCCESS, remote;
    int *s_bitmap = NULL, *r_bitmap = NULL;
    MPI_Aint rext, rsize, tmpoffset;
    char *tmpsend = NULL, *tmprecv = NULL;
    MPIR_Request **requests = NULL;
    int is_contig = 0, nbytes;
    MPI_Aint true_lb, true_extent;
    void *tmpbuf = NULL;
    int tmpcount;
    MPI_Datatype tmptype;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    steps = MPL_log2(comm_size);
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(recvtype, rext);
    MPIR_Datatype_get_size_macro(recvtype, rsize);

    if (HANDLE_IS_BUILTIN(recvtype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(recvtype, &is_contig);
    }

    nbytes = rsize * recvcount;

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);
        tmpbuf = MPIR_get_contig_ptr(recvbuf, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmpbuf, nbytes * comm_size);
    }

    /* if recvtype isn't contiguous, we send/recv
     * the packed data using MPIR_BYTE_INTERNAL.
     * Otherwise, the original recvtype is used directly.
     */
    if (!is_contig) {
        tmpcount = nbytes;
        tmptype = MPIR_BYTE_INTERNAL;
        tmpoffset = nbytes;
    } else {
        tmpcount = recvcount;
        tmptype = recvtype;
        tmpoffset = recvcount * rext;
    }

    if (MPI_IN_PLACE != sendbuf) {
        tmpsend = (char *) sendbuf;
        tmprecv = (char *) tmpbuf + (MPI_Aint) rank * tmpoffset;
        mpi_errno = MPIR_Localcopy(tmpsend, sendcount, sendtype,
                                   tmprecv, tmpcount, tmptype);
        MPIR_ERR_CHECK(mpi_errno);
    /* if true_lb == 0, then tmpsend == tmprecv */
    } else if (!is_contig) {
        tmpsend = (char *) recvbuf + (MPI_Aint) rank * (MPI_Aint) recvcount * rext;
        tmprecv = (char *) tmpbuf + (MPI_Aint) rank * (MPI_Aint) nbytes;
        mpi_errno = MPIR_Localcopy(tmpsend, recvcount, recvtype,
                                   tmprecv, nbytes, MPIR_BYTE_INTERNAL);
        MPIR_ERR_CHECK(mpi_errno);
    }

    MPIR_CHKLMEM_MALLOC(s_bitmap, comm_size * sizeof(int));
    MPIR_CHKLMEM_MALLOC(r_bitmap, comm_size * sizeof(int));
    MPIR_CHKLMEM_MALLOC(requests, comm_size * sizeof(MPIR_Request *));

    for (int step = steps - 1; step >= 0; step--) {
        int num_reqs = 0;
        remote = MPII_Bine_pi(rank, step, comm_size);

        memset(s_bitmap, 0, comm_size * sizeof(int));
        memset(r_bitmap, 0, comm_size * sizeof(int));
        MPII_Bine_get_indexes(rank, step, steps, comm_size, r_bitmap);
        MPII_Bine_get_indexes(remote, step, steps, comm_size, s_bitmap);

        for (int block = 0; block < comm_size; block++) {
            if (s_bitmap[block] != 0) {
                tmpsend = (char *) tmpbuf + (MPI_Aint) block * tmpoffset;
                mpi_errno = MPIC_Isend(tmpsend, tmpcount, tmptype, remote,
                                       MPIR_ALLGATHER_TAG, comm_ptr,
                                       requests + num_reqs, coll_attr);
                MPIR_ERR_CHECK(mpi_errno);
                num_reqs++;
            }
            if (r_bitmap[block] != 0) {
                tmprecv = (char *) tmpbuf + (MPI_Aint) block * tmpoffset;
                mpi_errno = MPIC_Irecv(tmprecv, tmpcount, tmptype, remote,
                                       MPIR_ALLGATHER_TAG, comm_ptr, requests + num_reqs);
                MPIR_ERR_CHECK(mpi_errno);
                num_reqs++;
            }
        }
        mpi_errno = MPIC_Waitall(num_reqs, requests, MPI_STATUSES_IGNORE);
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (!is_contig) {
        mpi_errno = MPIR_Localcopy(tmpbuf, nbytes * comm_size, MPIR_BYTE_INTERNAL,
                                   recvbuf, recvcount * comm_size, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

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
 * For an allgather, we have a symmetric scenario to reduce-scatter:
 * the transmission to reorder the blocks is done before the actual
 * steps of the collective operation. Ranks first exchange their
 * blocks with the appropriate partner so that the subsequent
 * gather steps naturally place the data in the correct final order,
 * skipping the need for a final permutation.
 *
 * Cost = (lgp + 1).alpha + n.((p-1)/p).beta
 */
static inline int MPIR_Allgather_intra_bine_send_remap(const void *sendbuf, MPI_Aint sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       MPI_Aint recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm *comm_ptr, int coll_attr)
{
    int rank, comm_size, steps, mpi_errno = MPI_SUCCESS;
    int vrank, remote, vremote, send_block_location, distance, is_contig, nbytes, tmpcount;
    MPI_Aint rext, rsize, true_lb, true_extent;
    MPI_Aint step_scount;
    MPI_Aint tmpoffset;
    char *tmpsend = NULL, *tmprecv = NULL;
    void *tmpbuf = NULL;
    MPI_Datatype tmptype;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    steps = MPL_log2(comm_size);
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(recvtype, rext);
    MPIR_Datatype_get_size_macro(recvtype, rsize);

    if (HANDLE_IS_BUILTIN(recvtype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(recvtype, &is_contig);
    }

    nbytes = rsize * recvcount;

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);
        tmpbuf = MPIR_get_contig_ptr(recvbuf, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmpbuf, nbytes * comm_size);
    }

    /* if recvtype isn't contiguous, we send/recv
     * the packed data using MPIR_BYTE_INTERNAL.
     * Otherwise, the original recvtype is used directly.
     */
    if (!is_contig) {
        tmpcount = nbytes;
        tmptype = MPIR_BYTE_INTERNAL;
        tmpoffset = nbytes;
    } else {
        tmpcount = recvcount;
        tmptype = recvtype;
        tmpoffset = recvcount * rext;
    }

    /* Initialization step:
     * - if I gather the result for another rank, I send my buffer to that rank
     *   and I receive the data from the rank at the inverse permutation
     * - if I gather the result for myself, I copy the data from the send buffer
     */
    vrank = (int) MPII_Bine_remap_rank(comm_size, rank);
    tmpsend = (char *) recvbuf + (MPI_Aint) rank * (MPI_Aint) recvcount * rext;
    tmprecv = (char *) tmpbuf + (MPI_Aint) vrank * tmpoffset;
    if (vrank != rank) {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno =
                MPIC_Sendrecv(sendbuf, sendcount, sendtype,
                              MPII_Bine_get_sender_rec(comm_size, rank), MPIR_ALLGATHER_TAG,
                              tmprecv, tmpcount, tmptype, vrank,
                              MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        } else if (vrank != 0) {
            /* if vrank != rank, then tmprecv != tmpsend. So it is safe to use
             * MPIC_Sendrecv without the problem of an overlapping, even if the datatype
             * is not contiguous.
            */
            mpi_errno =
                MPIC_Sendrecv(tmpsend, recvcount, recvtype,
                              MPII_Bine_get_sender_rec(comm_size, rank), MPIR_ALLGATHER_TAG,
                              tmprecv, tmpcount, tmptype, vrank,
                              MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        }
        MPIR_ERR_CHECK(mpi_errno);
    } else {
        if (sendbuf != MPI_IN_PLACE) {
            mpi_errno = MPIR_Localcopy(sendbuf, sendcount, sendtype,
                                       recvbuf, recvcount, recvtype);
        } else if (!is_contig) {
            /* Instead, if vrank == rank, then tmpsend and tmprecv point at the same
             * address, resulting to an overlapping in memory. So we make sure
             * it's not contiguous, because in that case recvbuf and tmpbuf point at the same
             * address in memory.
             */
            mpi_errno = MPIR_Localcopy(tmpsend, recvcount, recvtype,
                                       tmprecv, tmpcount, tmptype);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Communication step:
     * At every step i, rank r:
     * - exchanges message with rank remote = (r ^ 2^i).
     */
    distance = 0x1;
    send_block_location = vrank;
    for (int step = steps - 1; step >= 0; step--) {
        step_scount = tmpcount * distance;
        remote = MPII_Bine_pi(rank, step, comm_size);
        vremote = (int) MPII_Bine_remap_rank(comm_size, remote);

        if (vrank < vremote) {
            tmpsend = (char *) tmpbuf + (MPI_Aint) send_block_location *
                tmpoffset;
            tmprecv = (char *) tmpbuf + (MPI_Aint) (send_block_location + distance) *
                tmpoffset;
        } else {
            tmpsend = (char *) tmpbuf +
                (MPI_Aint) send_block_location * tmpoffset;
            tmprecv = (char *) tmpbuf + (MPI_Aint) (send_block_location - distance) *
                tmpoffset;
            send_block_location -= distance;
        }

        mpi_errno =
            MPIC_Sendrecv(tmpsend, step_scount, tmptype, remote,
                          MPIR_ALLGATHER_TAG, tmprecv, step_scount, tmptype,
                          remote, MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        distance <<= 1;
    }

    if (!is_contig) {
        mpi_errno = MPIR_Localcopy(tmpbuf, nbytes * comm_size, MPIR_BYTE_INTERNAL,
                                   recvbuf, recvcount * comm_size, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
/*
 * Algorithm: BINE two blocks
 *
 * Restrictions: power-of-two no. of processes
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * This approach performs the gather using a distance-halving
 * rather than a distance-doubling butterfly. It simplifies handling
 * non-contiguous data by considering the buffer as circular: any
 * overflow at the end wraps around to the beginning. However,
 * this may require sending two segments (one at the end of the buffer
 * and one at the beginning) and can increase traffic over globally
 * oversubscribed links.
 *
 * Cost = lgp.alpha + n.((p-1)/p).beta
 */
static inline int MPIR_Allgather_intra_bine_two_blocks(const void *sendbuf, MPI_Aint sendcount,
                                                       MPI_Datatype sendtype, void *recvbuf,
                                                       MPI_Aint recvcount, MPI_Datatype recvtype,
                                                       MPIR_Comm *comm_ptr, int coll_attr)
{

    int rank, comm_size, steps, mpi_errno = MPI_SUCCESS, remote;
    int mask, my_first, recv_index, send_index, is_contig, nbytes, tmpcount;
    int send_count, recv_count, extra_send, extra_recv;
    MPI_Aint rext, rsize, true_lb, true_extent;
    MPI_Aint tmpoffset;
    MPI_Datatype tmptype;
    MPIR_Request *req;
    char *tmpsend = NULL, *tmprecv = NULL, *tmpbuf = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    /*
     * Current implementation only handles power-of-two number of processes.
     */
    steps = MPL_log2(comm_size);
    MPIR_ERR_CHKANDJUMP(!MPL_is_pof2(comm_size), mpi_errno, MPI_ERR_COMM, "**comm");

    MPIR_Datatype_get_extent_macro(recvtype, rext);
    MPIR_Datatype_get_size_macro(recvtype, rsize);

    if (HANDLE_IS_BUILTIN(recvtype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(recvtype, &is_contig);
    }

    nbytes = rsize * recvcount;

    if (is_contig) {
        /* contiguous. no need to pack. */
        MPIR_Type_get_true_extent_impl(recvtype, &true_lb, &true_extent);
        tmpbuf = MPIR_get_contig_ptr(recvbuf, true_lb);
    } else {
        MPIR_CHKLMEM_MALLOC(tmpbuf, nbytes * comm_size);
    }

    /* if recvtype isn't contiguous, we send/recv
     * the packed data using MPIR_BYTE_INTERNAL.
     * Otherwise, the original recvtype is used directly.
     */
    if (!is_contig) {
        tmpcount = nbytes;
        tmptype = MPIR_BYTE_INTERNAL;
        tmpoffset = nbytes;
    } else {
        tmpcount = recvcount;
        tmptype = recvtype;
        tmpoffset = recvcount * rext;
    }

    /* Initialization step:
     * - if send buffer is not MPI_IN_PLACE, copy send buffer to block  of
     * receive buffer
     */
    if (MPI_IN_PLACE != sendbuf) {
        tmpsend = (char *) sendbuf;
        tmprecv = (char *) tmpbuf + (MPI_Aint) rank * tmpoffset;
        mpi_errno = MPIR_Localcopy(tmpsend, sendcount, sendtype,
                                   tmprecv, tmpcount, tmptype);
        MPIR_ERR_CHECK(mpi_errno);
    } else if (rank != 0) {
        tmpsend = (char *) recvbuf + (MPI_Aint) rank *(MPI_Aint) recvcount * rext;
        tmprecv = (char *) tmpbuf;
        mpi_errno = MPIR_Localcopy(tmpsend, recvcount, recvtype,
                                   tmprecv, tmpcount, tmptype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Communication step.
     *  At every step i, rank r:
     *  - communication peer is calculated by pi(rank, step, comm_size)
     *  - if the step is even, even ranks send the next `mask` blocks and
     *  odd ranks send the previous `mask` blocks.
     *  - if the step is odd, even ranks send the previous `mask` blocks and
     *  odd ranks send the next `mask` blocks.
     */
    mask = 0x1;
    my_first = rank;
    for (int step = 0; step < steps; step++) {
        req = NULL;
        remote = MPII_Bine_pi(rank, step, comm_size);
        send_index = my_first;

        /* Calculate the send and receive indexes by alternating send/recv
         * direction.
         */
        if ((step & 1) == (rank & 1)) {
            recv_index = MPII_Bine_mod((send_index + mask + comm_size), comm_size);
        } else {
            recv_index = MPII_Bine_mod((send_index - mask + comm_size), comm_size);
            my_first = recv_index;
        }

        /* Control if the previously calculated indexes imply out of bound
         * send/recv. If so, split the communication with an extra send/recv.
         */
        extra_recv = (recv_index + mask > comm_size) ? ((recv_index + mask) - comm_size) : 0;
        recv_count = mask - extra_recv;

        extra_send = (send_index + mask > comm_size) ? ((send_index + mask) - comm_size) : 0;
        send_count = mask - extra_send;

        /* warparound communication */
        if (extra_recv != 0) {
            tmprecv = (char *) tmpbuf;
            mpi_errno =
                MPIC_Irecv(tmprecv, (MPI_Aint) extra_recv * tmpcount, tmptype,
                           remote, MPIR_ALLGATHER_TAG, comm_ptr, &req);
            MPIR_ERR_CHECK(mpi_errno);
        }
        if (extra_send != 0) {
            tmpsend = (char *) tmpbuf;
            mpi_errno =
                MPIC_Send(tmpsend, (MPI_Aint) extra_send * tmpcount, tmptype,
                          remote, MPIR_ALLGATHER_TAG, comm_ptr, coll_attr);
            MPIR_ERR_CHECK(mpi_errno);
        }
        /* Simple case: no wrap-around */
        tmpsend = (char *) tmpbuf + (MPI_Aint) send_index * tmpoffset;
        tmprecv = (char *) tmpbuf + (MPI_Aint) recv_index * tmpoffset;

        mpi_errno =
            MPIC_Sendrecv(tmpsend, (MPI_Aint) send_count * tmpcount, tmptype,
                          remote, MPIR_ALLGATHER_TAG, tmprecv,
                          (MPI_Aint) recv_count * tmpcount, tmptype, remote,
                          MPIR_ALLGATHER_TAG, comm_ptr, MPI_STATUS_IGNORE, coll_attr);
        MPIR_ERR_CHECK(mpi_errno);

        if (extra_recv != 0) {
            mpi_errno = MPIC_Wait(req);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Request_free(req);
        }
        mask <<= 1;
    }

    if (!is_contig) {
        mpi_errno = MPIR_Localcopy(tmpbuf, nbytes * comm_size, MPIR_BYTE_INTERNAL,
                                   recvbuf, recvcount * comm_size, recvtype);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Allgather_intra_bine(const void *sendbuf, MPI_Aint sendcount,
                              MPI_Datatype sendtype, void *recvbuf,
                              MPI_Aint recvcount, MPI_Datatype recvtype,
                              MPIR_Comm *comm_ptr, int bine_type, int coll_attr)
{

    int mpi_errno = MPI_SUCCESS;

    if (((sendcount == 0) && (sendbuf != MPI_IN_PLACE)) || (recvcount == 0))
        goto fn_exit;

    /* Here we use the CVAR MPIR_CVAR_ALLGATHER_BINE_TYPE to select the
     * correct algorithm. If an invalid value is given, then the algorithm
     * MPIR_Allgather_intra_bine_permute is used by default.
     */
    switch (bine_type) {
        case MPIR_BINE_TYPE_PERMUTE:
            mpi_errno = MPIR_Allgather_intra_bine_permute(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, coll_attr);
            break;
        case MPIR_BINE_TYPE_SEND_REMAP:
            mpi_errno = MPIR_Allgather_intra_bine_send_remap(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype,
                                                             comm_ptr, coll_attr);
            break;
        case MPIR_BINE_TYPE_BLOCK_BY_BLOCK:
            mpi_errno = MPIR_Allgather_intra_bine_block_by_block(sendbuf, sendcount, sendtype,
                                                                 recvbuf, recvcount, recvtype,
                                                                 comm_ptr, coll_attr);
            break;
        case MPIR_BINE_TYPE_TWO_BLOCKS:
            mpi_errno = MPIR_Allgather_intra_bine_two_blocks(sendbuf, sendcount, sendtype,
                                                             recvbuf, recvcount, recvtype,
                                                             comm_ptr, coll_attr);
            break;
        default:
            mpi_errno = MPIR_Allgather_intra_bine_permute(sendbuf, sendcount, sendtype,
                                                          recvbuf, recvcount, recvtype,
                                                          comm_ptr, coll_attr);
            break;

    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
