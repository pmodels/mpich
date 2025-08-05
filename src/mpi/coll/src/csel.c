/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpl.h"
#include "mpir_csel.h"
#include "csel_internal.h"
#include <fcntl.h>      /* open */
#include <sys/mman.h>   /* mmap */
#include <sys/stat.h>
#include <json.h>

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLLECTIVE_SELECTION_REPORT
      category    : COLLECTIVE
      type        : enum
      default     : none
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : |-
        Variable to select report type.
        none    - No print out
        summary - Print a summary of each algorithm
        tree    - Print the collective selection tree
        all     - Print both tree and summary

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

static void validate_tree(csel_node_s * node)
{
    static int coll = -1;

    /* if we reached a leaf node, we are done */
    if (node->type == CSEL_NODE_TYPE__CONTAINER)
        return;

    /* if we see the collective type, store it */
    if (node->type == CSEL_NODE_TYPE__OPERATOR__COLLECTIVE)
        coll = node->u.collective.coll_type;

    /* success path should never be NULL */
    if (node->success == NULL) {
        fprintf(stderr, "unexpected NULL success path for coll %d\n", coll);
        MPIR_Assert(0);
    } else {
        validate_tree(node->success);
    }

    if (node->type == CSEL_NODE_TYPE__OPERATOR__ANY) {
        /* for "ANY"-style operators, the failure path must be NULL */
        if (node->failure) {
            fprintf(stderr, "unexpected non-NULL failure path for coll %d\n", coll);
            MPIR_Assert(0);
        }
    } else if (node->type != CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED &&
               node->type != CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA &&
               node->type != CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER &&
               node->type != CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY &&
               node->type != CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE &&
               node->type != CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE &&
               node->type != CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR &&
               node->type != CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE &&
               node->type != CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN &&
               node->type != CSEL_NODE_TYPE__OPERATOR__COLLECTIVE) {
        /* for boolean types, the failure path might or might not be
         * NULL, but for everything else, the failure path must not be
         * NULL */
        if (node->failure == NULL) {
            fprintf(stderr, "unexpected NULL failure path for coll %d\n", coll);
            MPIR_Assert(0);
        }
    }

    if (node->success)
        validate_tree(node->success);
    if (node->failure)
        validate_tree(node->failure);
}

static bool key_is_any(const char *ckey)
{
    int len = strlen(ckey);

    if (strcmp(ckey, "any") == 0 || strcmp(ckey + len - 4, "=any") == 0) {
        return true;
    } else {
        return false;
    }
}

