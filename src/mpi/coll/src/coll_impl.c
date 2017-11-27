/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpiimpl.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_DEVICE_COLLECTIVES
      category    : COLLECTIVE
      type        : boolean
      default     : true
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        If set to true, MPI collectives will allow the device to override
        the MPIR-level collective algorithms. The device still has the
        option to call the MPIR-level algorithms manually.  If set to false,
        the device-level collective function will not be called.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/*
 * Initializing the below choice variables is not strictly necessary.
 * However, such uninitialized variables are common variables, which
 * some linkers treat as undefined references.  Initializing them to
 * AUTO keeps such linkers happy.
 *
 * More discussion on this can be found in the "--warn-common"
 * discussion of
 * https://sourceware.org/binutils/docs/ld/Options.html#Options
 */
int MPIR_Allgather_alg_intra_choice = MPIR_ALLGATHER_ALG_INTRA_AUTO;
int MPIR_Allgather_alg_inter_choice = MPIR_ALLGATHER_ALG_INTER_AUTO;
int MPIR_Allgatherv_alg_intra_choice = MPIR_ALLGATHERV_ALG_INTRA_AUTO;
int MPIR_Allgatherv_alg_inter_choice = MPIR_ALLGATHERV_ALG_INTER_AUTO;
int MPIR_Allreduce_alg_intra_choice = MPIR_ALLREDUCE_ALG_INTRA_AUTO;
int MPIR_Allreduce_alg_inter_choice = MPIR_ALLREDUCE_ALG_INTER_AUTO;
int MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_AUTO;
int MPIR_Alltoall_alg_inter_choice = MPIR_ALLTOALL_ALG_INTER_AUTO;
int MPIR_Alltoallv_alg_intra_choice = MPIR_ALLTOALLV_ALG_INTRA_AUTO;
int MPIR_Alltoallv_alg_inter_choice = MPIR_ALLTOALLV_ALG_INTER_AUTO;
int MPIR_Alltoallw_alg_intra_choice = MPIR_ALLTOALLW_ALG_INTRA_AUTO;
int MPIR_Alltoallw_alg_inter_choice = MPIR_ALLTOALLW_ALG_INTER_AUTO;
int MPIR_Barrier_alg_intra_choice = MPIR_BARRIER_ALG_INTRA_AUTO;
int MPIR_Barrier_alg_inter_choice = MPIR_BARRIER_ALG_INTER_AUTO;
int MPIR_Bcast_alg_intra_choice = MPIR_BCAST_ALG_INTRA_AUTO;
int MPIR_Bcast_alg_inter_choice = MPIR_BCAST_ALG_INTER_AUTO;
int MPIR_Exscan_alg_intra_choice = MPIR_EXSCAN_ALG_INTRA_AUTO;
int MPIR_Gather_alg_intra_choice = MPIR_GATHER_ALG_INTRA_AUTO;
int MPIR_Gather_alg_inter_choice = MPIR_GATHER_ALG_INTER_AUTO;
int MPIR_Gatherv_alg_intra_choice = MPIR_GATHERV_ALG_INTRA_AUTO;
int MPIR_Gatherv_alg_inter_choice = MPIR_GATHERV_ALG_INTER_AUTO;
int MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHER_ALG_INTRA_AUTO;
int MPIR_Iallgather_alg_inter_choice = MPIR_IALLGATHER_ALG_INTER_AUTO;
int MPIR_Iallgatherv_alg_intra_choice = MPIR_IALLGATHERV_ALG_INTRA_AUTO;
int MPIR_Iallgatherv_alg_inter_choice = MPIR_IALLGATHERV_ALG_INTER_AUTO;
int MPIR_Iallreduce_alg_intra_choice = MPIR_IALLREDUCE_ALG_INTRA_AUTO;
int MPIR_Iallreduce_alg_inter_choice = MPIR_IALLREDUCE_ALG_INTER_AUTO;
int MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_AUTO;
int MPIR_Ialltoall_alg_inter_choice = MPIR_IALLTOALL_ALG_INTER_AUTO;
int MPIR_Ialltoallv_alg_intra_choice = MPIR_IALLTOALLV_ALG_INTRA_AUTO;
int MPIR_Ialltoallv_alg_inter_choice = MPIR_IALLTOALLV_ALG_INTER_AUTO;
int MPIR_Ialltoallw_alg_intra_choice = MPIR_IALLTOALLW_ALG_INTRA_AUTO;
int MPIR_Ialltoallw_alg_inter_choice = MPIR_IALLTOALLW_ALG_INTER_AUTO;
int MPIR_Ibarrier_alg_intra_choice = MPIR_IBARRIER_ALG_INTRA_AUTO;
int MPIR_Ibarrier_alg_inter_choice = MPIR_IBARRIER_ALG_INTER_AUTO;
int MPIR_Ibcast_alg_intra_choice = MPIR_IBCAST_ALG_INTRA_AUTO;
int MPIR_Ibcast_alg_inter_choice = MPIR_IBCAST_ALG_INTER_AUTO;
int MPIR_Iexscan_alg_intra_choice = MPIR_IEXSCAN_ALG_INTRA_AUTO;
int MPIR_Igather_alg_intra_choice = MPIR_IGATHER_ALG_INTRA_AUTO;
int MPIR_Igather_alg_inter_choice = MPIR_IGATHER_ALG_INTER_AUTO;
int MPIR_Igatherv_alg_intra_choice = MPIR_IGATHERV_ALG_INTRA_AUTO;
int MPIR_Igatherv_alg_inter_choice = MPIR_IGATHERV_ALG_INTER_AUTO;
int MPIR_Inhb_allgather_alg_intra_choice = MPIR_INHB_ALLGATHER_ALG_INTRA_AUTO;
int MPIR_Inhb_allgather_alg_inter_choice = MPIR_INHB_ALLGATHER_ALG_INTER_AUTO;
int MPIR_Inhb_allgatherv_alg_intra_choice = MPIR_INHB_ALLGATHERV_ALG_INTRA_AUTO;
int MPIR_Inhb_allgatherv_alg_inter_choice = MPIR_INHB_ALLGATHERV_ALG_INTER_AUTO;
int MPIR_Inhb_alltoall_alg_intra_choice = MPIR_INHB_ALLTOALL_ALG_INTRA_AUTO;
int MPIR_Inhb_alltoall_alg_inter_choice = MPIR_INHB_ALLTOALL_ALG_INTER_AUTO;
int MPIR_Inhb_alltoallv_alg_intra_choice = MPIR_INHB_ALLTOALLV_ALG_INTRA_AUTO;
int MPIR_Inhb_alltoallv_alg_inter_choice = MPIR_INHB_ALLTOALLV_ALG_INTER_AUTO;
int MPIR_Inhb_alltoallw_alg_intra_choice = MPIR_INHB_ALLTOALLW_ALG_INTRA_AUTO;
int MPIR_Inhb_alltoallw_alg_inter_choice = MPIR_INHB_ALLTOALLW_ALG_INTER_AUTO;
int MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_AUTO;
int MPIR_Ireduce_scatter_alg_inter_choice = MPIR_IREDUCE_SCATTER_ALG_INTER_AUTO;
int MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_AUTO;
int MPIR_Ireduce_scatter_block_alg_inter_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTER_AUTO;
int MPIR_Ireduce_alg_intra_choice = MPIR_IREDUCE_ALG_INTRA_AUTO;
int MPIR_Ireduce_alg_inter_choice = MPIR_IREDUCE_ALG_INTER_AUTO;
int MPIR_Iscan_alg_intra_choice = MPIR_ISCAN_ALG_INTRA_AUTO;
int MPIR_Iscatter_alg_intra_choice = MPIR_ISCATTER_ALG_INTRA_AUTO;
int MPIR_Iscatter_alg_inter_choice = MPIR_ISCATTER_ALG_INTER_AUTO;
int MPIR_Iscatterv_alg_intra_choice = MPIR_ISCATTERV_ALG_INTRA_AUTO;
int MPIR_Iscatterv_alg_inter_choice = MPIR_ISCATTERV_ALG_INTER_AUTO;
int MPIR_Nhb_allgather_alg_intra_choice = MPIR_NHB_ALLGATHER_ALG_INTRA_AUTO;
int MPIR_Nhb_allgather_alg_inter_choice = MPIR_NHB_ALLGATHER_ALG_INTER_AUTO;
int MPIR_Nhb_allgatherv_alg_intra_choice = MPIR_NHB_ALLGATHERV_ALG_INTRA_AUTO;
int MPIR_Nhb_allgatherv_alg_inter_choice = MPIR_NHB_ALLGATHERV_ALG_INTER_AUTO;
int MPIR_Nhb_alltoall_alg_intra_choice = MPIR_NHB_ALLTOALL_ALG_INTRA_AUTO;
int MPIR_Nhb_alltoall_alg_inter_choice = MPIR_NHB_ALLTOALL_ALG_INTER_AUTO;
int MPIR_Nhb_alltoallv_alg_intra_choice = MPIR_NHB_ALLTOALLV_ALG_INTRA_AUTO;
int MPIR_Nhb_alltoallv_alg_inter_choice = MPIR_NHB_ALLTOALLV_ALG_INTER_AUTO;
int MPIR_Nhb_alltoallw_alg_intra_choice = MPIR_NHB_ALLTOALLW_ALG_INTRA_AUTO;
int MPIR_Nhb_alltoallw_alg_inter_choice = MPIR_NHB_ALLTOALLW_ALG_INTER_AUTO;
int MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_AUTO;
int MPIR_Reduce_scatter_alg_inter_choice = MPIR_REDUCE_SCATTER_ALG_INTER_AUTO;
int MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_AUTO;
int MPIR_Reduce_scatter_block_alg_inter_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTER_AUTO;
int MPIR_Reduce_alg_intra_choice = MPIR_REDUCE_ALG_INTRA_AUTO;
int MPIR_Reduce_alg_inter_choice = MPIR_REDUCE_ALG_INTER_AUTO;
int MPIR_Scan_alg_intra_choice = MPIR_SCAN_ALG_INTRA_AUTO;
int MPIR_Scatterv_alg_intra_choice = MPIR_SCATTERV_ALG_INTRA_AUTO;
int MPIR_Scatterv_alg_inter_choice = MPIR_SCATTERV_ALG_INTER_AUTO;
int MPIR_Scatter_alg_intra_choice = MPIR_SCATTER_ALG_INTRA_AUTO;
int MPIR_Scatter_alg_inter_choice = MPIR_SCATTER_ALG_INTER_AUTO;

