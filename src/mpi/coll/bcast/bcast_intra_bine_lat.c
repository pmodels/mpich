/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpir_bine.h"

/* Algorithm: Bine tree Latency
 *
 * Based on "BINE Trees: Enhancing Collective Operations
 * by Optimizing Communication Locality" Nov 2025
 * paper: https://doi.org/10.1145/3712285.3759835
 *
 * For short messages, we use a Bine tree algorithm implemented with distance
 * halving Bine trees for broadcasting data from the root to the leaves.
 *
 * Cost = lgp.alpha + n.lgp.beta
 */
int MPIR_Bcast_intra_bine_lat(void *buf, MPI_Aint count,
                              MPI_Datatype datatype, int root,
                              MPIR_Comm *comm_ptr, int coll_attr)
{
    int comm_size, rank, mpi_errno = MPI_SUCCESS, btnb_vrank;
    int vrank, mask, recvd, req_count = 0, steps, pow2;
    int partner, mask_lsbs, lsbs, equal_lsbs;
    MPIR_Request **requests;
    int is_contig, is_in_pow2;
    MPI_Aint nbytes, type_size;
    void *tmp_buf = NULL;

    MPIR_CHKLMEM_DECL();

    MPIR_COMM_RANK_SIZE(comm_ptr, rank, comm_size);

    if (comm_size == 1) {
        goto fn_exit;
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);
    if (HANDLE_IS_BUILTIN(datatype)) {
        is_contig = 1;
    } else {
        MPIR_Datatype_is_contig(datatype, &is_contig);
    }

    nbytes = type_size * count;
    if (nbytes == 0) {
        goto fn_exit;
    }

    if (!is_contig) {
        MPIR_CHKLMEM_MALLOC(tmp_buf, nbytes);
        if (rank == root) {
            mpi_errno = MPIR_Localcopy(buf, count, datatype, tmp_buf, nbytes, MPIR_BYTE_INTERNAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    pow2 = MPL_pof2(comm_size);

    /* MPII_Bine_mod computes math modulo rather than reminder */
    vrank = (rank >= root) ? rank - root : rank - root + comm_size;
    steps = MPII_Bine_log2(pow2);
    mask = 0x1 << (int) (steps - 1);
    recvd = (root == rank);
    btnb_vrank = MPII_Bine_binary_to_negabinary(vrank);

    MPIR_CHKLMEM_MALLOC(requests, steps * sizeof(MPIR_Request *));

    is_in_pow2 = (vrank < pow2);

    while ( is_in_pow2 && (mask > 0) ) {
        partner = btnb_vrank ^ ((mask << 1) - 1);
        partner = MPII_Bine_mod(MPII_Bine_negabinary_to_binary(partner), pow2);
        partner = MPII_Bine_mod(partner + root, comm_size);

        mask_lsbs = (mask << 1) - 1;    /* Mask with num_steps - step + 1 LSBs set to 1 */
        lsbs = btnb_vrank & mask_lsbs;  /* Extract k LSBs */
        equal_lsbs = (lsbs == 0 || lsbs == mask_lsbs);

        if (recvd) {
            if (!is_contig) {
                mpi_errno = MPIC_Isend(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, partner,
                                       MPIR_BCAST_TAG, comm_ptr, &requests[req_count++], coll_attr);
            } else {
                mpi_errno = MPIC_Isend(buf, count, datatype, partner,
                                       MPIR_BCAST_TAG, comm_ptr, &requests[req_count++], coll_attr);
            }
            MPIR_ERR_CHECK(mpi_errno);
        } else if (equal_lsbs) {
            if (!is_contig) {
                mpi_errno = MPIC_Recv(tmp_buf, nbytes, MPIR_BYTE_INTERNAL,
                                      partner, MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE);
            } else {
                mpi_errno = MPIC_Recv(buf, count, datatype,
                                      partner, MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE);
            }
            MPIR_ERR_CHECK(mpi_errno);
            recvd = 1;
        }
        mask >>= 1;
    }

    mpi_errno = MPIC_Waitall(req_count, requests, MPI_STATUSES_IGNORE);
    MPIR_ERR_CHECK(mpi_errno);

    if (vrank >= pow2) {
        /* recv partner */
        partner = vrank - pow2;
        partner = MPII_Bine_mod(partner + root, comm_size);
        if (!is_contig) {
            mpi_errno = MPIC_Recv(tmp_buf, nbytes, MPIR_BYTE_INTERNAL,
                                  partner, MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE);
        } else {
            mpi_errno = MPIC_Recv(buf, count, datatype,
                                  partner, MPIR_BCAST_TAG, comm_ptr, MPI_STATUS_IGNORE);
        }
        MPIR_ERR_CHECK(mpi_errno);
    } else if (vrank < comm_size - pow2) {
        /* send partner */
        partner = pow2 + vrank;
        partner = MPII_Bine_mod(partner + root, comm_size);
        if (!is_contig) {
            mpi_errno = MPIC_Send(tmp_buf, nbytes, MPIR_BYTE_INTERNAL,
                                  partner, MPIR_BCAST_TAG, comm_ptr, coll_attr);
        } else {
            mpi_errno = MPIC_Send(buf, count, datatype,
                                  partner, MPIR_BCAST_TAG, comm_ptr, coll_attr);
        }
        MPIR_ERR_CHECK(mpi_errno);
    }

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_Localcopy(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, buf, count, datatype);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
