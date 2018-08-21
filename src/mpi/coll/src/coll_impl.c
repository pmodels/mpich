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
#include "coll_impl.h"

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

int MPIR_Nbc_progress_hook_id = 0;
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
MPIR_Allgather_intra_algo_t MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_AUTO;
MPIR_Allgather_inter_algo_t MPIR_Allgather_inter_algo_choice = MPIR_ALLGATHER_INTER_ALGO_AUTO;
MPIR_Allgatherv_intra_algo_t MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_AUTO;
MPIR_Allgatherv_inter_algo_t MPIR_Allgatherv_inter_algo_choice = MPIR_ALLGATHERV_INTER_ALGO_AUTO;
MPIR_Allreduce_intra_algo_t MPIR_Allreduce_intra_algo_choice = MPIR_ALLREDUCE_INTRA_ALGO_AUTO;
MPIR_Allreduce_inter_algo_t MPIR_Allreduce_inter_algo_choice = MPIR_ALLREDUCE_INTER_ALGO_AUTO;
MPIR_Alltoall_intra_algo_t MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_AUTO;
MPIR_Alltoall_inter_algo_t MPIR_Alltoall_inter_algo_choice = MPIR_ALLTOALL_INTER_ALGO_AUTO;
MPIR_Alltoallv_intra_algo_t MPIR_Alltoallv_intra_algo_choice = MPIR_ALLTOALLV_INTRA_ALGO_AUTO;
MPIR_Alltoallv_inter_algo_t MPIR_Alltoallv_inter_algo_choice = MPIR_ALLTOALLV_INTER_ALGO_AUTO;
MPIR_Alltoallw_intra_algo_t MPIR_Alltoallw_intra_algo_choice = MPIR_ALLTOALLW_INTRA_ALGO_AUTO;
MPIR_Alltoallw_inter_algo_t MPIR_Alltoallw_inter_algo_choice = MPIR_ALLTOALLW_INTER_ALGO_AUTO;
MPIR_Barrier_intra_algo_t MPIR_Barrier_intra_algo_choice = MPIR_BARRIER_INTRA_ALGO_AUTO;
MPIR_Barrier_inter_algo_t MPIR_Barrier_inter_algo_choice = MPIR_BARRIER_INTER_ALGO_AUTO;
MPIR_Bcast_intra_algo_t MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_AUTO;
MPIR_Bcast_inter_algo_t MPIR_Bcast_inter_algo_choice = MPIR_BCAST_INTER_ALGO_AUTO;
MPIR_Exscan_intra_algo_t MPIR_Exscan_intra_algo_choice = MPIR_EXSCAN_INTRA_ALGO_AUTO;
MPIR_Gather_intra_algo_t MPIR_Gather_intra_algo_choice = MPIR_GATHER_INTRA_ALGO_AUTO;
MPIR_Gather_inter_algo_t MPIR_Gather_inter_algo_choice = MPIR_GATHER_INTER_ALGO_AUTO;
MPIR_Gatherv_intra_algo_t MPIR_Gatherv_intra_algo_choice = MPIR_GATHERV_INTRA_ALGO_AUTO;
MPIR_Gatherv_inter_algo_t MPIR_Gatherv_inter_algo_choice = MPIR_GATHERV_INTER_ALGO_AUTO;
MPIR_Iallgather_intra_algo_t MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_AUTO;
MPIR_Iallgather_inter_algo_t MPIR_Iallgather_inter_algo_choice = MPIR_IALLGATHER_INTER_ALGO_AUTO;
MPIR_Iallgatherv_intra_algo_t MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_AUTO;
MPIR_Iallgatherv_inter_algo_t MPIR_Iallgatherv_inter_algo_choice = MPIR_IALLGATHERV_INTER_ALGO_AUTO;
MPIR_Iallreduce_intra_algo_t MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_AUTO;
MPIR_Iallreduce_inter_algo_t MPIR_Iallreduce_inter_algo_choice = MPIR_IALLREDUCE_INTER_ALGO_AUTO;
MPIR_Ialltoall_intra_algo_t MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_AUTO;
MPIR_Ialltoall_inter_algo_t MPIR_Ialltoall_inter_algo_choice = MPIR_IALLTOALL_INTER_ALGO_AUTO;
MPIR_Ialltoallv_intra_algo_t MPIR_Ialltoallv_intra_algo_choice = MPIR_IALLTOALLV_INTRA_ALGO_AUTO;
MPIR_Ialltoallv_inter_algo_t MPIR_Ialltoallv_inter_algo_choice = MPIR_IALLTOALLV_INTER_ALGO_AUTO;
MPIR_Ialltoallw_intra_algo_t MPIR_Ialltoallw_intra_algo_choice = MPIR_IALLTOALLW_INTRA_ALGO_AUTO;
MPIR_Ialltoallw_inter_algo_t MPIR_Ialltoallw_inter_algo_choice = MPIR_IALLTOALLW_INTER_ALGO_AUTO;
MPIR_Ibarrier_intra_algo_t MPIR_Ibarrier_intra_algo_choice = MPIR_IBARRIER_INTRA_ALGO_AUTO;
MPIR_Ibarrier_inter_algo_t MPIR_Ibarrier_inter_algo_choice = MPIR_IBARRIER_INTER_ALGO_AUTO;
MPIR_Ibcast_intra_algo_t MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_AUTO;
MPIR_Tree_type_t MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Ibcast_inter_algo_t MPIR_Ibcast_inter_algo_choice = MPIR_IBCAST_INTER_ALGO_AUTO;
MPIR_Iexscan_intra_algo_t MPIR_Iexscan_intra_algo_choice = MPIR_IEXSCAN_INTRA_ALGO_AUTO;
MPIR_Igather_intra_algo_t MPIR_Igather_intra_algo_choice = MPIR_IGATHER_INTRA_ALGO_AUTO;
MPIR_Igather_inter_algo_t MPIR_Igather_inter_algo_choice = MPIR_IGATHER_INTER_ALGO_AUTO;
MPIR_Igatherv_intra_algo_t MPIR_Igatherv_intra_algo_choice = MPIR_IGATHERV_INTRA_ALGO_AUTO;
MPIR_Igatherv_inter_algo_t MPIR_Igatherv_inter_algo_choice = MPIR_IGATHERV_INTER_ALGO_AUTO;
MPIR_Ineighbor_allgather_intra_algo_t MPIR_Ineighbor_allgather_intra_algo_choice =
    MPIR_INEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO;
MPIR_Ineighbor_allgather_inter_algo_t MPIR_Ineighbor_allgather_inter_algo_choice =
    MPIR_INEIGHBOR_ALLGATHER_INTER_ALGO_AUTO;