static csel_node_s *parse_json_tree(struct json_object *obj,
                                    void *(*create_container) (struct json_object *))
{
    enum json_type type ATTRIBUTE((unused));
    csel_node_s *prevnode = NULL, *tmp, *node = NULL;

    json_object_object_foreach(obj, key, val) {
        type = json_object_get_type(val);
        MPIR_Assert(type == json_type_object);

        char *ckey = MPL_strdup_no_spaces(key);

        tmp = MPL_malloc(sizeof(csel_node_s), MPL_MEM_COLL);

        if (!strncmp(ckey, "composition=", strlen("composition=")) ||
            !strncmp(ckey, "algorithm=", strlen("algorithm="))) {
            tmp->type = CSEL_NODE_TYPE__CONTAINER;
            tmp->u.cnt.container = create_container(obj);
            tmp->success = NULL;
            tmp->failure = NULL;
            MPL_free(ckey);
            return tmp;
        }

        /* this node must be an operator type */
        tmp->success = parse_json_tree(json_object_object_get(obj, key), create_container);
        tmp->failure = NULL;

        if (node == NULL)
            node = tmp;
        else if (prevnode != NULL)
            prevnode->failure = tmp;
        prevnode = tmp;

        /* =any condition must be checked first */
        if (key_is_any(ckey)) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__ANY;
        } else if (!strcmp(ckey, "is_multi_threaded=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED;
            tmp->u.is_multi_threaded.val = true;
        } else if (!strcmp(ckey, "is_multi_threaded=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED;
            tmp->u.is_multi_threaded.val = false;
        } else if (!strcmp(ckey, "comm_type=intra")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA;
        } else if (!strcmp(ckey, "comm_type=inter")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER;
        } else if (!strncmp(ckey, "collective=", strlen("collective="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COLLECTIVE;

            char *str = ckey + strlen("collective=");

            bool matched = false;
            for (int idx = 0; idx < MPIR_CSEL_COLL_TYPE__END; idx++) {
                if (strcmp(str, Csel_coll_type_str[idx]) == 0) {
                    tmp->u.collective.coll_type = idx;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                MPIR_Assert(0);
            }
        } else if (!strcmp(ckey, "comm_size=pow2")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2;
        } else if (!strcmp(ckey, "comm_size=node_comm_size")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE;
        } else if (!strncmp(ckey, "comm_size<=", strlen("comm_size<="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE;
            tmp->u.comm_size_le.val = atoi(ckey + strlen("comm_size<="));
        } else if (!strncmp(ckey, "comm_size<", strlen("comm_size<"))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT;
            tmp->u.comm_size_lt.val = atoi(ckey + strlen("comm_size<"));
        } else if (!strncmp(ckey, "count<=", strlen("count<="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COUNT_LE;
            tmp->u.count_le.val = atoi(ckey + strlen("count<="));
        } else if (!strcmp(ckey, "count<pow2")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2;
        } else if (!strncmp(ckey, "avg_msg_size<=", strlen("avg_msg_size<="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE;
            tmp->u.avg_msg_size_le.val = atoi(ckey + strlen("avg_msg_size<="));
        } else if (!strncmp(ckey, "avg_msg_size<", strlen("avg_msg_size<"))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT;
            tmp->u.avg_msg_size_lt.val = atoi(ckey + strlen("avg_msg_size<"));
        } else if (!strncmp(ckey, "total_msg_size<=", strlen("total_msg_size<="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE;
            tmp->u.total_msg_size_le.val = atoi(ckey + strlen("total_msg_size<="));
        } else if (!strncmp(ckey, "total_msg_size<", strlen("total_msg_size<"))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT;
            tmp->u.total_msg_size_lt.val = atoi(ckey + strlen("total_msg_size<"));
        } else if (!strcmp(ckey, "is_commutative=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE;
            tmp->u.is_commutative.val = true;
        } else if (!strcmp(ckey, "is_commutative=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE;
            tmp->u.is_commutative.val = false;
        } else if (!strcmp(ckey, "is_sendbuf_inplace=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE;
            tmp->u.is_sbuf_inplace.val = true;
        } else if (!strcmp(ckey, "is_sendbuf_inplace=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE;
            tmp->u.is_sbuf_inplace.val = false;
        } else if (!strcmp(ckey, "is_op_built_in=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN;
            tmp->u.is_op_built_in.val = true;
        } else if (!strcmp(ckey, "is_op_built_in=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN;
            tmp->u.is_op_built_in.val = false;
        } else if (!strcmp(ckey, "is_block_regular=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR;
            tmp->u.is_block_regular.val = true;
        } else if (!strcmp(ckey, "is_block_regular=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR;
            tmp->u.is_block_regular.val = false;
        } else if (!strcmp(ckey, "is_node_consecutive=yes")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE;
            tmp->u.is_node_consecutive.val = true;
        } else if (!strcmp(ckey, "is_node_consecutive=no")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE;
            tmp->u.is_node_consecutive.val = false;
        } else if (!strncmp(ckey, "comm_avg_ppn<=", strlen("comm_avg_ppn<="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE;
            tmp->u.comm_avg_ppn_le.val = atoi(ckey + strlen("comm_avg_ppn<="));
        } else if (!strncmp(ckey, "comm_avg_ppn<", strlen("comm_avg_ppn<"))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT;
            tmp->u.comm_avg_ppn_le.val = atoi(ckey + strlen("comm_avg_ppn<"));
        } else if (!strncmp(ckey, "comm_hierarchy=", strlen("comm_hierarchy="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY;

            char *str = ckey + strlen("comm_hierarchy=");

            for (int idx = 0; idx < MPIR_CSEL_COMM_HIERARCHY__END; idx++) {
                if (strcmp(str, Csel_comm_hierarchy_str[idx]) == 0) {
                    tmp->u.comm_hierarchy.val = idx;
                    break;
                }
            }
        } else {
            fprintf(stderr, "unknown key %s\n", key);
            fflush(stderr);
            MPIR_Assert(0);
        }

        MPL_free(ckey);
    }

    return node;
}

int MPIR_Csel_create_from_buf(const char *json,
                              void *(*create_container) (struct json_object *), void **csel_)
{
    csel_s *csel = NULL;
    struct json_object *tree;

    csel = (csel_s *) MPL_malloc(sizeof(csel_s), MPL_MEM_COLL);
    csel->type = CSEL_TYPE__ROOT;
    tree = json_tokener_parse(json);
    if (tree == NULL)
        goto fn_exit;
    csel->u.root.tree = parse_json_tree(tree, create_container);

    if (csel->u.root.tree)
        validate_tree(csel->u.root.tree);

    csel_tree_optimize(&csel->u.root.tree);

    json_object_put(tree);

  fn_exit:
    *csel_ = csel;
    return 0;
}

void MPIR_Csel_debug_summary(const char *name, void *csel_)
{
    csel_s *csel = (csel_s *) csel_;
    if (MPIR_Process.rank != 0) {
        return;
    }

    if (MPIR_CVAR_COLLECTIVE_SELECTION_REPORT == MPIR_CVAR_COLLECTIVE_SELECTION_REPORT_tree
        || MPIR_CVAR_COLLECTIVE_SELECTION_REPORT == MPIR_CVAR_COLLECTIVE_SELECTION_REPORT_all) {
        printf("Processed Collective Selection Tree for %s:\n", name);
        printf("============================================================\n");
        Csel_print_tree(csel->u.root.tree);
        printf("============================================================\n");
    }
    if (MPIR_CVAR_COLLECTIVE_SELECTION_REPORT == MPIR_CVAR_COLLECTIVE_SELECTION_REPORT_summary
        || MPIR_CVAR_COLLECTIVE_SELECTION_REPORT == MPIR_CVAR_COLLECTIVE_SELECTION_REPORT_all) {
        printf("Summary of rules per collective algorithm for %s:\n", name);
        printf("============================================================\n");
        Csel_print_rules(csel->u.root.tree);
        printf("============================================================\n");
    }
}

int MPIR_Csel_create_from_file(const char *json_file,
                               void *(*create_container) (struct json_object *), void **csel_)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Assert(strcmp(json_file, ""));

    int fd = open(json_file, O_RDONLY);
    MPIR_ERR_CHKANDJUMP1(fd == -1, mpi_errno, MPI_ERR_INTERN, "**opencolltuningfile",
                         "**opencolltuningfile %s", json_file);

    struct stat st;
    stat(json_file, &st);
    char *json = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    MPIR_Csel_create_from_buf(json, create_container, csel_);

  fn_fail:
    return mpi_errno;
}

static csel_node_s *prune_tree(csel_node_s * root, MPIR_Comm * comm_ptr)
{
    /* Do not prune tree based on CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED, as during init
     * MPIR_IS_THREADED is set to 0 temporarily, which results in having incorrect pruned tree */
    for (csel_node_s * node = root; node;) {
        switch (node->type) {
            case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA:
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER:
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE:
                if (comm_ptr->local_size <= node->u.comm_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT:
                if (comm_ptr->local_size < node->u.comm_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) && comm_ptr->num_external == 1)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2:
                if (comm_ptr->local_size & (comm_ptr->local_size - 1))
                    node = node->failure;
                else
                    node = node->success;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY:
                if (MPIR_Comm_is_parent_comm(comm_ptr)
                    && (node->u.comm_hierarchy.val == MPIR_CSEL_COMM_HIERARCHY__PARENT))
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE:
                if (MPII_Comm_is_node_consecutive(comm_ptr) == node->u.is_node_consecutive.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) &&
                    comm_ptr->local_size <= node->u.comm_avg_ppn_le.val * comm_ptr->num_external)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) &&
                    comm_ptr->local_size < node->u.comm_avg_ppn_le.val * comm_ptr->num_external)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__ANY:
                node = node->success;
                break;

            default:
                return node;
        }
    }
    return root;
}

/* The prune function allows us to simplify the tree for specific
 * communicators using comm-specific information (such as size and
 * intra/inter comm type. */
int MPIR_Csel_prune(void *root_csel, MPIR_Comm * comm_ptr, void **comm_csel_)
{
    int mpi_errno = MPI_SUCCESS;
    csel_s *csel = (csel_s *) root_csel;
    csel_s *comm_csel = NULL;

    MPIR_Assert(root_csel);
    MPIR_Assert(comm_ptr);

    comm_csel = (csel_s *) MPL_malloc(sizeof(csel_s), MPL_MEM_COLL);
    MPIR_Assert(comm_csel);

    comm_csel->type = CSEL_TYPE__PRUNED;
    for (int i = 0; i < MPIR_CSEL_COLL_TYPE__END; i++)
        comm_csel->u.pruned.coll_trees[i] = NULL;

    /* prune the tree as far as possible */
    csel_node_s *node = prune_tree(csel->u.root.tree, comm_ptr);

    /* if the tree is not NULL, we should be at a collective branch at
     * this point */
    if (node) {
        MPIR_Assert(node->type == CSEL_NODE_TYPE__OPERATOR__COLLECTIVE);
    }

    while (node) {
        /* see if any additional pruning is possible once the
         * collective type is removed from the tree */
        comm_csel->u.pruned.coll_trees[node->u.collective.coll_type] =
            prune_tree(node->success, comm_ptr);
        node = node->failure;
    }

    *comm_csel_ = comm_csel;
    return mpi_errno;
}

int MPIR_Csel_free(void *csel_)
{
    int mpi_errno = MPI_SUCCESS;
    csel_s *csel = (csel_s *) csel_;

    if (csel->type == CSEL_TYPE__ROOT && csel->u.root.tree)
        csel_tree_free(&csel->u.root.tree);

    MPL_free(csel);
    return mpi_errno;
}

static inline bool is_sendbuf_inplace(MPIR_Csel_coll_sig_s coll_info)
{
    bool sendbuf_inplace = false;
    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLTOALL:
            sendbuf_inplace = (coll_info.u.alltoall.sendbuf == MPI_IN_PLACE);
            break;
        case MPIR_CSEL_COLL_TYPE__IALLTOALL:
            sendbuf_inplace = (coll_info.u.ialltoall.sendbuf == MPI_IN_PLACE);
            break;
        case MPIR_CSEL_COLL_TYPE__ALLTOALLV:
            sendbuf_inplace = (coll_info.u.alltoallv.sendbuf == MPI_IN_PLACE);
            break;
        case MPIR_CSEL_COLL_TYPE__IALLTOALLV:
            sendbuf_inplace = (coll_info.u.ialltoallv.sendbuf == MPI_IN_PLACE);
            break;
        case MPIR_CSEL_COLL_TYPE__ALLTOALLW:
            sendbuf_inplace = (coll_info.u.alltoallw.sendbuf == MPI_IN_PLACE);
            break;
        case MPIR_CSEL_COLL_TYPE__IALLTOALLW:
            sendbuf_inplace = (coll_info.u.ialltoallw.sendbuf == MPI_IN_PLACE);
            break;
        default:
            fprintf(stderr, "is_sendbuf_inplace not defined for coll_type %d\n",
                    coll_info.coll_type);
            MPIR_Assert(0);
            break;
    }
    return sendbuf_inplace;
}

static inline bool is_commutative(MPIR_Csel_coll_sig_s coll_info)
{
    bool commutative = false;
    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
            commutative = MPIR_Op_is_commutative(coll_info.u.allreduce.op);
            break;
        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            commutative = MPIR_Op_is_commutative(coll_info.u.iallreduce.op);
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE:
            commutative = MPIR_Op_is_commutative(coll_info.u.reduce.op);
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE:
            commutative = MPIR_Op_is_commutative(coll_info.u.ireduce.op);
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER:
            commutative = MPIR_Op_is_commutative(coll_info.u.reduce_scatter.op);
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER:
            commutative = MPIR_Op_is_commutative(coll_info.u.ireduce_scatter.op);
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK:
            commutative = MPIR_Op_is_commutative(coll_info.u.reduce_scatter_block.op);
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK:
            commutative = MPIR_Op_is_commutative(coll_info.u.ireduce_scatter_block.op);
            break;
        default:
            fprintf(stderr, "is_commutative not defined for coll_type %d\n", coll_info.coll_type);
            MPIR_Assert(0);
            break;
    }
    return commutative;
}

static inline bool is_op_built_in(MPIR_Csel_coll_sig_s coll_info)
{
    bool op_built_in = false;
    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
            op_built_in = HANDLE_GET_KIND(coll_info.u.allreduce.op) == HANDLE_KIND_BUILTIN;
            break;
        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            op_built_in = HANDLE_GET_KIND(coll_info.u.iallreduce.op) == HANDLE_KIND_BUILTIN;
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE:
            op_built_in = HANDLE_GET_KIND(coll_info.u.reduce.op) == HANDLE_KIND_BUILTIN;
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE:
            op_built_in = HANDLE_GET_KIND(coll_info.u.ireduce.op) == HANDLE_KIND_BUILTIN;
            break;
        default:
            fprintf(stderr, "is_op_builtin not defined for coll_type %d\n", coll_info.coll_type);
            MPIR_Assert(0);
            break;
    }
    return op_built_in;
}

static inline bool is_block_regular(MPIR_Csel_coll_sig_s coll_info)
{
    bool is_regular = true;
    int i = 0;
    const MPI_Aint *recvcounts = NULL;

    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER:
            recvcounts = coll_info.u.reduce_scatter.recvcounts;
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER:
            recvcounts = coll_info.u.ireduce_scatter.recvcounts;
            break;
        default:
            MPIR_Assert(0);
            break;
    }
    for (i = 0; i < (coll_info.comm_ptr->local_size - 1); ++i) {
        if (recvcounts[i] != recvcounts[i + 1]) {
            is_regular = false;
            break;
        }
    }
    return is_regular;
}

static inline MPI_Aint get_avg_msgsize(MPIR_Csel_coll_sig_s coll_info)
{
    MPI_Aint msgsize = 0;

    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.allreduce.datatype, msgsize);
            msgsize *= coll_info.u.allreduce.count;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.iallreduce.datatype, msgsize);
            msgsize *= coll_info.u.iallreduce.count;
            break;

        case MPIR_CSEL_COLL_TYPE__BCAST:
            MPIR_Datatype_get_size_macro(coll_info.u.bcast.datatype, msgsize);
            msgsize *= coll_info.u.bcast.count;
            break;

        case MPIR_CSEL_COLL_TYPE__IBCAST:
            MPIR_Datatype_get_size_macro(coll_info.u.ibcast.datatype, msgsize);
            msgsize *= coll_info.u.ibcast.count;
            break;

        case MPIR_CSEL_COLL_TYPE__REDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.reduce.datatype, msgsize);
            msgsize *= coll_info.u.reduce.count;
            break;

        case MPIR_CSEL_COLL_TYPE__IREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.ireduce.datatype, msgsize);
            msgsize *= coll_info.u.ireduce.count;
            break;

        case MPIR_CSEL_COLL_TYPE__ALLTOALL:
            MPIR_Datatype_get_size_macro(coll_info.u.alltoall.sendtype, msgsize);
            msgsize *= coll_info.u.alltoall.sendcount;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLTOALL:
            MPIR_Datatype_get_size_macro(coll_info.u.ialltoall.sendtype, msgsize);
            msgsize *= coll_info.u.ialltoall.sendcount;
            break;

        default:
            fprintf(stderr, "avg_msg_size not defined for coll_type %d\n", coll_info.coll_type);
            MPIR_Assert(0);
            break;
    }

    return msgsize;
}

static inline MPI_Aint get_count(MPIR_Csel_coll_sig_s coll_info)
{
    MPI_Aint count = 0;
    int i = 0;
    int comm_size = coll_info.comm_ptr->local_size;

    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__BCAST:
            count = coll_info.u.bcast.count;
            break;
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
            count = coll_info.u.allreduce.count;
            break;
        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            count = coll_info.u.iallreduce.count;
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE:
            count = coll_info.u.reduce.count;
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE:
            count = coll_info.u.ireduce.count;
            break;
        case MPIR_CSEL_COLL_TYPE__ALLGATHER:
            count = coll_info.u.allgather.recvcount;
            break;
        case MPIR_CSEL_COLL_TYPE__ALLGATHERV:
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.allgatherv.recvcounts[i];
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER:
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.reduce_scatter.recvcounts[i];
            break;
        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK:
            count = coll_info.u.reduce_scatter_block.recvcount;
            break;
        case MPIR_CSEL_COLL_TYPE__IALLGATHER:
            count = coll_info.u.iallgather.recvcount;
            break;
        case MPIR_CSEL_COLL_TYPE__IALLGATHERV:
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.iallgatherv.recvcounts[i];
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER:
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.ireduce_scatter.recvcounts[i];
            break;
        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK:
            count = coll_info.u.ireduce_scatter_block.recvcount;
            break;
        default:
            MPIR_Assert(0);
            break;
    }
    return count;
}

static inline MPI_Aint get_total_msgsize(MPIR_Csel_coll_sig_s coll_info)
{
    MPI_Aint total_bytes = 0, i = 0, count = 0, typesize = 0;
    int comm_size = coll_info.comm_ptr->local_size;

    switch (coll_info.coll_type) {
        case MPIR_CSEL_COLL_TYPE__ALLREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.allreduce.datatype, total_bytes);
            total_bytes *= coll_info.u.allreduce.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__BCAST:
            MPIR_Datatype_get_size_macro(coll_info.u.bcast.datatype, total_bytes);
            total_bytes *= coll_info.u.bcast.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__REDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.reduce.datatype, total_bytes);
            total_bytes *= coll_info.u.reduce.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__ALLTOALL:
            MPIR_Datatype_get_size_macro(coll_info.u.alltoall.sendtype, total_bytes);
            total_bytes *= coll_info.u.alltoall.sendcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__ALLTOALLV:
            MPIR_Datatype_get_size_macro(coll_info.u.alltoallv.sendtype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.alltoallv.sendcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__ALLTOALLW:
            count = 0;
            typesize = 0;
            for (i = 0; i < comm_size; i++) {
                MPIR_Datatype_get_size_macro(coll_info.u.alltoallw.sendtypes[i], typesize);
                count = coll_info.u.alltoallw.sendcounts[i];
                total_bytes += (count * typesize);
            }
            break;

        case MPIR_CSEL_COLL_TYPE__ALLGATHER:
            MPIR_Datatype_get_size_macro(coll_info.u.allgather.recvtype, total_bytes);
            total_bytes *= coll_info.u.allgather.recvcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__ALLGATHERV:
            MPIR_Datatype_get_size_macro(coll_info.u.allgatherv.recvtype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.allgatherv.recvcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__GATHER:
            if (coll_info.u.gather.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_info.u.gather.recvtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_info.comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes = coll_info.u.gather.recvcount * (coll_info.comm_ptr->remote_size);
                else
                    total_bytes = coll_info.u.gather.recvcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_info.u.gather.sendtype, total_bytes);
                total_bytes = coll_info.u.gather.sendcount * comm_size;
            }
            break;

        case MPIR_CSEL_COLL_TYPE__SCATTER:
            if (coll_info.u.scatter.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_info.u.scatter.sendtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_info.comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes = coll_info.u.scatter.sendcount * (coll_info.comm_ptr->remote_size);
                else
                    total_bytes = coll_info.u.scatter.sendcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_info.u.scatter.recvtype, total_bytes);
                total_bytes = coll_info.u.scatter.recvcount * comm_size;
            }
            break;

        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER:
            MPIR_Datatype_get_size_macro(coll_info.u.reduce_scatter.datatype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.reduce_scatter.recvcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__REDUCE_SCATTER_BLOCK:
            MPIR_Datatype_get_size_macro(coll_info.u.reduce_scatter_block.datatype, total_bytes);
            total_bytes *= coll_info.u.reduce_scatter_block.recvcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.iallreduce.datatype, total_bytes);
            total_bytes *= coll_info.u.iallreduce.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IBCAST:
            MPIR_Datatype_get_size_macro(coll_info.u.ibcast.datatype, total_bytes);
            total_bytes *= coll_info.u.ibcast.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IREDUCE:
            MPIR_Datatype_get_size_macro(coll_info.u.ireduce.datatype, total_bytes);
            total_bytes *= coll_info.u.ireduce.count * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLTOALL:
            MPIR_Datatype_get_size_macro(coll_info.u.ialltoall.sendtype, total_bytes);
            total_bytes *= coll_info.u.ialltoall.sendcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLTOALLV:
            MPIR_Datatype_get_size_macro(coll_info.u.ialltoallv.sendtype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.ialltoallv.sendcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLTOALLW:
            count = 0;
            typesize = 0;
            for (i = 0; i < comm_size; i++) {
                MPIR_Datatype_get_size_macro(coll_info.u.ialltoallw.sendtypes[i], typesize);
                count = coll_info.u.ialltoallw.sendcounts[i];
                total_bytes += (count * typesize);
            }
            break;

        case MPIR_CSEL_COLL_TYPE__IALLGATHER:
            MPIR_Datatype_get_size_macro(coll_info.u.iallgather.recvtype, total_bytes);
            total_bytes *= coll_info.u.iallgather.recvcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IALLGATHERV:
            MPIR_Datatype_get_size_macro(coll_info.u.iallgatherv.recvtype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.iallgatherv.recvcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER:
            MPIR_Datatype_get_size_macro(coll_info.u.ireduce_scatter.datatype, total_bytes);
            count = 0;
            for (i = 0; i < comm_size; i++)
                count += coll_info.u.ireduce_scatter.recvcounts[i];
            total_bytes *= count;
            break;

        case MPIR_CSEL_COLL_TYPE__IREDUCE_SCATTER_BLOCK:
            MPIR_Datatype_get_size_macro(coll_info.u.ireduce_scatter_block.datatype, total_bytes);
            total_bytes = coll_info.u.ireduce_scatter_block.recvcount * comm_size;
            break;

        case MPIR_CSEL_COLL_TYPE__IGATHER:
            if (coll_info.u.igather.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_info.u.igather.recvtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_info.comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes = coll_info.u.igather.recvcount * (coll_info.comm_ptr->remote_size);
                else
                    total_bytes = coll_info.u.igather.recvcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_info.u.igather.sendtype, total_bytes);
                total_bytes = coll_info.u.igather.sendcount * comm_size;
            }
            break;

        case MPIR_CSEL_COLL_TYPE__ISCATTER:
            if (coll_info.u.iscatter.root == MPI_ROOT) {
                MPIR_Datatype_get_size_macro(coll_info.u.iscatter.sendtype, total_bytes);
                /* use remote size for intercomm root */
                if (coll_info.comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    total_bytes =
                        coll_info.u.iscatter.sendcount * (coll_info.comm_ptr->remote_size);
                else
                    total_bytes = coll_info.u.iscatter.sendcount * comm_size;
            } else {
                MPIR_Datatype_get_size_macro(coll_info.u.iscatter.recvtype, total_bytes);
                total_bytes = coll_info.u.iscatter.recvcount * comm_size;
            }
            break;

        default:
            MPIR_Assert(0);
            break;
    }

    return total_bytes;
}

void *MPIR_Csel_search(void *csel_, MPIR_Csel_coll_sig_s coll_info)
{
    csel_s *csel = (csel_s *) csel_;
    csel_node_s *node = NULL;
    MPIR_Comm *comm_ptr = coll_info.comm_ptr;

    MPIR_Assert(csel_);

    csel_node_s *root;
    if (csel->type == CSEL_TYPE__ROOT)
        root = csel->u.root.tree;
    else
        root = csel->u.pruned.coll_trees[coll_info.coll_type];

    for (node = root; node;) {
        switch (node->type) {
            case CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED:
                if (MPIR_IS_THREADED == node->u.is_multi_threaded.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA:
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER:
                if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTERCOMM)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE:
                if (comm_ptr->local_size <= node->u.comm_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT:
                if (comm_ptr->local_size <= node->u.comm_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) && comm_ptr->num_external == 1)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2:
                if (!(comm_ptr->local_size & (comm_ptr->local_size - 1)))
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
                if (node->u.collective.coll_type == coll_info.coll_type)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE:
                if (get_avg_msgsize(coll_info) <= node->u.avg_msg_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT:
                if (get_avg_msgsize(coll_info) < node->u.avg_msg_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE:
                if (get_total_msgsize(coll_info) <= node->u.total_msg_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT:
                if (get_total_msgsize(coll_info) < node->u.total_msg_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COUNT_LE:
                if (get_count(coll_info) <= node->u.count_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2:
                if (get_count(coll_info) < MPL_pof2(coll_info.comm_ptr->local_size))
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE:
                if (is_commutative(coll_info) == node->u.is_commutative.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE:
                if (is_sendbuf_inplace(coll_info) == node->u.is_sbuf_inplace.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN:
                if (is_op_built_in(coll_info) == node->u.is_op_built_in.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR:
                if (is_block_regular(coll_info) == node->u.is_block_regular.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE:
                if (MPII_Comm_is_node_consecutive(coll_info.comm_ptr) ==
                    node->u.is_node_consecutive.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) &&
                    comm_ptr->local_size <= node->u.comm_avg_ppn_le.val * comm_ptr->num_external)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT:
                if ((comm_ptr->attr & MPIR_COMM_ATTR__HIERARCHY) &&
                    comm_ptr->local_size < node->u.comm_avg_ppn_le.val * comm_ptr->num_external)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY:
                if (MPIR_Comm_is_parent_comm(comm_ptr) == node->u.comm_hierarchy.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__ANY:
                node = node->success;
                break;

            case CSEL_NODE_TYPE__CONTAINER:
                goto fn_exit;

            default:
                MPIR_Assert(0);
        }
    }

    MPIR_Assert(node == NULL);

  fn_exit:
    return node ? node->u.cnt.container : NULL;
}
