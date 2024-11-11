/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* TODO: move this to commutil.c */
static void MPIR_Comm_construct_internode_roots_group(MPIR_Comm * comm, int root,
                                                      int *group_p, int *root_rank_p)
{
    int inter_size = comm->num_external;
    int inter_rank = comm->internode_table[comm->rank];

    int inter_group, *proc_table;
    MPIR_COMM_PUSH_SUBGROUP(comm, inter_size, inter_rank, inter_group, proc_table);

    for (int i = 0; i < inter_size; i++) {
        proc_table[i] = -1;
    }
    for (int i = 0; i < comm->local_size; i++) {
        int r = comm->internode_table[i];
        if (proc_table[r] == -1) {
            proc_table[r] = i;
        }
    }
    int inter_root_rank = comm->internode_table[root];
    proc_table[inter_root_rank] = root;

    comm->subgroups[inter_group].proc_table = proc_table;

    *group_p = inter_group;
    *root_rank_p = inter_root_rank;
}

/* FIXME This function uses some heuristsics based off of some testing on a
 * cluster at Argonne.  We need a better system for detrmining and controlling
 * the cutoff points for these algorithms.  If I've done this right, you should
 * be able to make changes along these lines almost exclusively in this function
 * and some new functions. [goodell@ 2008/01/07] */
int MPIR_Bcast_intra_smp(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                         MPIR_Comm * comm_ptr, int coll_group, MPIR_Errflag_t errflag)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;

#ifdef HAVE_ERROR_CHECKING
    MPIR_Assert(MPIR_Comm_is_parent_comm(comm_ptr, coll_group));
#endif
    int comm_size = comm_ptr->local_size;

    int node_group = 0, node_roots_group = 0;
    int local_rank, local_size, local_root, inter_root = -1;

    node_group = MPIR_SUBGROUP_NODE;
#define NODEGROUP(field) comm_ptr->subgroups[node_group].field

    local_rank = NODEGROUP(rank);
    local_size = NODEGROUP(size);
    local_root = MPIR_Get_intranode_rank(comm_ptr, root);
    if (local_root < 0) {
        /* non-root node use local rank 0 as local root */
        local_root = 0;
    }
    if (local_rank == local_root) {
        MPIR_Comm_construct_internode_roots_group(comm_ptr, root, &node_roots_group, &inter_root);
        MPIR_Assert(node_roots_group > 0);
    }

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (comm_size < MPIR_CVAR_BCAST_MIN_PROCS) ||
        (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(comm_size))) {
        /* local roots perform the internode broadcast */
        if (local_rank == local_root) {
            mpi_errno = MPIR_Bcast(buffer, count, datatype, inter_root,
                                   comm_ptr, node_roots_group, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* perform the intranode broadcast */
        if (local_size > 1) {
            mpi_errno = MPIR_Bcast(buffer, count, datatype, 0, comm_ptr, node_group, errflag);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        /* FIXME It would be good to have an SMP-aware version of this
         * algorithm that (at least approximately) minimized internode
         * communication. */
        mpi_errno = MPIR_Bcast_intra_scatter_ring_allgather(buffer, count, datatype, root, comm_ptr,
                                                            coll_group, errflag);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    if (node_roots_group) {
        MPIR_COMM_POP_SUBGROUP(comm_ptr);
    }
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
