/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Checks of a loaded tuning file. They improve the error checking by refusing a file that would
 * fail or hang at run time, but are not critical to run a correct profile: a correct file selects
 * the same way without them. */

#include "mpiimpl.h"
#include "coll_csel.h"

/* See coll_algorithms.txt for the rules of the conditions. */

/* a condition is reached only by the collectives that define it; seen marks (subtree, collective)
 * pairs */
static int validate_defined_conditions(MPIR_Csel_node_s * node, int tree, int coll_type,
                                       bool * seen);

/* Check a loaded tuning file, walking the trees that it can reach. */
int MPII_Csel_validate(void)
{
    int mpi_errno = MPI_SUCCESS;
    int main_idx = MPII_Csel_name_to_idx("main");
    int n = MPII_Csel_num_trees();
    bool *seen = MPL_calloc((size_t) n * (MPIR_CSEL_NUM_COLL_TYPES + 1), sizeof(bool),
                            MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!seen, mpi_errno, MPI_ERR_OTHER, "**nomem");

    seen[main_idx * (MPIR_CSEL_NUM_COLL_TYPES + 1)] = true;
    mpi_errno =
        validate_defined_conditions(MPII_Csel_get_tree_by_idx(main_idx), main_idx, -1, seen);

  fn_fail:
    MPL_free(seen);
    return mpi_errno;
}

static bool condition_is_defined(int node_type, MPIR_Csel_coll_type_e coll_type, const char **name);

static int validate_defined_conditions(MPIR_Csel_node_s * node, int tree, int coll_type,
                                       bool * seen)
{
    int mpi_errno = MPI_SUCCESS;
    int ntypes = MPIR_CSEL_NUM_COLL_TYPES + 1;  /* coll_type -1: no collective= node yet */

    for (; node; node = node->failure) {
        int next = -1;
        switch (node->type) {
            case CSEL_NODE_TYPE__CONTAINER:
            case CSEL_NODE_TYPE__ERROR:
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                next = node->u.call.idx;
                break;
            case CSEL_NODE_TYPE__OPERATOR__DISPATCH:
                for (int c = 0; c < (1 << node->u.dispatch->nbits); c++) {
                    int idx = node->u.dispatch->idx[c];
                    if (!seen[idx * ntypes + coll_type + 1]) {
                        seen[idx * ntypes + coll_type + 1] = true;
                        mpi_errno = validate_defined_conditions(MPII_Csel_get_tree_by_idx(idx), idx,
                                                                coll_type, seen);
                        MPIR_ERR_CHECK(mpi_errno);
                    }
                }
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
                mpi_errno = validate_defined_conditions(node->success, tree,
                                                        node->u.collective.coll_type, seen);
                MPIR_ERR_CHECK(mpi_errno);
                continue;
            case CSEL_NODE_TYPE__OPERATOR__ANY:
                break;
            default:
                const char *cond_name;
                if (coll_type >= 0 && !condition_is_defined(node->type, coll_type, &cond_name)) {
                    /* unused when MPIR_ERR_SETANDJUMP3 drops its message arguments */
                    MPL_UNUSED const char *name = MPII_Csel_tree_name(tree);
                    MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljsoncond",
                                         "**colljsoncond %s %s %s", name, cond_name,
                                         MPIR_Coll_type_names[coll_type]);
                }
                break;
        }
        if (next >= 0) {
            if (!seen[next * ntypes + coll_type + 1]) {
                seen[next * ntypes + coll_type + 1] = true;
                mpi_errno = validate_defined_conditions(MPII_Csel_get_tree_by_idx(next), next,
                                                        coll_type, seen);
                MPIR_ERR_CHECK(mpi_errno);
            }
            return MPI_SUCCESS;
        }
        mpi_errno = validate_defined_conditions(node->success, tree, coll_type, seen);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_fail:
    return mpi_errno;
}

static bool has_msg_size(MPIR_Csel_coll_type_e coll_type);
static bool has_sendbuf_inplace(MPIR_Csel_coll_type_e coll_type);
static bool has_op(MPIR_Csel_coll_type_e coll_type);

/* Whether the condition of a node type is defined for coll_type, i.e. its checker has a case for
 * it. Only the conditions that some collective lacks are listed; all others are defined for
 * every collective. name is set to the condition's name. */
static bool condition_is_defined(int node_type, MPIR_Csel_coll_type_e coll_type, const char **name)
{
    switch (node_type) {
        case CSEL_NODE_TYPE__OPERATOR__msg_size:
            *name = "msg_size";
            return has_msg_size(coll_type);
        case CSEL_NODE_TYPE__OPERATOR__inplace:
            *name = "inplace";
            return has_sendbuf_inplace(coll_type);
        case CSEL_NODE_TYPE__OPERATOR__builtin_op:
            *name = "builtin_op";
            return has_op(coll_type);
        case CSEL_NODE_TYPE__OPERATOR__commutative:
            *name = "commutative";
            return has_op(coll_type);
        default:
            *name = "?";
            return true;
    }
}

/* Whether coll_type has a msg_size. Must match the cases of MPIR_Csel_msg_size in coll_csel.h. */
static bool has_msg_size(MPIR_Csel_coll_type_e coll_type)
{
    switch (coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
          COLL_TYPE_ALL_CASE(BCAST):
          COLL_TYPE_ALL_CASE(REDUCE):
          COLL_TYPE_ALL_CASE(ALLTOALL):
          COLL_TYPE_ALL_CASE(ALLGATHER):
          COLL_TYPE_INTRA_CASE(ALLGATHERV):
          COLL_TYPE_ALL_CASE(GATHER):
          COLL_TYPE_ALL_CASE(SCATTER):
          COLL_TYPE_INTRA_CASE(REDUCE_SCATTER):
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
          COLL_TYPE_ALL_CASE(SCAN):
          COLL_TYPE_ALL_CASE(EXSCAN):
            return true;
        default:
            return false;
    }
}

/* Whether coll_type has an MPI_IN_PLACE sendbuf that is the same on every rank. Must match the
 * cases of MPIR_Csel_sendbuf_inplace in coll_csel.h. The rooted collectives (gather, scatter,
 * reduce) take MPI_IN_PLACE at the root only, so they have none. */
static bool has_sendbuf_inplace(MPIR_Csel_coll_type_e coll_type)
{
    switch (coll_type) {
          COLL_TYPE_ALL_CASE(ALLGATHER):
          COLL_TYPE_ALL_CASE(ALLGATHERV):
          COLL_TYPE_ALL_CASE(ALLTOALL):
          COLL_TYPE_ALL_CASE(ALLTOALLV):
          COLL_TYPE_ALL_CASE(ALLTOALLW):
          COLL_TYPE_ALL_CASE(ALLREDUCE):
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER):
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
          COLL_TYPE_ALL_CASE(SCAN):
          COLL_TYPE_ALL_CASE(EXSCAN):
            return true;
        default:
            return false;
    }
}

/* Whether coll_type has a reduction op. Must match the cases of MPIR_Csel_op_is_builtin and
 * MPIR_Csel_op_is_commutative in coll_csel.h. */
static bool has_op(MPIR_Csel_coll_type_e coll_type)
{
    switch (coll_type) {
          COLL_TYPE_ALL_CASE(ALLREDUCE):
          COLL_TYPE_ALL_CASE(REDUCE):
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER):
          COLL_TYPE_ALL_CASE(REDUCE_SCATTER_BLOCK):
          COLL_TYPE_ALL_CASE(SCAN):
          COLL_TYPE_ALL_CASE(EXSCAN):
            return true;
        default:
            return false;
    }
}
