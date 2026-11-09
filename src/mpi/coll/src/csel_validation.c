/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Checks of a loaded tuning file. They improve the error checking by refusing a file that would
 * fail or hang at run time, but are not critical to run a correct profile: a correct file selects
 * the same way without them. */

#include "mpiimpl.h"
#include "coll_csel.h"

/* See coll_algorithms.txt for the rules of the conditions and Collective_Selection.md for the
 * restriction sets. */

/* a condition is reached only by the collectives that define it; seen marks (subtree, collective)
 * pairs */
static int validate_defined_conditions(MPIR_Csel_node_s * node, int tree_idx, int coll_type,
                                       bool * seen);
/* a restriction set that is not local, and the free subtrees it calls: no local condition, an
 * algorithm restricted on a _coll restriction only if the set's mask has it, and each
 * dispatch(<mask>) leaves out one of the set's restrictions; seen marks subtrees */
static int validate_set(MPIR_Csel_node_s * node, int tree_idx, unsigned set_mask, bool * seen);
/* a local set's leaves are an algorithm restricted on one of its local restrictions or a
 * dispatch(<mask>) that leaves out local restrictions only, one of its own among them, and it
 * splits only on conditions that are the same on every rank */
static int validate_local_set(MPIR_Csel_node_s * node, int tree_idx, unsigned local_bits);
static bool is_local_node(MPIR_Csel_node_s * node);
/* a restriction set's mask and variants have only restrictions that its collective defines, and
 * a _coll and a _local restriction exclude each other */
static int validate_set_name(int tree_idx, int coll_type);