MPIR_Ineighbor_allgatherv_intra_algo_t MPIR_Ineighbor_allgatherv_intra_algo_choice =
    MPIR_INEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO;
MPIR_Ineighbor_allgatherv_inter_algo_t MPIR_Ineighbor_allgatherv_inter_algo_choice =
    MPIR_INEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO;
MPIR_Ineighbor_alltoall_intra_algo_t MPIR_Ineighbor_alltoall_intra_algo_choice =
    MPIR_INEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO;
MPIR_Ineighbor_alltoall_inter_algo_t MPIR_Ineighbor_alltoall_inter_algo_choice =
    MPIR_INEIGHBOR_ALLTOALL_INTER_ALGO_AUTO;
MPIR_Ineighbor_alltoallv_intra_algo_t MPIR_Ineighbor_alltoallv_intra_algo_choice =
    MPIR_INEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO;
MPIR_Ineighbor_alltoallv_inter_algo_t MPIR_Ineighbor_alltoallv_inter_algo_choice =
    MPIR_INEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO;
MPIR_Ineighbor_alltoallw_intra_algo_t MPIR_Ineighbor_alltoallw_intra_algo_choice =
    MPIR_INEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO;
MPIR_Ineighbor_alltoallw_inter_algo_t MPIR_Ineighbor_alltoallw_inter_algo_choice =
    MPIR_INEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO;
MPIR_Ireduce_scatter_intra_algo_t MPIR_Ireduce_scatter_intra_algo_choice =
    MPIR_IREDUCE_SCATTER_INTRA_ALGO_AUTO;
MPIR_Tree_type_t MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;
MPIR_Ireduce_scatter_inter_algo_t MPIR_Ireduce_scatter_inter_algo_choice =
    MPIR_IREDUCE_SCATTER_INTER_ALGO_AUTO;
MPIR_Ireduce_scatter_block_intra_algo_t MPIR_Ireduce_scatter_block_intra_algo_choice =
    MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO;
MPIR_Ireduce_scatter_block_inter_algo_t MPIR_Ireduce_scatter_block_inter_algo_choice =
    MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO;
MPIR_Ireduce_intra_algo_t MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_AUTO;
MPIR_Ireduce_inter_algo_t MPIR_Ireduce_inter_algo_choice = MPIR_IREDUCE_INTER_ALGO_AUTO;
MPIR_Iscan_intra_algo_t MPIR_Iscan_intra_algo_choice = MPIR_ISCAN_INTRA_ALGO_AUTO;
MPIR_Iscatter_intra_algo_t MPIR_Iscatter_intra_algo_choice = MPIR_ISCATTER_INTRA_ALGO_AUTO;
MPIR_Iscatter_inter_algo_t MPIR_Iscatter_inter_algo_choice = MPIR_ISCATTER_INTER_ALGO_AUTO;
MPIR_Iscatterv_intra_algo_t MPIR_Iscatterv_intra_algo_choice = MPIR_ISCATTERV_INTRA_ALGO_AUTO;
MPIR_Iscatterv_inter_algo_t MPIR_Iscatterv_inter_algo_choice = MPIR_ISCATTERV_INTER_ALGO_AUTO;
MPIR_Neighbor_allgather_intra_algo_t MPIR_Neighbor_allgather_intra_algo_choice =
    MPIR_NEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO;
MPIR_Neighbor_allgather_inter_algo_t MPIR_Neighbor_allgather_inter_algo_choice =
    MPIR_NEIGHBOR_ALLGATHER_INTER_ALGO_AUTO;
MPIR_Neighbor_allgatherv_intra_algo_t MPIR_Neighbor_allgatherv_intra_algo_choice =
    MPIR_NEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO;
MPIR_Neighbor_allgatherv_inter_algo_t MPIR_Neighbor_allgatherv_inter_algo_choice =
    MPIR_NEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO;
MPIR_Neighbor_alltoall_intra_algo_t MPIR_Neighbor_alltoall_intra_algo_choice =
    MPIR_NEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO;
MPIR_Neighbor_alltoall_inter_algo_t MPIR_Neighbor_alltoall_inter_algo_choice =
    MPIR_NEIGHBOR_ALLTOALL_INTER_ALGO_AUTO;
MPIR_Neighbor_alltoallv_intra_algo_t MPIR_Neighbor_alltoallv_intra_algo_choice =
    MPIR_NEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO;
MPIR_Neighbor_alltoallv_inter_algo_t MPIR_Neighbor_alltoallv_inter_algo_choice =
    MPIR_NEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO;
MPIR_Neighbor_alltoallw_intra_algo_t MPIR_Neighbor_alltoallw_intra_algo_choice =
    MPIR_NEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO;
MPIR_Neighbor_alltoallw_inter_algo_t MPIR_Neighbor_alltoallw_inter_algo_choice =
    MPIR_NEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO;
MPIR_Reduce_scatter_intra_algo_t MPIR_Reduce_scatter_intra_algo_choice =
    MPIR_REDUCE_SCATTER_INTRA_ALGO_AUTO;
MPIR_Reduce_scatter_inter_algo_t MPIR_Reduce_scatter_inter_algo_choice =
    MPIR_REDUCE_SCATTER_INTER_ALGO_AUTO;
MPIR_Reduce_scatter_block_intra_algo_t MPIR_Reduce_scatter_block_intra_algo_choice =
    MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO;
MPIR_Reduce_scatter_block_inter_algo_t MPIR_Reduce_scatter_block_inter_algo_choice =
    MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO;
MPIR_Reduce_intra_algo_t MPIR_Reduce_intra_algo_choice = MPIR_REDUCE_INTRA_ALGO_AUTO;
MPIR_Reduce_inter_algo_t MPIR_Reduce_inter_algo_choice = MPIR_REDUCE_INTER_ALGO_AUTO;
MPIR_Scan_intra_algo_t MPIR_Scan_intra_algo_choice = MPIR_SCAN_INTRA_ALGO_AUTO;
MPIR_Scatterv_intra_algo_t MPIR_Scatterv_intra_algo_choice = MPIR_SCATTERV_INTRA_ALGO_AUTO;
MPIR_Scatterv_inter_algo_t MPIR_Scatterv_inter_algo_choice = MPIR_SCATTERV_INTER_ALGO_AUTO;
MPIR_Scatter_intra_algo_t MPIR_Scatter_intra_algo_choice = MPIR_SCATTER_INTRA_ALGO_AUTO;
MPIR_Scatter_inter_algo_t MPIR_Scatter_inter_algo_choice = MPIR_SCATTER_INTER_ALGO_AUTO;