int MPIR_COLL_init(void) {
    int mpi_errno = MPI_SUCCESS;

    /* =========================================================================== */
    /* | Check if the user manually selected any collective algorithms via CVARs | */
    /* =========================================================================== */

    /* Allgather Intra */
    if (0 == strcmp(MPIR_CVAR_ALLGATHER_ALGORITHM_INTRA, "brucks"))
        MPIR_Allgather_alg_intra_choice = MPIR_ALLGATHER_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Allgather_alg_intra_choice = MPIR_ALLGATHER_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_ALGORITHM_INTRA, "ring"))
        MPIR_Allgather_alg_intra_choice = MPIR_ALLGATHER_ALG_INTRA_RING;
    else
        MPIR_Allgather_alg_intra_choice = MPIR_ALLGATHER_ALG_INTRA_AUTO;

    /* Allgather Inter */
    if (0 == strcmp(MPIR_CVAR_ALLGATHER_ALGORITHM_INTER, "generic"))
        MPIR_Allgather_alg_inter_choice = MPIR_ALLGATHER_ALG_INTER_GENERIC;
    else
        MPIR_Allgather_alg_inter_choice = MPIR_ALLGATHER_ALG_INTER_AUTO;

    /* Allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_ALLGATHERV_ALGORITHM_INTRA, "brucks"))
        MPIR_Allgatherv_alg_intra_choice = MPIR_ALLGATHERV_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Allgatherv_alg_intra_choice = MPIR_ALLGATHERV_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_ALGORITHM_INTRA, "ring"))
        MPIR_Allgatherv_alg_intra_choice = MPIR_ALLGATHERV_ALG_INTRA_RING;
    else
        MPIR_Allgatherv_alg_intra_choice = MPIR_ALLGATHERV_ALG_INTRA_AUTO;

    /* Allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_ALLGATHERV_ALGORITHM_INTER, "generic"))
        MPIR_Allgatherv_alg_inter_choice = MPIR_ALLGATHERV_ALG_INTER_GENERIC;
    else
        MPIR_Allgatherv_alg_inter_choice = MPIR_ALLGATHERV_ALG_INTER_AUTO;

    /* Allreduce Intra */
    if (0 == strcmp(MPIR_CVAR_ALLREDUCE_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Allreduce_alg_intra_choice = MPIR_ALLREDUCE_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLREDUCE_ALGORITHM_INTRA, "redscat_allgather"))
        MPIR_Allreduce_alg_intra_choice = MPIR_ALLREDUCE_ALG_INTRA_REDSCAT_ALLGATHER;
    else
        MPIR_Allreduce_alg_intra_choice = MPIR_ALLREDUCE_ALG_INTRA_AUTO;

    /* Allreduce Inter */
    if (0 == strcmp(MPIR_CVAR_ALLREDUCE_ALGORITHM_INTER, "generic"))
        MPIR_Allreduce_alg_inter_choice = MPIR_ALLREDUCE_ALG_INTER_GENERIC;
    else
        MPIR_Allreduce_alg_inter_choice = MPIR_ALLREDUCE_ALG_INTER_AUTO;

    /* Alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALL_ALGORITHM_INTRA, "brucks"))
        MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_ALGORITHM_INTRA, "pairwise"))
        MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_ALGORITHM_INTRA, "pairwise_sendrecv_replace"))
        MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_ALGORITHM_INTRA, "scattered"))
        MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_SCATTERED;
    else
        MPIR_Alltoall_alg_intra_choice = MPIR_ALLTOALL_ALG_INTRA_AUTO;

    /* Alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALL_ALGORITHM_INTER, "generic"))
        MPIR_Alltoall_alg_inter_choice = MPIR_ALLTOALL_ALG_INTER_GENERIC;
    else
        MPIR_Alltoall_alg_inter_choice = MPIR_ALLTOALL_ALG_INTER_AUTO;

    /* Alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLV_ALGORITHM_INTRA, "pairwise_sendrecv_replace"))
        MPIR_Alltoallv_alg_intra_choice = MPIR_ALLTOALLV_ALG_INTRA_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLV_ALGORITHM_INTRA, "scattered"))
        MPIR_Alltoallv_alg_intra_choice = MPIR_ALLTOALLV_ALG_INTRA_SCATTERED;
    else
        MPIR_Alltoallv_alg_intra_choice = MPIR_ALLTOALLV_ALG_INTRA_AUTO;

    /* Alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLV_ALGORITHM_INTER, "generic"))
        MPIR_Alltoallv_alg_inter_choice = MPIR_ALLTOALLV_ALG_INTER_GENERIC;
    else
        MPIR_Alltoallv_alg_inter_choice = MPIR_ALLTOALLV_ALG_INTER_AUTO;

    /* Alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLW_ALGORITHM_INTRA, "pairwise_sendrecv_replace"))
        MPIR_Alltoallw_alg_intra_choice = MPIR_ALLTOALLW_ALG_INTRA_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLW_ALGORITHM_INTRA, "scattered"))
        MPIR_Alltoallw_alg_intra_choice = MPIR_ALLTOALLW_ALG_INTRA_SCATTERED;
    else
        MPIR_Alltoallw_alg_intra_choice = MPIR_ALLTOALLW_ALG_INTRA_AUTO;

    /* Alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLW_ALGORITHM_INTER, "generic"))
        MPIR_Alltoallw_alg_inter_choice = MPIR_ALLTOALLW_ALG_INTER_GENERIC;
    else
        MPIR_Alltoallw_alg_inter_choice = MPIR_ALLTOALLW_ALG_INTER_AUTO;

    /* Barrier Intra */
    if (0 == strcmp(MPIR_CVAR_BARRIER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Barrier_alg_intra_choice = MPIR_BARRIER_ALG_INTRA_RECURSIVE_DOUBLING;
    else
        MPIR_Barrier_alg_intra_choice = MPIR_BARRIER_ALG_INTRA_AUTO;

    /* Barrier Inter */
    if (0 == strcmp(MPIR_CVAR_BARRIER_ALGORITHM_INTER, "generic"))
        MPIR_Barrier_alg_inter_choice = MPIR_BARRIER_ALG_INTER_GENERIC;
    else
        MPIR_Barrier_alg_inter_choice = MPIR_BARRIER_ALG_INTER_AUTO;

    /* Bcast Intra */
    if (0 == strcmp(MPIR_CVAR_BCAST_ALGORITHM_INTRA, "binomial"))
        MPIR_Bcast_alg_intra_choice = MPIR_BCAST_ALG_INTRA_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_BCAST_ALGORITHM_INTRA, "scatter_doubling_allgather"))
        MPIR_Bcast_alg_intra_choice = MPIR_BCAST_ALG_INTRA_SCATTER_DOUBLING_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_BCAST_ALGORITHM_INTRA, "scatter_ring_allgather"))
        MPIR_Bcast_alg_intra_choice = MPIR_BCAST_ALG_INTRA_SCATTER_RING_ALLGATHER;
    else
        MPIR_Bcast_alg_intra_choice = MPIR_BCAST_ALG_INTRA_AUTO;

    /* Bcast Inter */
    if (0 == strcmp(MPIR_CVAR_BCAST_ALGORITHM_INTER, "generic"))
        MPIR_Bcast_alg_inter_choice = MPIR_BCAST_ALG_INTER_GENERIC;
    else
        MPIR_Bcast_alg_inter_choice = MPIR_BCAST_ALG_INTER_AUTO;

    /* Exscan Intra */
    if (0 == strcmp(MPIR_CVAR_EXSCAN_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Exscan_alg_intra_choice = MPIR_EXSCAN_ALG_INTRA_RECURSIVE_DOUBLING;
    else
        MPIR_Exscan_alg_intra_choice = MPIR_EXSCAN_ALG_INTRA_AUTO;

    /* Gather Intra */
    if (0 == strcmp(MPIR_CVAR_GATHER_ALGORITHM_INTRA, "binomial"))
        MPIR_Gather_alg_intra_choice = MPIR_GATHER_ALG_INTRA_BINOMIAL;
    else
        MPIR_Gather_alg_intra_choice = MPIR_GATHER_ALG_INTRA_AUTO;

    /* Gather Inter */
    if (0 == strcmp(MPIR_CVAR_GATHER_ALGORITHM_INTER, "generic"))
        MPIR_Gather_alg_inter_choice = MPIR_GATHER_ALG_INTER_GENERIC;
    else
        MPIR_Gather_alg_inter_choice = MPIR_GATHER_ALG_INTER_AUTO;

    /* Gatherv Intra */
    if (0 == strcmp(MPIR_CVAR_GATHERV_ALGORITHM_INTRA, "linear"))
        MPIR_Gather_alg_intra_choice = MPIR_GATHERV_ALG_INTRA_LINEAR;
    else
        MPIR_Gather_alg_intra_choice = MPIR_GATHERV_ALG_INTRA_AUTO;

    /* Gatherv Inter */
    if (0 == strcmp(MPIR_CVAR_GATHERV_ALGORITHM_INTER, "linear"))
        MPIR_Gather_alg_inter_choice = MPIR_GATHERV_ALG_INTER_LINEAR;
    else
        MPIR_Gather_alg_inter_choice = MPIR_GATHERV_ALG_INTER_AUTO;

    /* Iallgather Intra */
    if (0 == strcmp(MPIR_CVAR_IALLGATHER_ALGORITHM_INTRA, "brucks"))
        MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHER_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHER_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_ALGORITHM_INTRA, "ring"))
        MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHER_ALG_INTRA_RING;
    else
        MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHER_ALG_INTRA_AUTO;

    /* Iallgather Inter */
    if (0 == strcmp(MPIR_CVAR_IALLGATHER_ALGORITHM_INTER, "generic"))
        MPIR_Iallgather_alg_inter_choice = MPIR_IALLGATHER_ALG_INTER_GENERIC;
    else
        MPIR_Iallgather_alg_inter_choice = MPIR_IALLGATHER_ALG_INTER_AUTO;

    /* Iallgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_IALLGATHERV_ALGORITHM_INTRA, "brucks"))
        MPIR_Iallgatherv_alg_intra_choice = MPIR_IALLGATHERV_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Iallgatherv_alg_intra_choice = MPIR_IALLGATHERV_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_ALGORITHM_INTRA, "ring"))
        MPIR_Iallgatherv_alg_intra_choice = MPIR_IALLGATHERV_ALG_INTRA_RING;
    else
        MPIR_Iallgather_alg_intra_choice = MPIR_IALLGATHERV_ALG_INTRA_AUTO;

    /* Iallgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_IALLGATHERV_ALGORITHM_INTER, "generic"))
        MPIR_Iallgatherv_alg_inter_choice = MPIR_IALLGATHERV_ALG_INTER_GENERIC;
    else
        MPIR_Iallgatherv_alg_inter_choice = MPIR_IALLGATHERV_ALG_INTER_AUTO;

    /* Iallreduce Intra */
    if (0 == strcmp(MPIR_CVAR_IALLREDUCE_ALGORITHM_INTRA, "naive"))
        MPIR_Iallreduce_alg_intra_choice = MPIR_IALLREDUCE_ALG_INTRA_NAIVE;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Iallreduce_alg_intra_choice = MPIR_IALLREDUCE_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_ALGORITHM_INTRA, "redscat_allgather"))
        MPIR_Iallreduce_alg_intra_choice = MPIR_IALLREDUCE_ALG_INTRA_REDSCAT_ALLGATHER;
    else
        MPIR_Iallreduce_alg_intra_choice = MPIR_IALLREDUCE_ALG_INTRA_AUTO;

    /* Iallreduce Inter */
    if (0 == strcmp(MPIR_CVAR_IALLREDUCE_ALGORITHM_INTER, "generic"))
        MPIR_Iallreduce_alg_inter_choice = MPIR_IALLREDUCE_ALG_INTER_GENERIC;
    else
        MPIR_Iallreduce_alg_inter_choice = MPIR_IALLREDUCE_ALG_INTER_AUTO;

    /* Ialltoall Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALL_ALGORITHM_INTRA, "brucks"))
        MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_ALGORITHM_INTRA, "inplace"))
        MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_INPLACE;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_ALGORITHM_INTRA, "pairwise"))
        MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_ALGORITHM_INTRA, "perm_sr"))
        MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_PERM_SR;
    else
        MPIR_Ialltoall_alg_intra_choice = MPIR_IALLTOALL_ALG_INTRA_AUTO;

    /* Ialltoall Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALL_ALGORITHM_INTER, "generic"))
        MPIR_Ialltoall_alg_inter_choice = MPIR_IALLTOALL_ALG_INTER_GENERIC;
    else
        MPIR_Ialltoall_alg_inter_choice = MPIR_IALLTOALL_ALG_INTER_AUTO;

    /* Ialltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLV_ALGORITHM_INTRA, "blocked"))
        MPIR_Ialltoallv_alg_intra_choice = MPIR_IALLTOALLV_ALG_INTRA_BLOCKED;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALLV_ALGORITHM_INTRA, "inplace"))
        MPIR_Ialltoallv_alg_intra_choice = MPIR_IALLTOALLV_ALG_INTRA_INPLACE;
    else
        MPIR_Ialltoallv_alg_intra_choice = MPIR_IALLTOALLV_ALG_INTRA_AUTO;

    /* Ialltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLV_ALGORITHM_INTER, "pairwise_xchg"))
        MPIR_Ialltoallv_alg_inter_choice = MPIR_IALLTOALLV_ALG_INTER_PAIRWISE_XCHG;
    else
        MPIR_Ialltoallv_alg_inter_choice = MPIR_IALLTOALLV_ALG_INTER_AUTO;

    /* Ialltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLW_ALGORITHM_INTRA, "blocked"))
        MPIR_Ialltoallw_alg_intra_choice = MPIR_IALLTOALLW_ALG_INTRA_BLOCKED;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALLW_ALGORITHM_INTRA, "inplace"))
        MPIR_Ialltoallw_alg_intra_choice = MPIR_IALLTOALLW_ALG_INTRA_INPLACE;
    else
        MPIR_Ialltoallw_alg_intra_choice = MPIR_IALLTOALLW_ALG_INTRA_AUTO;

    /* Ialltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLW_ALGORITHM_INTER, "pairwise_xchg"))
        MPIR_Ialltoallw_alg_inter_choice = MPIR_IALLTOALLW_ALG_INTER_PAIRWISE_XCHG;
    else
        MPIR_Ialltoallw_alg_inter_choice = MPIR_IALLTOALLW_ALG_INTER_AUTO;

    /* Ibarrier Intra */
    if (0 == strcmp(MPIR_CVAR_IBARRIER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Ibarrier_alg_intra_choice = MPIR_IBARRIER_ALG_INTRA_RECURSIVE_DOUBLING;
    else
        MPIR_Ibarrier_alg_intra_choice = MPIR_IBARRIER_ALG_INTRA_AUTO;

    /* Ibarrier Inter */
    if (0 == strcmp(MPIR_CVAR_IBARRIER_ALGORITHM_INTER, "bcast"))
        MPIR_Ibarrier_alg_inter_choice = MPIR_IBARRIER_ALG_INTER_BCAST;
    else
        MPIR_Ibarrier_alg_inter_choice = MPIR_IBARRIER_ALG_INTER_AUTO;

    /* Ibcast Intra */
    if (0 == strcmp(MPIR_CVAR_IBCAST_ALGORITHM_INTRA, "binomial"))
        MPIR_Ibcast_alg_intra_choice = MPIR_IBCAST_ALG_INTRA_BINOMIAL;
    else
        MPIR_Ibcast_alg_intra_choice = MPIR_IBCAST_ALG_INTRA_AUTO;

    /* Ibcast Inter */
    if (0 == strcmp(MPIR_CVAR_IBCAST_ALGORITHM_INTER, "flat"))
        MPIR_Ibcast_alg_inter_choice = MPIR_IBCAST_ALG_INTER_FLAT;
    else
        MPIR_Ibcast_alg_inter_choice = MPIR_IBCAST_ALG_INTER_AUTO;

    /* Iexscan Intra */
    if (0 == strcmp(MPIR_CVAR_IEXSCAN_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Iexscan_alg_intra_choice = MPIR_IEXSCAN_ALG_INTRA_RECURSIVE_DOUBLING;
    else
        MPIR_Iexscan_alg_intra_choice = MPIR_IEXSCAN_ALG_INTRA_AUTO;

    /* Igather Intra */
    if (0 == strcmp(MPIR_CVAR_IGATHER_ALGORITHM_INTRA, "binomial"))
        MPIR_Igather_alg_intra_choice = MPIR_IGATHER_ALG_INTRA_BINOMIAL;
    else
        MPIR_Igather_alg_intra_choice = MPIR_IGATHER_ALG_INTRA_AUTO;

    /* Igather Inter */
    if (0 == strcmp(MPIR_CVAR_IGATHER_ALGORITHM_INTER, "long_inter"))
        MPIR_Igather_alg_inter_choice = MPIR_IGATHER_ALG_INTER_LONG;
    else if (0 == strcmp(MPIR_CVAR_IGATHER_ALGORITHM_INTRA, "short_inter"))
        MPIR_Igather_alg_inter_choice = MPIR_IGATHER_ALG_INTER_SHORT;
    else
        MPIR_Igather_alg_inter_choice = MPIR_IGATHER_ALG_INTER_AUTO;

    /* Igatherv Intra */
    if (0 == strcmp(MPIR_CVAR_IGATHERV_ALGORITHM_INTRA, "generic"))
        MPIR_Igatherv_alg_intra_choice = MPIR_IGATHERV_ALG_INTRA_GENERIC;
    else
        MPIR_Igatherv_alg_intra_choice = MPIR_IGATHERV_ALG_INTRA_AUTO;

    /* Igatherv Inter */
    if (0 == strcmp(MPIR_CVAR_IGATHERV_ALGORITHM_INTER, "generic"))
        MPIR_Igatherv_alg_inter_choice = MPIR_IGATHERV_ALG_INTER_GENERIC;
    else
        MPIR_Igatherv_alg_inter_choice = MPIR_IGATHERV_ALG_INTER_AUTO;

    /* Inhb_allgather Intra */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLGATHER_ALGORITHM_INTRA, "generic"))
        MPIR_Inhb_allgather_alg_intra_choice = MPIR_INHB_ALLGATHER_ALG_INTRA_GENERIC;
    else
        MPIR_Inhb_allgather_alg_intra_choice = MPIR_INHB_ALLGATHER_ALG_INTRA_AUTO;

    /* Inhb_allgather Inter */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLGATHER_ALGORITHM_INTER, "generic"))
        MPIR_Inhb_allgather_alg_inter_choice = MPIR_INHB_ALLGATHER_ALG_INTER_GENERIC;
    else
        MPIR_Inhb_allgather_alg_inter_choice = MPIR_INHB_ALLGATHER_ALG_INTER_AUTO;

    /* Inhb_allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLGATHERV_ALGORITHM_INTRA, "generic"))
        MPIR_Inhb_allgatherv_alg_intra_choice = MPIR_INHB_ALLGATHERV_ALG_INTRA_GENERIC;
    else
        MPIR_Inhb_allgatherv_alg_intra_choice = MPIR_INHB_ALLGATHERV_ALG_INTRA_AUTO;

    /* Inhb_allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLGATHERV_ALGORITHM_INTER, "generic"))
        MPIR_Inhb_allgatherv_alg_inter_choice = MPIR_INHB_ALLGATHERV_ALG_INTER_GENERIC;
    else
        MPIR_Inhb_allgatherv_alg_inter_choice = MPIR_INHB_ALLGATHERV_ALG_INTER_AUTO;

    /* Inhb_alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALL_ALGORITHM_INTRA, "generic"))
        MPIR_Inhb_alltoall_alg_intra_choice = MPIR_INHB_ALLTOALL_ALG_INTRA_GENERIC;
    else
        MPIR_Inhb_alltoall_alg_intra_choice = MPIR_INHB_ALLTOALL_ALG_INTRA_AUTO;

    /* Inhb_alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALL_ALGORITHM_INTER, "generic"))
        MPIR_Inhb_alltoall_alg_inter_choice = MPIR_INHB_ALLTOALL_ALG_INTER_GENERIC;
    else
        MPIR_Inhb_alltoall_alg_inter_choice = MPIR_INHB_ALLTOALL_ALG_INTER_AUTO;

    /* Inhb_alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALLV_ALGORITHM_INTRA, "generic"))
        MPIR_Inhb_alltoallv_alg_intra_choice = MPIR_INHB_ALLTOALLV_ALG_INTRA_GENERIC;
    else
        MPIR_Inhb_alltoallv_alg_intra_choice = MPIR_INHB_ALLTOALLV_ALG_INTRA_AUTO;

    /* Inhb_alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALLV_ALGORITHM_INTER, "generic"))
        MPIR_Inhb_alltoallv_alg_inter_choice = MPIR_INHB_ALLTOALLV_ALG_INTER_GENERIC;
    else
        MPIR_Inhb_alltoallv_alg_inter_choice = MPIR_INHB_ALLTOALLV_ALG_INTER_AUTO;

    /* Inhb_alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALLW_ALGORITHM_INTRA, "generic"))
        MPIR_Inhb_alltoallw_alg_intra_choice = MPIR_INHB_ALLTOALLW_ALG_INTRA_GENERIC;
    else
        MPIR_Inhb_alltoallw_alg_intra_choice = MPIR_INHB_ALLTOALLW_ALG_INTRA_AUTO;

    /* Inhb_alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_INHB_ALLTOALLW_ALGORITHM_INTER, "generic"))
        MPIR_Inhb_alltoallw_alg_inter_choice = MPIR_INHB_ALLTOALLW_ALG_INTER_GENERIC;
    else
        MPIR_Inhb_alltoallw_alg_inter_choice = MPIR_INHB_ALLTOALLW_ALG_INTER_AUTO;

    /* Ired_scat Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_ALGORITHM_INTRA, "noncomm"))
        MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_NONCOMM;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_ALGORITHM_INTRA, "pairwise"))
        MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_ALGORITHM_INTRA, "recursive_halving"))
        MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_RECURSIVE_HALVING;
    else
        MPIR_Ireduce_scatter_alg_intra_choice = MPIR_IREDUCE_SCATTER_ALG_INTRA_AUTO;

    /* Ired_scat Inter */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_ALGORITHM_INTER, "generic"))
        MPIR_Ireduce_scatter_alg_inter_choice = MPIR_IREDUCE_SCATTER_ALG_INTER_GENERIC;
    else
        MPIR_Ireduce_scatter_alg_inter_choice = MPIR_IREDUCE_SCATTER_ALG_INTER_AUTO;

    /* Ired_scat_block Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "noncomm"))
        MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_NONCOMM;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "pairwise"))
        MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "recursive_halving"))
        MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_RECURSIVE_HALVING;
    else
        MPIR_Ireduce_scatter_block_alg_intra_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTRA_AUTO;

    /* Ired_scat_block Inter */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_ALGORITHM_INTER, "generic"))
        MPIR_Ireduce_scatter_block_alg_inter_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTER_GENERIC;
    else
        MPIR_Ireduce_scatter_block_alg_inter_choice = MPIR_IREDUCE_SCATTER_BLOCK_ALG_INTER_AUTO;

    /* Ireduce Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_ALGORITHM_INTRA, "binomial"))
        MPIR_Ireduce_alg_intra_choice = MPIR_IREDUCE_ALG_INTRA_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_ALGORITHM_INTRA, "redscat_gather"))
        MPIR_Ireduce_alg_intra_choice = MPIR_IREDUCE_ALG_INTRA_REDSCAT_GATHER;
    else
        MPIR_Ireduce_alg_intra_choice = MPIR_IREDUCE_ALG_INTRA_AUTO;

    /* Ireduce Inter */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_ALGORITHM_INTER, "generic"))
        MPIR_Ireduce_alg_inter_choice = MPIR_IREDUCE_ALG_INTER_GENERIC;
    else
        MPIR_Ireduce_alg_inter_choice = MPIR_IREDUCE_ALG_INTER_AUTO;

    /* Iscan Intra */
    if (0 == strcmp(MPIR_CVAR_ISCAN_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Iscan_alg_intra_choice = MPIR_ISCAN_ALG_INTRA_RECURSIVE_DOUBLING;
    else
        MPIR_Iscan_alg_intra_choice = MPIR_ISCAN_ALG_INTRA_AUTO;

    /* Iscatter Intra */
    if (0 == strcmp(MPIR_CVAR_ISCATTER_ALGORITHM_INTRA, "binomial"))
        MPIR_Iscatter_alg_intra_choice = MPIR_ISCATTER_ALG_INTRA_BINOMIAL;
    else
        MPIR_Iscatter_alg_intra_choice = MPIR_ISCATTER_ALG_INTRA_AUTO;

    /* Iscatter Inter */
    if (0 == strcmp(MPIR_CVAR_ISCATTER_ALGORITHM_INTER, "generic"))
        MPIR_Iscatter_alg_inter_choice = MPIR_ISCATTER_ALG_INTER_GENERIC;
    else
        MPIR_Iscatter_alg_inter_choice = MPIR_ISCATTER_ALG_INTER_AUTO;

    /* Iscatterv Intra */
    if (0 == strcmp(MPIR_CVAR_ISCATTERV_ALGORITHM_INTRA, "linear"))
        MPIR_Iscatterv_alg_intra_choice = MPIR_ISCATTERV_ALG_INTRA_LINEAR;
    else
        MPIR_Iscatterv_alg_intra_choice = MPIR_ISCATTERV_ALG_INTRA_AUTO;

    /* Iscatterv Inter */
    if (0 == strcmp(MPIR_CVAR_ISCATTERV_ALGORITHM_INTER, "linear"))
        MPIR_Iscatterv_alg_inter_choice = MPIR_ISCATTERV_ALG_INTER_LINEAR;
    else
        MPIR_Iscatterv_alg_inter_choice = MPIR_ISCATTERV_ALG_INTER_AUTO;

    /* Nhb_allgather Intra */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLGATHER_ALGORITHM_INTRA, "nb"))
        MPIR_Nhb_allgather_alg_intra_choice = MPIR_NHB_ALLGATHER_ALG_INTRA_NB;
    else
        MPIR_Nhb_allgather_alg_intra_choice = MPIR_NHB_ALLGATHER_ALG_INTRA_AUTO;

    /* Nhb_allgather Inter */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLGATHER_ALGORITHM_INTER, "nb"))
        MPIR_Nhb_allgather_alg_inter_choice = MPIR_NHB_ALLGATHER_ALG_INTER_NB;
    else
        MPIR_Nhb_allgather_alg_inter_choice = MPIR_NHB_ALLGATHER_ALG_INTER_AUTO;

    /* Nhb_allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLGATHERV_ALGORITHM_INTRA, "nb"))
        MPIR_Nhb_allgatherv_alg_intra_choice = MPIR_NHB_ALLGATHERV_ALG_INTRA_NB;
    else
        MPIR_Nhb_allgatherv_alg_intra_choice = MPIR_NHB_ALLGATHERV_ALG_INTRA_AUTO;

    /* Nhb_allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLGATHERV_ALGORITHM_INTER, "nb"))
        MPIR_Nhb_allgatherv_alg_inter_choice = MPIR_NHB_ALLGATHERV_ALG_INTER_NB;
    else
        MPIR_Nhb_allgatherv_alg_inter_choice = MPIR_NHB_ALLGATHERV_ALG_INTER_AUTO;

    /* Nhb_alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALL_ALGORITHM_INTRA, "nb"))
        MPIR_Nhb_alltoall_alg_intra_choice = MPIR_NHB_ALLTOALL_ALG_INTRA_NB;
    else
        MPIR_Nhb_alltoall_alg_intra_choice = MPIR_NHB_ALLTOALL_ALG_INTRA_AUTO;

    /* Nhb_alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALL_ALGORITHM_INTER, "nb"))
        MPIR_Nhb_alltoall_alg_inter_choice = MPIR_NHB_ALLTOALL_ALG_INTER_NB;
    else
        MPIR_Nhb_alltoall_alg_inter_choice = MPIR_NHB_ALLTOALL_ALG_INTER_AUTO;

    /* Nhb_alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALLV_ALGORITHM_INTRA, "nb"))
        MPIR_Nhb_alltoallv_alg_intra_choice = MPIR_NHB_ALLTOALLV_ALG_INTRA_NB;
    else
        MPIR_Nhb_alltoallv_alg_intra_choice = MPIR_NHB_ALLTOALLV_ALG_INTRA_AUTO;

    /* Nhb_alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALLV_ALGORITHM_INTER, "nb"))
        MPIR_Nhb_alltoallv_alg_inter_choice = MPIR_NHB_ALLTOALLV_ALG_INTER_NB;
    else
        MPIR_Nhb_alltoallv_alg_inter_choice = MPIR_NHB_ALLTOALLV_ALG_INTER_AUTO;

    /* Nhb_alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALLW_ALGORITHM_INTRA, "nb"))
        MPIR_Nhb_alltoallw_alg_intra_choice = MPIR_NHB_ALLTOALLW_ALG_INTRA_NB;
    else
        MPIR_Nhb_alltoallw_alg_intra_choice = MPIR_NHB_ALLTOALLW_ALG_INTRA_AUTO;

    /* Nhb_alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_NHB_ALLTOALLW_ALGORITHM_INTER, "nb"))
        MPIR_Nhb_alltoallw_alg_inter_choice = MPIR_NHB_ALLTOALLW_ALG_INTER_NB;
    else
        MPIR_Nhb_alltoallw_alg_inter_choice = MPIR_NHB_ALLTOALLW_ALG_INTER_AUTO;

    /* Red_scat Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_ALGORITHM_INTRA, "noncomm"))
        MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_NONCOMM;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_ALGORITHM_INTRA, "pairwise"))
        MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_ALGORITHM_INTRA, "recursive_halving"))
        MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_RECURSIVE_HALVING;
    else
        MPIR_Reduce_scatter_alg_intra_choice = MPIR_REDUCE_SCATTER_ALG_INTRA_AUTO;

    /* Red_scat Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_ALGORITHM_INTER, "generic"))
        MPIR_Reduce_scatter_alg_inter_choice = MPIR_REDUCE_SCATTER_ALG_INTER_GENERIC;
    else
        MPIR_Reduce_scatter_alg_inter_choice = MPIR_REDUCE_SCATTER_ALG_INTER_AUTO;

    /* Red_scat_block Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "noncomm"))
        MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_NONCOMM;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "pairwise"))
        MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "recursive_doubling"))
        MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_ALGORITHM_INTRA, "recursive_halving"))
        MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_RECURSIVE_HALVING;
    else
        MPIR_Reduce_scatter_block_alg_intra_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTRA_AUTO;

    /* Red_scat_block Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_ALGORITHM_INTER, "generic"))
        MPIR_Reduce_scatter_block_alg_inter_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTER_GENERIC;
    else
        MPIR_Reduce_scatter_block_alg_inter_choice = MPIR_REDUCE_SCATTER_BLOCK_ALG_INTER_AUTO;

    /* Reduce Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_ALGORITHM_INTRA, "binomial"))
        MPIR_Reduce_alg_intra_choice = MPIR_REDUCE_ALG_INTRA_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_ALGORITHM_INTRA, "redscat_gather"))
        MPIR_Reduce_alg_intra_choice = MPIR_REDUCE_ALG_INTRA_REDSCAT_GATHER;
    else
        MPIR_Reduce_alg_intra_choice = MPIR_REDUCE_ALG_INTRA_AUTO;

    /* Reduce Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_ALGORITHM_INTER, "generic"))
        MPIR_Reduce_alg_inter_choice = MPIR_REDUCE_ALG_INTER_GENERIC;
    else
        MPIR_Reduce_alg_inter_choice = MPIR_REDUCE_ALG_INTER_AUTO;

    /* Scan Intra */
    if (0 == strcmp(MPIR_CVAR_SCAN_ALGORITHM_INTRA, "generic"))
        MPIR_Scan_alg_intra_choice = MPIR_SCAN_ALG_INTRA_GENERIC;
    else
        MPIR_Scan_alg_intra_choice = MPIR_SCAN_ALG_INTRA_AUTO;

    /* Scatter Intra */
    if (0 == strcmp(MPIR_CVAR_SCATTER_ALGORITHM_INTRA, "binomial"))
        MPIR_Scatter_alg_intra_choice = MPIR_SCATTER_ALG_INTRA_BINOMIAL;
    else
        MPIR_Scatter_alg_intra_choice = MPIR_SCATTER_ALG_INTRA_AUTO;

    /* Scatter Inter */
    if (0 == strcmp(MPIR_CVAR_SCATTER_ALGORITHM_INTRA, "generic"))
        MPIR_Scatter_alg_inter_choice = MPIR_SCATTER_ALG_INTER_GENERIC;
    else
        MPIR_Scatter_alg_inter_choice = MPIR_SCATTER_ALG_INTER_AUTO;

    /* Scatterv Intra */
    if (0 == strcmp(MPIR_CVAR_SCATTERV_ALGORITHM_INTRA, "linear"))
        MPIR_Scatterv_alg_intra_choice = MPIR_SCATTERV_ALG_INTRA_LINEAR;
    else
        MPIR_Scatterv_alg_intra_choice = MPIR_SCATTERV_ALG_INTRA_AUTO;

    /* Scatterv Inter */
    if (0 == strcmp(MPIR_CVAR_SCATTERV_ALGORITHM_INTER, "linear"))
        MPIR_Scatterv_alg_inter_choice = MPIR_SCATTERV_ALG_INTER_LINEAR;
    else
        MPIR_Scatterv_alg_inter_choice = MPIR_SCATTERV_ALG_INTER_AUTO;

    return mpi_errno;
}

