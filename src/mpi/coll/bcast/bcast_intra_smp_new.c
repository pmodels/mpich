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

    MPIR_COMM_NEW_SUBGROUP(comm, MPIR_SUBGROUP_TEMP, inter_size, inter_rank);
    int inter_group = MPIR_COMM_LAST_SUBGROUP(comm);

    int *proc_table = MPL_malloc(inter_size * sizeof(int), MPL_MEM_OTHER);
    for (int i = 0; i < inter_size; i++) {
        proc_table[i] = -1;
    }
    for (int i = 0; i < comm->remote_size; i++) {
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

/* The sticky point in the old smp bcast is when root is not a local root, resulting
 * an extra send/recv. With the new MPIR_Subgroup, we don't have to, in principle.
 */
int MPIR_Bcast_intra_smp_new(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                             MPIR_Comm * comm_ptr, int coll_attr)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPI_Aint type_size, nbytes = 0;
    int node_group = -1, inter_group = -1;

    MPIR_Assert(comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__PARENT ||
                comm_ptr->hierarchy_kind == MPIR_COMM_HIERARCHY_KIND__FLAT);
    MPIR_Assert(MPIR_COLL_ATTR_GET_SUBGROUP(coll_attr) == 0);

    for (int i = 0; i < comm_ptr->num_subgroups; i++) {
        if (comm_ptr->subgroups[i].kind == MPIR_SUBGROUP_NODE) {
            node_group = i;
            break;
        }
    }
    MPIR_Assert(node_group > 0);

#define NODEGROUP(field) comm_ptr->subgroups[node_group].field
    int local_rank, local_size;
    local_rank = NODEGROUP(rank);
    local_size = NODEGROUP(size);

    int local_root_rank = MPIR_Get_intranode_rank(comm_ptr, root);

    int local_root = 0;
    if (local_root_rank > 0) {
        local_root = local_root_rank;
    }

    /* Construct an internode group */
    int inter_root_rank = -1;
#define INTERGROUP(field) comm_ptr->subgroups[inter_group].field
    if (local_rank == local_root) {
        MPIR_Comm_construct_internode_roots_group(comm_ptr, root, &inter_group, &inter_root_rank);
        MPIR_Assert(inter_group > 0);
    }

    int node_attr = coll_attr | MPIR_COLL_ATTR_SUBGROUP(node_group);
    int inter_attr = coll_attr | MPIR_COLL_ATTR_SUBGROUP(inter_group);

    MPIR_Datatype_get_size_macro(datatype, type_size);

    nbytes = type_size * count;
    if (nbytes == 0)
        goto fn_exit;   /* nothing to do */

    if ((nbytes < MPIR_CVAR_BCAST_SHORT_MSG_SIZE) || (local_size < MPIR_CVAR_BCAST_MIN_PROCS) ||
        (nbytes < MPIR_CVAR_BCAST_LONG_MSG_SIZE && MPL_is_pof2(local_size))) {
        /* perform the internode broadcast */
        if (local_rank == local_root) {
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, inter_root_rank,
                                                  comm_ptr, inter_attr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
        }

        /* perform the intranode broadcast */
        if (local_size > 1) {
            mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, local_root,
                                                  comm_ptr, node_attr);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
        }
    } else {
        /* large msg or non-pof2 */
        /* FIXME: better algorithm selection */
        mpi_errno = MPIR_Bcast_intra_binomial(buffer, count, datatype, root, comm_ptr, coll_attr);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, coll_attr, mpi_errno_ret);
    }

  fn_exit:
    if (inter_group > 0) {
        MPIR_COMM_POP_SUBGROUP(comm_ptr);
    }
    return mpi_errno_ret;
}