#undef FUNCNAME
#define FUNCNAME MPII_Coll_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Coll_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* =========================================================================== */
    /* | Check if the user manually selected any collective algorithms via CVARs | */
    /* =========================================================================== */

    /* Allgather Intra */
    if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM, "brucks"))
        MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM, "nb"))
        MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTRA_ALGORITHM, "ring"))
        MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_RING;
    else
        MPIR_Allgather_intra_algo_choice = MPIR_ALLGATHER_INTRA_ALGO_AUTO;

    /* Allgather Inter */
    if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTER_ALGORITHM, "local_gather_remote_bcast"))
        MPIR_Allgather_inter_algo_choice = MPIR_ALLGATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_BCAST;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHER_INTER_ALGORITHM, "nb"))
        MPIR_Allgather_inter_algo_choice = MPIR_ALLGATHER_INTER_ALGO_NB;
    else
        MPIR_Allgather_inter_algo_choice = MPIR_ALLGATHER_INTER_ALGO_AUTO;

    /* Allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM, "brucks"))
        MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM, "nb"))
        MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTRA_ALGORITHM, "ring"))
        MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_RING;
    else
        MPIR_Allgatherv_intra_algo_choice = MPIR_ALLGATHERV_INTRA_ALGO_AUTO;

    /* Allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM, "nb"))
        MPIR_Allgatherv_inter_algo_choice = MPIR_ALLGATHERV_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLGATHERV_INTER_ALGORITHM, "remote_gather_local_bcast"))
        MPIR_Allgatherv_inter_algo_choice = MPIR_ALLGATHERV_INTER_ALGO_REMOTE_GATHER_LOCAL_BCAST;
    else
        MPIR_Allgatherv_inter_algo_choice = MPIR_ALLGATHERV_INTER_ALGO_AUTO;

    /* Allreduce Intra */
    if (0 == strcmp(MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM, "nb"))
        MPIR_Allreduce_intra_algo_choice = MPIR_ALLREDUCE_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Allreduce_intra_algo_choice = MPIR_ALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM, "reduce_scatter_allgather"))
        MPIR_Allreduce_intra_algo_choice = MPIR_ALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER;
    else
        MPIR_Allreduce_intra_algo_choice = MPIR_ALLREDUCE_INTRA_ALGO_AUTO;

    /* Allreduce Inter */
    if (0 == strcmp(MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM, "nb"))
        MPIR_Allreduce_inter_algo_choice = MPIR_ALLREDUCE_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM, "reduce_exchange_bcast"))
        MPIR_Allreduce_inter_algo_choice = MPIR_ALLREDUCE_INTER_ALGO_REDUCE_EXCHANGE_BCAST;
    else
        MPIR_Allreduce_inter_algo_choice = MPIR_ALLREDUCE_INTER_ALGO_AUTO;

    /* Alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM, "brucks"))
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM, "nb"))
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM, "pairwise"))
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM, "pairwise_sendrecv_replace"))
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTRA_ALGORITHM, "scattered"))
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_SCATTERED;
    else
        MPIR_Alltoall_intra_algo_choice = MPIR_ALLTOALL_INTRA_ALGO_AUTO;

    /* Alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Alltoall_inter_algo_choice = MPIR_ALLTOALL_INTER_ALGO_PAIRWISE_EXCHANGE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALL_INTER_ALGORITHM, "nb"))
        MPIR_Alltoall_inter_algo_choice = MPIR_ALLTOALL_INTER_ALGO_NB;
    else
        MPIR_Alltoall_inter_algo_choice = MPIR_ALLTOALL_INTER_ALGO_AUTO;

    /* Alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM, "nb"))
        MPIR_Alltoallv_intra_algo_choice = MPIR_ALLTOALLV_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM, "pairwise_sendrecv_replace"))
        MPIR_Alltoallv_intra_algo_choice = MPIR_ALLTOALLV_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLV_INTRA_ALGORITHM, "scattered"))
        MPIR_Alltoallv_intra_algo_choice = MPIR_ALLTOALLV_INTRA_ALGO_SCATTERED;
    else
        MPIR_Alltoallv_intra_algo_choice = MPIR_ALLTOALLV_INTRA_ALGO_AUTO;

    /* Alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM, "nb"))
        MPIR_Alltoallv_inter_algo_choice = MPIR_ALLTOALLV_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLV_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Alltoallv_inter_algo_choice = MPIR_ALLTOALLV_INTER_ALGO_PAIRWISE_EXCHANGE;
    else
        MPIR_Alltoallv_inter_algo_choice = MPIR_ALLTOALLV_INTER_ALGO_AUTO;

    /* Alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM, "nb"))
        MPIR_Alltoallw_intra_algo_choice = MPIR_ALLTOALLW_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM, "pairwise_sendrecv_replace"))
        MPIR_Alltoallw_intra_algo_choice = MPIR_ALLTOALLW_INTRA_ALGO_PAIRWISE_SENDRECV_REPLACE;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLW_INTRA_ALGORITHM, "scattered"))
        MPIR_Alltoallw_intra_algo_choice = MPIR_ALLTOALLW_INTRA_ALGO_SCATTERED;
    else
        MPIR_Alltoallw_intra_algo_choice = MPIR_ALLTOALLW_INTRA_ALGO_AUTO;

    /* Alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM, "nb"))
        MPIR_Alltoallw_inter_algo_choice = MPIR_ALLTOALLW_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_ALLTOALLW_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Alltoallw_inter_algo_choice = MPIR_ALLTOALLW_INTER_ALGO_PAIRWISE_EXCHANGE;
    else
        MPIR_Alltoallw_inter_algo_choice = MPIR_ALLTOALLW_INTER_ALGO_AUTO;

    /* Barrier Intra */
    if (0 == strcmp(MPIR_CVAR_BARRIER_INTRA_ALGORITHM, "nb"))
        MPIR_Barrier_intra_algo_choice = MPIR_BARRIER_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_BARRIER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Barrier_intra_algo_choice = MPIR_BARRIER_INTRA_ALGO_RECURSIVE_DOUBLING;
    else
        MPIR_Barrier_intra_algo_choice = MPIR_BARRIER_INTRA_ALGO_AUTO;

    /* Barrier Inter */
    if (0 == strcmp(MPIR_CVAR_BARRIER_INTER_ALGORITHM, "bcast"))
        MPIR_Barrier_inter_algo_choice = MPIR_BARRIER_INTER_ALGO_BCAST;
    else if (0 == strcmp(MPIR_CVAR_BARRIER_INTER_ALGORITHM, "nb"))
        MPIR_Barrier_inter_algo_choice = MPIR_BARRIER_INTER_ALGO_NB;
    else
        MPIR_Barrier_inter_algo_choice = MPIR_BARRIER_INTER_ALGO_AUTO;

    /* Bcast Intra */
    if (0 == strcmp(MPIR_CVAR_BCAST_INTRA_ALGORITHM, "binomial"))
        MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTRA_ALGORITHM, "nb"))
        MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTRA_ALGORITHM, "scatter_recursive_doubling_allgather"))
        MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_SCATTER_RECURSIVE_DOUBLING_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTRA_ALGORITHM, "scatter_ring_allgather"))
        MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_SCATTER_RING_ALLGATHER;
    else
        MPIR_Bcast_intra_algo_choice = MPIR_BCAST_INTRA_ALGO_AUTO;

    /* Bcast Inter */
    if (0 == strcmp(MPIR_CVAR_BCAST_INTER_ALGORITHM, "nb"))
        MPIR_Bcast_inter_algo_choice = MPIR_BCAST_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTER_ALGORITHM, "remote_send_local_bcast"))
        MPIR_Bcast_inter_algo_choice = MPIR_BCAST_INTER_ALGO_REMOTE_SEND_LOCAL_BCAST;
    else
        MPIR_Bcast_inter_algo_choice = MPIR_BCAST_INTER_ALGO_AUTO;

    /* Exscan Intra */
    if (0 == strcmp(MPIR_CVAR_EXSCAN_INTRA_ALGORITHM, "nb"))
        MPIR_Exscan_intra_algo_choice = MPIR_EXSCAN_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_EXSCAN_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Exscan_intra_algo_choice = MPIR_EXSCAN_INTRA_ALGO_RECURSIVE_DOUBLING;
    else
        MPIR_Exscan_intra_algo_choice = MPIR_EXSCAN_INTRA_ALGO_AUTO;

    /* Gather Intra */
    if (0 == strcmp(MPIR_CVAR_GATHER_INTRA_ALGORITHM, "binomial"))
        MPIR_Gather_intra_algo_choice = MPIR_GATHER_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_GATHER_INTRA_ALGORITHM, "nb"))
        MPIR_Gather_intra_algo_choice = MPIR_GATHER_INTRA_ALGO_NB;
    else
        MPIR_Gather_intra_algo_choice = MPIR_GATHER_INTRA_ALGO_AUTO;

    /* Gather Inter */
    if (0 == strcmp(MPIR_CVAR_GATHER_INTER_ALGORITHM, "linear"))
        MPIR_Gather_inter_algo_choice = MPIR_GATHER_INTER_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_GATHER_INTER_ALGORITHM, "local_gather_remote_send"))
        MPIR_Gather_inter_algo_choice = MPIR_GATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_SEND;
    else if (0 == strcmp(MPIR_CVAR_GATHER_INTER_ALGORITHM, "nb"))
        MPIR_Gather_inter_algo_choice = MPIR_GATHER_INTER_ALGO_NB;
    else
        MPIR_Gather_inter_algo_choice = MPIR_GATHER_INTER_ALGO_AUTO;

    /* Gatherv Intra */
    if (0 == strcmp(MPIR_CVAR_GATHERV_INTRA_ALGORITHM, "linear"))
        MPIR_Gatherv_intra_algo_choice = MPIR_GATHERV_INTRA_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_GATHERV_INTRA_ALGORITHM, "nb"))
        MPIR_Gatherv_intra_algo_choice = MPIR_GATHERV_INTRA_ALGO_NB;
    else
        MPIR_Gatherv_intra_algo_choice = MPIR_GATHERV_INTRA_ALGO_AUTO;

    /* Gatherv Inter */
    if (0 == strcmp(MPIR_CVAR_GATHERV_INTER_ALGORITHM, "linear"))
        MPIR_Gatherv_inter_algo_choice = MPIR_GATHERV_INTER_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_GATHERV_INTER_ALGORITHM, "nb"))
        MPIR_Gatherv_inter_algo_choice = MPIR_GATHERV_INTER_ALGO_NB;
    else
        MPIR_Gatherv_inter_algo_choice = MPIR_GATHERV_INTER_ALGO_AUTO;

    /* Iallgather Intra */
    if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "brucks"))
        MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "ring"))
        MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_RING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "recexch_distance_doubling"))
        MPIR_Iallgather_intra_algo_choice =
            MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "recexch_distance_halving"))
        MPIR_Iallgather_intra_algo_choice =
            MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_HALVING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTRA_ALGORITHM, "gentran_brucks"))
        MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_GENTRAN_BRUCKS;
    else
        MPIR_Iallgather_intra_algo_choice = MPIR_IALLGATHER_INTRA_ALGO_AUTO;

    /* Iallgather Inter */
    if (0 == strcmp(MPIR_CVAR_IALLGATHER_INTER_ALGORITHM, "local_gather_remote_bcast"))
        MPIR_Iallgather_inter_algo_choice = MPIR_IALLGATHER_INTER_ALGO_LOCAL_GATHER_REMOTE_BCAST;
    else
        MPIR_Iallgather_inter_algo_choice = MPIR_IALLGATHER_INTER_ALGO_AUTO;

    /* Iallgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "brucks"))
        MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "ring"))
        MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_RING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "recexch_distance_doubling"))
        MPIR_Iallgatherv_intra_algo_choice =
            MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "recexch_distance_halving"))
        MPIR_Iallgatherv_intra_algo_choice =
            MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RECEXCH_DISTANCE_HALVING;
    else if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM, "gentran_ring"))
        MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_GENTRAN_RING;
    else
        MPIR_Iallgatherv_intra_algo_choice = MPIR_IALLGATHERV_INTRA_ALGO_AUTO;

    /* Iallgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_IALLGATHERV_INTER_ALGORITHM, "remote_gather_local_bcast"))
        MPIR_Iallgatherv_inter_algo_choice = MPIR_IALLGATHERV_INTER_ALGO_REMOTE_GATHER_LOCAL_BCAST;
    else
        MPIR_Iallgatherv_inter_algo_choice = MPIR_IALLGATHERV_INTER_ALGO_AUTO;

    /* Iallreduce Intra */
    if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "naive"))
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_NAIVE;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "reduce_scatter_allgather"))
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_REDUCE_SCATTER_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "recexch_single_buffer"))
        MPIR_Iallreduce_intra_algo_choice =
            MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_SINGLE_BUFFER;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "recexch_multiple_buffer"))
        MPIR_Iallreduce_intra_algo_choice =
            MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_RECEXCH_MULTIPLE_BUFFER;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "tree_kary"))
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTRA_ALGORITHM, "tree_knomial"))
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_GENTRAN_TREE_KNOMIAL;
    else
        MPIR_Iallreduce_intra_algo_choice = MPIR_IALLREDUCE_INTRA_ALGO_AUTO;

    /* Iallreduce Inter */
    if (0 == strcmp(MPIR_CVAR_IALLREDUCE_INTER_ALGORITHM, "remote_reduce_local_bcast"))
        MPIR_Iallreduce_inter_algo_choice = MPIR_IALLREDUCE_INTER_ALGO_REMOTE_REDUCE_LOCAL_BCAST;
    else
        MPIR_Iallreduce_inter_algo_choice = MPIR_IALLREDUCE_INTER_ALGO_AUTO;

    /* Ialltoall Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM, "brucks"))
        MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_BRUCKS;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM, "inplace"))
        MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_INPLACE;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM, "pairwise"))
        MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALL_INTRA_ALGORITHM, "permuted_sendrecv"))
        MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_PERMUTED_SENDRECV;
    else
        MPIR_Ialltoall_intra_algo_choice = MPIR_IALLTOALL_INTRA_ALGO_AUTO;

    /* Ialltoall Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALL_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Ialltoall_inter_algo_choice = MPIR_IALLTOALL_INTER_ALGO_PAIRWISE_EXCHANGE;
    else
        MPIR_Ialltoall_inter_algo_choice = MPIR_IALLTOALL_INTER_ALGO_AUTO;

    /* Ialltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM, "blocked"))
        MPIR_Ialltoallv_intra_algo_choice = MPIR_IALLTOALLV_INTRA_ALGO_BLOCKED;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALLV_INTRA_ALGORITHM, "inplace"))
        MPIR_Ialltoallv_intra_algo_choice = MPIR_IALLTOALLV_INTRA_ALGO_INPLACE;
    else
        MPIR_Ialltoallv_intra_algo_choice = MPIR_IALLTOALLV_INTRA_ALGO_AUTO;

    /* Ialltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLV_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Ialltoallv_inter_algo_choice = MPIR_IALLTOALLV_INTER_ALGO_PAIRWISE_EXCHANGE;
    else
        MPIR_Ialltoallv_inter_algo_choice = MPIR_IALLTOALLV_INTER_ALGO_AUTO;

    /* Ialltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM, "blocked"))
        MPIR_Ialltoallw_intra_algo_choice = MPIR_IALLTOALLW_INTRA_ALGO_BLOCKED;
    else if (0 == strcmp(MPIR_CVAR_IALLTOALLW_INTRA_ALGORITHM, "inplace"))
        MPIR_Ialltoallw_intra_algo_choice = MPIR_IALLTOALLW_INTRA_ALGO_INPLACE;
    else
        MPIR_Ialltoallw_intra_algo_choice = MPIR_IALLTOALLW_INTRA_ALGO_AUTO;

    /* Ialltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_IALLTOALLW_INTER_ALGORITHM, "pairwise_exchange"))
        MPIR_Ialltoallw_inter_algo_choice = MPIR_IALLTOALLW_INTER_ALGO_PAIRWISE_EXCHANGE;
    else
        MPIR_Ialltoallw_inter_algo_choice = MPIR_IALLTOALLW_INTER_ALGO_AUTO;

    /* Ibarrier Intra */
    if (0 == strcmp(MPIR_CVAR_IBARRIER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Ibarrier_intra_algo_choice = MPIR_IBARRIER_INTRA_ALGO_RECURSIVE_DOUBLING;
    if (0 == strcmp(MPIR_CVAR_IBARRIER_INTRA_ALGORITHM, "recexch"))
        MPIR_Ibarrier_intra_algo_choice = MPIR_IBARRIER_INTRA_ALGO_GENTRAN_RECEXCH;
    else
        MPIR_Ibarrier_intra_algo_choice = MPIR_IBARRIER_INTRA_ALGO_AUTO;

    /* Ibarrier Inter */
    if (0 == strcmp(MPIR_CVAR_IBARRIER_INTER_ALGORITHM, "bcast"))
        MPIR_Ibarrier_inter_algo_choice = MPIR_IBARRIER_INTER_ALGO_BCAST;
    else
        MPIR_Ibarrier_inter_algo_choice = MPIR_IBARRIER_INTER_ALGO_AUTO;

    /* Ibcast */
    if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "kary"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_1"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_TREE_TYPE, "knomial_2"))
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Ibcast_tree_type = MPIR_TREE_TYPE_KARY;

    /* Ibcast Intra */
    if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "binomial"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "scatter_recursive_doubling_allgather"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_SCATTER_RECURSIVE_DOUBLING_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "scatter_ring_allgather"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_SCATTER_RING_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "tree"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_GENTRAN_TREE;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "scatter_recexch_allgather"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_GENTRAN_SCATTER_RECEXCH_ALLGATHER;
    else if (0 == strcmp(MPIR_CVAR_IBCAST_INTRA_ALGORITHM, "ring"))
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_GENTRAN_RING;
    else
        MPIR_Ibcast_intra_algo_choice = MPIR_IBCAST_INTRA_ALGO_AUTO;

    /* Ibcast Inter */
    if (0 == strcmp(MPIR_CVAR_IBCAST_INTER_ALGORITHM, "flat"))
        MPIR_Ibcast_inter_algo_choice = MPIR_IBCAST_INTER_ALGO_FLAT;
    else
        MPIR_Ibcast_inter_algo_choice = MPIR_IBCAST_INTER_ALGO_AUTO;

    /* Iexscan Intra */
    if (0 == strcmp(MPIR_CVAR_IEXSCAN_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Iexscan_intra_algo_choice = MPIR_IEXSCAN_INTRA_ALGO_RECURSIVE_DOUBLING;
    else
        MPIR_Iexscan_intra_algo_choice = MPIR_IEXSCAN_INTRA_ALGO_AUTO;

    /* Igather Intra */
    if (0 == strcmp(MPIR_CVAR_IGATHER_INTRA_ALGORITHM, "binomial"))
        MPIR_Igather_intra_algo_choice = MPIR_IGATHER_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_IGATHER_INTRA_ALGORITHM, "tree"))
        MPIR_Igather_intra_algo_choice = MPIR_IGATHER_INTRA_ALGO_TREE;
    else
        MPIR_Igather_intra_algo_choice = MPIR_IGATHER_INTRA_ALGO_AUTO;

    /* Igather Inter */
    if (0 == strcmp(MPIR_CVAR_IGATHER_INTER_ALGORITHM, "long_inter"))
        MPIR_Igather_inter_algo_choice = MPIR_IGATHER_INTER_ALGO_LONG;
    else if (0 == strcmp(MPIR_CVAR_IGATHER_INTER_ALGORITHM, "short_inter"))
        MPIR_Igather_inter_algo_choice = MPIR_IGATHER_INTER_ALGO_SHORT;
    else
        MPIR_Igather_inter_algo_choice = MPIR_IGATHER_INTER_ALGO_AUTO;

    /* Igatherv Intra */
    if (0 == strcmp(MPIR_CVAR_IGATHERV_INTRA_ALGORITHM, "linear"))
        MPIR_Igatherv_intra_algo_choice = MPIR_IGATHERV_INTRA_ALGO_LINEAR;
    else
        MPIR_Igatherv_intra_algo_choice = MPIR_IGATHERV_INTRA_ALGO_AUTO;

    /* Igatherv Inter */
    if (0 == strcmp(MPIR_CVAR_IGATHERV_INTER_ALGORITHM, "linear"))
        MPIR_Igatherv_inter_algo_choice = MPIR_IGATHERV_INTER_ALGO_LINEAR;
    else
        MPIR_Igatherv_inter_algo_choice = MPIR_IGATHERV_INTER_ALGO_AUTO;

    /* Ineighbor_allgather Intra */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLGATHER_INTRA_ALGORITHM, "linear"))
        MPIR_Ineighbor_allgather_intra_algo_choice = MPIR_INEIGHBOR_ALLGATHER_INTRA_ALGO_LINEAR;
    else
        MPIR_Ineighbor_allgather_intra_algo_choice = MPIR_INEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO;

    /* Ineighbor_allgather Inter */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLGATHER_INTER_ALGORITHM, "linear"))
        MPIR_Ineighbor_allgather_inter_algo_choice = MPIR_INEIGHBOR_ALLGATHER_INTER_ALGO_LINEAR;
    else
        MPIR_Ineighbor_allgather_inter_algo_choice = MPIR_INEIGHBOR_ALLGATHER_INTER_ALGO_AUTO;

    /* Ineighbor_allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTRA_ALGORITHM, "linear"))
        MPIR_Ineighbor_allgatherv_intra_algo_choice = MPIR_INEIGHBOR_ALLGATHERV_INTRA_ALGO_LINEAR;
    else
        MPIR_Ineighbor_allgatherv_intra_algo_choice = MPIR_INEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO;

    /* Ineighbor_allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLGATHERV_INTER_ALGORITHM, "linear"))
        MPIR_Ineighbor_allgatherv_inter_algo_choice = MPIR_INEIGHBOR_ALLGATHERV_INTER_ALGO_LINEAR;
    else
        MPIR_Ineighbor_allgatherv_inter_algo_choice = MPIR_INEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO;

    /* Ineighbor_alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALL_INTRA_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoall_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALL_INTRA_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoall_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO;

    /* Ineighbor_alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALL_INTER_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoall_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALL_INTER_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoall_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALL_INTER_ALGO_AUTO;

    /* Ineighbor_alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTRA_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoallv_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALLV_INTRA_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoallv_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO;

    /* Ineighbor_alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALLV_INTER_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoallv_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALLV_INTER_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoallv_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO;

    /* Ineighbor_alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTRA_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoallw_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALLW_INTRA_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoallw_intra_algo_choice = MPIR_INEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO;

    /* Ineighbor_alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_INEIGHBOR_ALLTOALLW_INTER_ALGORITHM, "linear"))
        MPIR_Ineighbor_alltoallw_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALLW_INTER_ALGO_LINEAR;
    else
        MPIR_Ineighbor_alltoallw_inter_algo_choice = MPIR_INEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO;

    /* Ireduce_scatter Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM, "noncommutative"))
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_NONCOMMUTATIVE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM, "pairwise"))
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM, "recursive_halving"))
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_RECURSIVE_HALVING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTRA_ALGORITHM, "recexch"))
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_GENTRAN_RECEXCH;
    else
        MPIR_Ireduce_scatter_intra_algo_choice = MPIR_IREDUCE_SCATTER_INTRA_ALGO_AUTO;

    /* Ireduce_scatter Inter */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_INTER_ALGORITHM, "remote_reduce_local_scatterv"))
        MPIR_Ireduce_scatter_inter_algo_choice =
            MPIR_IREDUCE_SCATTER_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTERV;
    else
        MPIR_Ireduce_scatter_inter_algo_choice = MPIR_IREDUCE_SCATTER_INTER_ALGO_AUTO;

    /* Ireduce_scatter_block Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "noncommutative"))
        MPIR_Ireduce_scatter_block_intra_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_NONCOMMUTATIVE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "pairwise"))
        MPIR_Ireduce_scatter_block_intra_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Ireduce_scatter_block_intra_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "recursive_halving"))
        MPIR_Ireduce_scatter_block_intra_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_HALVING;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "recexch"))
        MPIR_Ireduce_scatter_block_intra_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_GENTRAN_RECEXCH;
    else
        MPIR_Ireduce_scatter_block_intra_algo_choice = MPIR_IREDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO;

    /* Ireduce_scatter_block Inter */
    if (0 ==
        strcmp(MPIR_CVAR_IREDUCE_SCATTER_BLOCK_INTER_ALGORITHM, "remote_reduce_local_scatterv"))
        MPIR_Ireduce_scatter_block_inter_algo_choice =
            MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTERV;
    else
        MPIR_Ireduce_scatter_block_inter_algo_choice = MPIR_IREDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO;

    /* Ireduce */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "kary"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "knomial_1"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_TREE_TYPE, "knomial_2"))
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KNOMIAL_2;
    else
        MPIR_Ireduce_tree_type = MPIR_TREE_TYPE_KARY;

    /* Ireduce Intra */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_INTRA_ALGORITHM, "binomial"))
        MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_INTRA_ALGORITHM, "reduce_scatter_gather"))
        MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_REDUCE_SCATTER_GATHER;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_INTRA_ALGORITHM, "tree"))
        MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_GENTRAN_TREE;
    else if (0 == strcmp(MPIR_CVAR_IREDUCE_INTRA_ALGORITHM, "ring"))
        MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_GENTRAN_RING;
    else
        MPIR_Ireduce_intra_algo_choice = MPIR_IREDUCE_INTRA_ALGO_AUTO;

    /* Ireduce Inter */
    if (0 == strcmp(MPIR_CVAR_IREDUCE_INTER_ALGORITHM, "local_reduce_remote_send"))
        MPIR_Ireduce_inter_algo_choice = MPIR_IREDUCE_INTER_ALGO_LOCAL_REDUCE_REMOTE_SEND;
    else
        MPIR_Ireduce_inter_algo_choice = MPIR_IREDUCE_INTER_ALGO_AUTO;

    /* Iscan Intra */
    if (0 == strcmp(MPIR_CVAR_ISCAN_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Iscan_intra_algo_choice = MPIR_ISCAN_INTRA_ALGO_RECURSIVE_DOUBLING;
    else
        MPIR_Iscan_intra_algo_choice = MPIR_ISCAN_INTRA_ALGO_AUTO;

    /* Iscatter Intra */
    if (0 == strcmp(MPIR_CVAR_ISCATTER_INTRA_ALGORITHM, "binomial"))
        MPIR_Iscatter_intra_algo_choice = MPIR_ISCATTER_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_ISCATTER_INTRA_ALGORITHM, "tree"))
        MPIR_Iscatter_intra_algo_choice = MPIR_ISCATTER_INTRA_ALGO_TREE;
    else
        MPIR_Iscatter_intra_algo_choice = MPIR_ISCATTER_INTRA_ALGO_AUTO;

    /* Iscatter Inter */
    if (0 == strcmp(MPIR_CVAR_ISCATTER_INTER_ALGORITHM, "linear"))
        MPIR_Iscatter_inter_algo_choice = MPIR_ISCATTER_INTER_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_ISCATTER_INTER_ALGORITHM, "remote_send_local_scatter"))
        MPIR_Iscatter_inter_algo_choice = MPIR_ISCATTER_INTER_ALGO_REMOTE_SEND_LOCAL_SCATTER;
    else
        MPIR_Iscatter_inter_algo_choice = MPIR_ISCATTER_INTER_ALGO_AUTO;

    /* Iscatterv Intra */
    if (0 == strcmp(MPIR_CVAR_ISCATTERV_INTRA_ALGORITHM, "linear"))
        MPIR_Iscatterv_intra_algo_choice = MPIR_ISCATTERV_INTRA_ALGO_LINEAR;
    else
        MPIR_Iscatterv_intra_algo_choice = MPIR_ISCATTERV_INTRA_ALGO_AUTO;

    /* Iscatterv Inter */
    if (0 == strcmp(MPIR_CVAR_ISCATTERV_INTER_ALGORITHM, "linear"))
        MPIR_Iscatterv_inter_algo_choice = MPIR_ISCATTERV_INTER_ALGO_LINEAR;
    else
        MPIR_Iscatterv_inter_algo_choice = MPIR_ISCATTERV_INTER_ALGO_AUTO;

    /* Neighbor_allgather Intra */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLGATHER_INTRA_ALGORITHM, "nb"))
        MPIR_Neighbor_allgather_intra_algo_choice = MPIR_NEIGHBOR_ALLGATHER_INTRA_ALGO_NB;
    else
        MPIR_Neighbor_allgather_intra_algo_choice = MPIR_NEIGHBOR_ALLGATHER_INTRA_ALGO_AUTO;

    /* Neighbor_allgather Inter */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLGATHER_INTER_ALGORITHM, "nb"))
        MPIR_Neighbor_allgather_inter_algo_choice = MPIR_NEIGHBOR_ALLGATHER_INTER_ALGO_NB;
    else
        MPIR_Neighbor_allgather_inter_algo_choice = MPIR_NEIGHBOR_ALLGATHER_INTER_ALGO_AUTO;

    /* Neighbor_allgatherv Intra */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTRA_ALGORITHM, "nb"))
        MPIR_Neighbor_allgatherv_intra_algo_choice = MPIR_NEIGHBOR_ALLGATHERV_INTRA_ALGO_NB;
    else
        MPIR_Neighbor_allgatherv_intra_algo_choice = MPIR_NEIGHBOR_ALLGATHERV_INTRA_ALGO_AUTO;

    /* Neighbor_allgatherv Inter */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLGATHERV_INTER_ALGORITHM, "nb"))
        MPIR_Neighbor_allgatherv_inter_algo_choice = MPIR_NEIGHBOR_ALLGATHERV_INTER_ALGO_NB;
    else
        MPIR_Neighbor_allgatherv_inter_algo_choice = MPIR_NEIGHBOR_ALLGATHERV_INTER_ALGO_AUTO;

    /* Neighbor_alltoall Intra */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALL_INTRA_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoall_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALL_INTRA_ALGO_NB;
    else
        MPIR_Neighbor_alltoall_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALL_INTRA_ALGO_AUTO;

    /* Neighbor_alltoall Inter */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALL_INTER_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoall_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALL_INTER_ALGO_NB;
    else
        MPIR_Neighbor_alltoall_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALL_INTER_ALGO_AUTO;

    /* Neighbor_alltoallv Intra */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTRA_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoallv_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALLV_INTRA_ALGO_NB;
    else
        MPIR_Neighbor_alltoallv_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALLV_INTRA_ALGO_AUTO;

    /* Neighbor_alltoallv Inter */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALLV_INTER_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoallv_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALLV_INTER_ALGO_NB;
    else
        MPIR_Neighbor_alltoallv_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALLV_INTER_ALGO_AUTO;

    /* Neighbor_alltoallw Intra */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTRA_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoallw_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALLW_INTRA_ALGO_NB;
    else
        MPIR_Neighbor_alltoallw_intra_algo_choice = MPIR_NEIGHBOR_ALLTOALLW_INTRA_ALGO_AUTO;

    /* Neighbor_alltoallw Inter */
    if (0 == strcmp(MPIR_CVAR_NEIGHBOR_ALLTOALLW_INTER_ALGORITHM, "nb"))
        MPIR_Neighbor_alltoallw_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALLW_INTER_ALGO_NB;
    else
        MPIR_Neighbor_alltoallw_inter_algo_choice = MPIR_NEIGHBOR_ALLTOALLW_INTER_ALGO_AUTO;

    /* Red_scat Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM, "nb"))
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM, "noncommutative"))
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_NONCOMMUTATIVE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM, "pairwise"))
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTRA_ALGORITHM, "recursive_halving"))
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_RECURSIVE_HALVING;
    else
        MPIR_Reduce_scatter_intra_algo_choice = MPIR_REDUCE_SCATTER_INTRA_ALGO_AUTO;

    /* Red_scat Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTER_ALGORITHM, "nb"))
        MPIR_Reduce_scatter_inter_algo_choice = MPIR_REDUCE_SCATTER_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_INTER_ALGORITHM, "remote_reduce_local_scatter"))
        MPIR_Reduce_scatter_inter_algo_choice =
            MPIR_REDUCE_SCATTER_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTER;
    else
        MPIR_Reduce_scatter_inter_algo_choice = MPIR_REDUCE_SCATTER_INTER_ALGO_AUTO;

    /* Red_scat_block Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "nb"))
        MPIR_Reduce_scatter_block_intra_algo_choice = MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "noncommutative"))
        MPIR_Reduce_scatter_block_intra_algo_choice =
            MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_NONCOMMUTATIVE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "pairwise"))
        MPIR_Reduce_scatter_block_intra_algo_choice = MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_PAIRWISE;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Reduce_scatter_block_intra_algo_choice =
            MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_DOUBLING;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTRA_ALGORITHM, "recursive_halving"))
        MPIR_Reduce_scatter_block_intra_algo_choice =
            MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_RECURSIVE_HALVING;
    else
        MPIR_Reduce_scatter_block_intra_algo_choice = MPIR_REDUCE_SCATTER_BLOCK_INTRA_ALGO_AUTO;

    /* Red_scat_block Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM, "nb"))
        MPIR_Reduce_scatter_block_inter_algo_choice = MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_NB;
    else if (0 ==
             strcmp(MPIR_CVAR_REDUCE_SCATTER_BLOCK_INTER_ALGORITHM, "remote_reduce_local_scatter"))
        MPIR_Reduce_scatter_block_inter_algo_choice =
            MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_REMOTE_REDUCE_LOCAL_SCATTER;
    else
        MPIR_Reduce_scatter_block_inter_algo_choice = MPIR_REDUCE_SCATTER_BLOCK_INTER_ALGO_AUTO;

    /* Reduce Intra */
    if (0 == strcmp(MPIR_CVAR_REDUCE_INTRA_ALGORITHM, "binomial"))
        MPIR_Reduce_intra_algo_choice = MPIR_REDUCE_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_INTRA_ALGORITHM, "nb"))
        MPIR_Reduce_intra_algo_choice = MPIR_REDUCE_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_INTRA_ALGORITHM, "reduce_scatter_gather"))
        MPIR_Reduce_intra_algo_choice = MPIR_REDUCE_INTRA_ALGO_REDUCE_SCATTER_GATHER;
    else
        MPIR_Reduce_intra_algo_choice = MPIR_REDUCE_INTRA_ALGO_AUTO;

    /* Reduce Inter */
    if (0 == strcmp(MPIR_CVAR_REDUCE_INTER_ALGORITHM, "local_reduce_remote_send"))
        MPIR_Reduce_inter_algo_choice = MPIR_REDUCE_INTER_ALGO_LOCAL_REDUCE_REMOTE_SEND;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_INTER_ALGORITHM, "nb"))
        MPIR_Reduce_inter_algo_choice = MPIR_REDUCE_INTER_ALGO_NB;
    else
        MPIR_Reduce_inter_algo_choice = MPIR_REDUCE_INTER_ALGO_AUTO;

    /* Scan Intra */
    if (0 == strcmp(MPIR_CVAR_SCAN_INTRA_ALGORITHM, "nb"))
        MPIR_Scan_intra_algo_choice = MPIR_SCAN_INTRA_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_SCAN_INTRA_ALGORITHM, "recursive_doubling"))
        MPIR_Scan_intra_algo_choice = MPIR_SCAN_INTRA_ALGO_RECURSIVE_DOUBLING;
    else
        MPIR_Scan_intra_algo_choice = MPIR_SCAN_INTRA_ALGO_AUTO;

    /* Scatter Intra */
    if (0 == strcmp(MPIR_CVAR_SCATTER_INTRA_ALGORITHM, "binomial"))
        MPIR_Scatter_intra_algo_choice = MPIR_SCATTER_INTRA_ALGO_BINOMIAL;
    else if (0 == strcmp(MPIR_CVAR_SCATTER_INTRA_ALGORITHM, "nb"))
        MPIR_Scatter_intra_algo_choice = MPIR_SCATTER_INTRA_ALGO_NB;
    else
        MPIR_Scatter_intra_algo_choice = MPIR_SCATTER_INTRA_ALGO_AUTO;

    /* Scatter Inter */
    if (0 == strcmp(MPIR_CVAR_SCATTER_INTRA_ALGORITHM, "linear"))
        MPIR_Scatter_inter_algo_choice = MPIR_SCATTER_INTER_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_SCATTER_INTRA_ALGORITHM, "nb"))
        MPIR_Scatter_inter_algo_choice = MPIR_SCATTER_INTER_ALGO_NB;
    else if (0 == strcmp(MPIR_CVAR_SCATTER_INTRA_ALGORITHM, "remote_send_local_scatter"))
        MPIR_Scatter_inter_algo_choice = MPIR_SCATTER_INTER_ALGO_REMOTE_SEND_LOCAL_SCATTER;
    else
        MPIR_Scatter_inter_algo_choice = MPIR_SCATTER_INTER_ALGO_AUTO;

    /* Scatterv Intra */
    if (0 == strcmp(MPIR_CVAR_SCATTERV_INTRA_ALGORITHM, "linear"))
        MPIR_Scatterv_intra_algo_choice = MPIR_SCATTERV_INTRA_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_SCATTERV_INTRA_ALGORITHM, "nb"))
        MPIR_Scatterv_intra_algo_choice = MPIR_SCATTERV_INTRA_ALGO_NB;
    else
        MPIR_Scatterv_intra_algo_choice = MPIR_SCATTERV_INTRA_ALGO_AUTO;

    /* Scatterv Inter */
    if (0 == strcmp(MPIR_CVAR_SCATTERV_INTER_ALGORITHM, "linear"))
        MPIR_Scatterv_inter_algo_choice = MPIR_SCATTERV_INTER_ALGO_LINEAR;
    else if (0 == strcmp(MPIR_CVAR_SCATTERV_INTER_ALGORITHM, "nb"))
        MPIR_Scatterv_inter_algo_choice = MPIR_SCATTERV_INTER_ALGO_NB;
    else
        MPIR_Scatterv_inter_algo_choice = MPIR_SCATTERV_INTER_ALGO_AUTO;

    /* register non blocking collectives progress hook */
    mpi_errno = MPID_Progress_register_hook(MPIDU_Sched_progress, &MPIR_Nbc_progress_hook_id);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* initialize transports */
    mpi_errno = MPII_Stubtran_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Gentran_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* initialize algorithms */
    mpi_errno = MPII_Stubalgo_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Treealgo_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Recexchalgo_init();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPII_Coll_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Coll_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    /* deregister non blocking collectives progress hook */
    MPID_Progress_deregister_hook(MPIR_Nbc_progress_hook_id);

    mpi_errno = MPII_Gentran_finalize();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Stubtran_finalize();
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function used by CH3 progress engine to decide whether to
 * block for a recv operation */
int MPIR_Coll_safe_to_block()
{
    return MPII_Gentran_scheds_are_pending() == false;
}

/* Function to initialze communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    /* initialize any stub algo related data structures */
    mpi_errno = MPII_Stubalgo_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    /* initialize any tree algo related data structures */
    mpi_errno = MPII_Treealgo_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* initialize any transport data structures */
    mpi_errno = MPII_Stubtran_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Gentran_comm_init(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    /* cleanup all collective communicators */
    mpi_errno = MPII_Stubalgo_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Treealgo_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* cleanup transport data */
    mpi_errno = MPII_Stubtran_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPII_Gentran_comm_cleanup(comm);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