/* Check a loaded tuning file, walking the restriction sets and the subtrees they call. */
int MPII_Csel_validate(void)
{
    int mpi_errno = MPI_SUCCESS;
    int n = MPII_Csel_num_trees();
    int ntypes = MPIR_CSEL_NUM_COLL_TYPES + 1;
    bool *seen = MPL_calloc((size_t) n * ntypes, sizeof(bool), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!seen, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* Pass 0: the restrictions in the names of the restriction sets */
    for (int coll_type = 0; coll_type < MPIR_CSEL_NUM_COLL_TYPES; coll_type++) {
        for (int i = 0; i < n; i++) {
            if (MPII_Csel_tree_is_coll_type(i, coll_type)) {
                mpi_errno = validate_set_name(i, coll_type);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    /* Pass 1: a condition is reached only by the collectives that define it. A set's collective is
     * in its name. */
    for (int coll_type = 0; coll_type < MPIR_CSEL_NUM_COLL_TYPES; coll_type++) {
        for (int i = 0; i < n; i++) {
            if (MPII_Csel_tree_is_coll_type(i, coll_type) && !seen[i * ntypes + coll_type + 1]) {
                seen[i * ntypes + coll_type + 1] = true;
                mpi_errno = validate_defined_conditions(MPII_Csel_get_tree_by_idx(i), i,
                                                        coll_type, seen);
                MPIR_ERR_CHECK(mpi_errno);
            }
        }
    }

    /* Pass 2: the sets that are not local, and the subtrees they call, have no local condition,
     * since the dispatch has already taken the local sets, and each dispatch(<mask>) leaves out a
     * restriction of the set, so that it cannot come back to it */
    for (int i = 0; i < n; i++) {
        if (MPII_Csel_tree_is_reserved(i) && !MPII_Csel_tree_is_local(i)) {
            memset(seen, 0, (size_t) n * sizeof(bool));
            seen[i] = true;
            mpi_errno = validate_set(MPII_Csel_get_tree_by_idx(i), i, MPII_Csel_tree_mask(i), seen);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* Pass 3: the local sets */
    for (int i = 0; i < n; i++) {
        if (MPII_Csel_tree_is_local(i)) {
            mpi_errno = validate_local_set(MPII_Csel_get_tree_by_idx(i), i,
                                           MPII_Csel_tree_mask(i) & MPIR_CSEL_LOCAL_RESTRICTIONS);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_fail:
    MPL_free(seen);
    return mpi_errno;
}

static bool condition_is_defined(int node_type, MPIR_Csel_coll_type_e coll_type, const char **name);

/* fail with **colljsonset, naming the subtree and what is wrong */
#define SET_ERROR(tree_idx_, what_)                                         \
    do {                                                                    \
        /* unused when MPIR_ERR_SETANDJUMP2 drops its message arguments */  \
        MPL_UNUSED const char *name_ = MPII_Csel_tree_name(tree_idx_);      \
        MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**colljsonset",     \
                             "**colljsonset %s %s", name_, what_);          \
    } while (0)

static int validate_set_name(int tree_idx, int coll_type)
{
    int mpi_errno = MPI_SUCCESS;
    unsigned mask = MPII_Csel_tree_mask(tree_idx);
    unsigned variant_mask = MPII_Csel_tree_variant_mask(tree_idx);
    unsigned bits = mask | variant_mask;
    const char *cond_name;

    if (mask >> MPIR_CSEL_NUM_RESTRICTIONS) {
        SET_ERROR(tree_idx, "has a mask bit that is not a restriction");
    }
    for (int k = 0; k < MPIR_CSEL_NUM_RESTRICTIONS; k++) {
        if ((bits & (1u << k)) && !condition_is_defined(k, coll_type, &cond_name)) {
            /* unused when MPIR_ERR_SETANDJUMP3 drops its message arguments */
            MPL_UNUSED const char *name = MPII_Csel_tree_name(tree_idx);
            MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljsoncond",
                                 "**colljsoncond %s %s %s", name, cond_name,
                                 MPIR_Coll_type_names[coll_type]);
        }
    }
    if ((bits & MPIR_CSEL_COLL_RESTRICTIONS) && (bits & MPIR_CSEL_LOCAL_RESTRICTIONS)) {
        SET_ERROR(tree_idx, "has a _coll and a _local restriction, which exclude each other");
    }
    if (mask & variant_mask) {
        SET_ERROR(tree_idx, "has a restriction both in its mask and as its variant");
    }

  fn_fail:
    return mpi_errno;
}

static int validate_defined_conditions(MPIR_Csel_node_s * node, int tree_idx, int coll_type,
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
            case CSEL_NODE_TYPE__OPERATOR__DISPATCH:
                /* the sets are checked with their own collectives */
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                next = node->u.call.tree_idx;
                break;
            case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
                mpi_errno = validate_defined_conditions(node->success, tree_idx,
                                                        node->u.collective.coll_type, seen);
                MPIR_ERR_CHECK(mpi_errno);
                continue;
            case CSEL_NODE_TYPE__OPERATOR__ANY:
                break;
            default:
                {
                    const char *cond_name;
                    if (coll_type >= 0 && !condition_is_defined(node->type, coll_type, &cond_name)) {
                        /* unused when MPIR_ERR_SETANDJUMP3 drops its message arguments */
                        MPL_UNUSED const char *name = MPII_Csel_tree_name(tree_idx);
                        MPIR_ERR_SETANDJUMP3(mpi_errno, MPI_ERR_OTHER, "**colljsoncond",
                                             "**colljsoncond %s %s %s", name, cond_name,
                                             MPIR_Coll_type_names[coll_type]);
                    }
                    break;
                }
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
        mpi_errno = validate_defined_conditions(node->success, tree_idx, coll_type, seen);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_fail:
    return mpi_errno;
}

static int validate_set(MPIR_Csel_node_s * node, int tree_idx, unsigned set_mask, bool * seen)
{
    int mpi_errno = MPI_SUCCESS;

    for (; node; node = node->failure) {
        int next = -1;
        switch (node->type) {
            case CSEL_NODE_TYPE__CONTAINER:
                if (MPIR_Coll_get_algo_restriction_bits(node->u.container->id) &
                    MPIR_CSEL_COLL_RESTRICTIONS & ~set_mask) {
                    SET_ERROR(tree_idx,
                              "is reached from a restriction set and selects an "
                              "algorithm restricted on a _coll restriction that "
                              "the set's mask does not have");
                }
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__ERROR:
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__DISPATCH:
                if (!(node->u.dispatch.drop_mask & set_mask)) {
                    SET_ERROR(tree_idx,
                              "is reached from a restriction set and has a dispatch(<mask>) that "
                              "leaves out none of the set's restrictions, so it comes back to it");
                }
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                next = node->u.call.tree_idx;
                break;
            default:
                if (is_local_node(node)) {
                    /* unused when MPIR_ERR_SETANDJUMP2 drops its message arguments */
                    MPL_UNUSED const char *name = MPII_Csel_tree_name(tree_idx);
                    MPIR_ERR_SETANDJUMP2(mpi_errno, MPI_ERR_OTHER, "**colljsonlocal",
                                         "**colljsonlocal %s %s", name,
                                         MPIR_Csel_condition_names[node->type]);
                }
                mpi_errno = validate_set(node->success, tree_idx, set_mask, seen);
                MPIR_ERR_CHECK(mpi_errno);
                continue;
        }
        if (!seen[next]) {
            seen[next] = true;
            mpi_errno = validate_set(MPII_Csel_get_tree_by_idx(next), next, set_mask, seen);
            MPIR_ERR_CHECK(mpi_errno);
        }
        return MPI_SUCCESS;
    }

  fn_fail:
    return mpi_errno;
}

static int validate_local_set(MPIR_Csel_node_s * node, int tree_idx, unsigned local_bits)
{
    int mpi_errno = MPI_SUCCESS;

    for (; node; node = node->failure) {
        switch (node->type) {
            case CSEL_NODE_TYPE__CONTAINER:
                if (!(MPIR_Coll_get_algo_restriction_bits(node->u.container->id) & local_bits)) {
                    SET_ERROR(tree_idx, "is a local set and selects an algorithm that is not "
                              "restricted on its local restriction");
                }
                return MPI_SUCCESS;
            case CSEL_NODE_TYPE__OPERATOR__DISPATCH:{
                    unsigned drop_mask = node->u.dispatch.drop_mask;
                    if (!(drop_mask & local_bits) || (drop_mask & ~MPIR_CSEL_LOCAL_RESTRICTIONS)) {
                        SET_ERROR(tree_idx,
                                  "is a local set; its dispatch(<mask>) must leave out its "
                                  "local restriction, and only local ones, to go where the other "
                                  "ranks go");
                    }
                    return MPI_SUCCESS;
                }
            case CSEL_NODE_TYPE__ERROR:
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                SET_ERROR(tree_idx, "is a local set; its leaves can only be dispatch(<mask>) or an "
                          "algorithm restricted on its local restriction");
                break;
            default:
                if (is_local_node(node)) {
                    SET_ERROR(tree_idx, "is a local set and tests a local condition");
                }
                mpi_errno = validate_local_set(node->success, tree_idx, local_bits);
                MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_fail:
    return mpi_errno;
}

static bool has_msg_size(MPIR_Csel_coll_type_e coll_type);
static bool has_sendbuf_inplace(MPIR_Csel_coll_type_e coll_type);
static bool has_op(MPIR_Csel_coll_type_e coll_type);
static bool has_gpu_coll(MPIR_Csel_coll_type_e coll_type);

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
        case CSEL_NODE_TYPE__OPERATOR__gpu_coll:
            *name = "gpu_coll";
            return has_gpu_coll(coll_type);
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

/* Whether gpu_coll is the same on every rank of coll_type under MPIR_CVAR_ENABLE_GPU_COLL. A rank
 * of these three can have no buffer to query (a zero count) or skip the query (negative
 * displacements), so it cannot tell. */
static bool has_gpu_coll(MPIR_Csel_coll_type_e coll_type)
{
    switch (coll_type) {
          COLL_TYPE_ALL_CASE(GATHERV):
          COLL_TYPE_ALL_CASE(SCATTERV):
          COLL_TYPE_ALL_CASE(ALLTOALLV):
            return false;
        default:
            return true;
    }
}

static bool is_local_node(MPIR_Csel_node_s * node)
{
    return (int) node->type < 32 && (MPIR_CSEL_LOCAL_RESTRICTIONS & (1u << node->type));
}
