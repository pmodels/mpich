/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpl.h"
#include "coll_csel.h"
#include <fcntl.h>      /* open */
#include <sys/mman.h>   /* mmap */
#include <sys/stat.h>

typedef enum {
    /* global operator types */
    CSEL_NODE_TYPE__OPERATOR__IS_MULTI_THREADED = 0,

    /* comm-specific operator types */
    CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTRA,
    CSEL_NODE_TYPE__OPERATOR__COMM_TYPE_INTER,

    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_LT,
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_NODE_COMM_SIZE,
    CSEL_NODE_TYPE__OPERATOR__COMM_SIZE_POW2,

    CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY,
    CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE,

    CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LE,
    CSEL_NODE_TYPE__OPERATOR__COMM_AVG_PPN_LT,

    /* collective selection operator */
    CSEL_NODE_TYPE__OPERATOR__COLLECTIVE,

    /* message-specific operator types */
    CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT,
    CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE,
    CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT,

    CSEL_NODE_TYPE__OPERATOR__COUNT_LE,
    CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2,

    CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE,
    CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR,
    CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE,
    CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN,

    /* any - has to be the last branch in an array */
    CSEL_NODE_TYPE__OPERATOR__ANY,

    /* container type */
    CSEL_NODE_TYPE__CONTAINER,
} csel_node_type_e;

typedef struct csel_node {
    csel_node_type_e type;

    union {
        /* global types */
        struct {
            int val;
        } is_multi_threaded;

        /* comm-specific operator types */
        struct {
            int val;
        } comm_size_le;
        struct {
            int val;
        } comm_size_lt;

        /* collective selection operator */
        struct {
            MPIR_Csel_coll_type_e coll_type;
        } collective;

        /* message-specific operator types */
        struct {
            int val;
        } avg_msg_size_le;
        struct {
            int val;
        } avg_msg_size_lt;
        struct {
            int val;
        } total_msg_size_le;
        struct {
            int val;
        } total_msg_size_lt;
        struct {
            int val;
        } count_le;
        struct {
            bool val;
        } is_commutative;
        struct {
            bool val;
        } is_sbuf_inplace;
        struct {
            bool val;
        } is_op_built_in;
        struct {
            bool val;
        } is_block_regular;
        struct {
            bool val;
        } is_node_consecutive;
        struct {
            int val;
        } comm_avg_ppn_le;
        struct {
            int val;
        } comm_avg_ppn_lt;
        struct {
            bool val;
        } comm_hierarchy;
        struct {
            void *container;
        } cnt;
    } u;

    struct csel_node *success;
    struct csel_node *failure;
} csel_node_s;

static bool key_is_any(const char *ckey)
{
    int len = strlen(ckey);

    if (strcmp(ckey, "any") == 0 || strcmp(ckey + len - 4, "=any") == 0) {
        return true;
    } else {
        return false;
    }
}

static csel_node_s *parse_json_tree(struct json_object *obj)
{
    enum json_type type ATTRIBUTE((unused));
    csel_node_s *prevnode = NULL, *tmp, *node = NULL;

    json_object_object_foreach(obj, key, val) {
        char *ckey = key;

        tmp = MPL_malloc(sizeof(csel_node_s), MPL_MEM_COLL);

        if (!strncmp(ckey, "algorithm=", strlen("algorithm="))) {
            const char *s = ckey + strlen("algorithm=");

            MPII_Csel_container_s *cnt = MPL_calloc(sizeof(MPII_Csel_container_s), 1, MPL_MEM_COLL);
            cnt->id = MPIR_CSEL_NUM_ALGORITHMS;
            for (int i = 0; i < MPIR_CSEL_NUM_ALGORITHMS; i++) {
                if (!strcmp(s, MPIR_Coll_algo_names[i])) {
                    cnt->id = i;
                    break;
                }
            }
            /* TODO: process error */
            MPIR_Assert(cnt->id != MPIR_CSEL_NUM_ALGORITHMS);
            MPII_Csel_parse_container_params(val, cnt);

            tmp->type = CSEL_NODE_TYPE__CONTAINER;
            tmp->u.cnt.container = cnt;
            return tmp;
        }

        /* this node must be an operator type */
        tmp->success = parse_json_tree(json_object_object_get(obj, key));
        tmp->failure = NULL;

        if (node == NULL)
            node = tmp;
        else if (prevnode != NULL)
            prevnode->failure = tmp;
        prevnode = tmp;

        if (!strcmp(ckey, "is_multi_threaded=yes")) {
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
            const char *s = ckey + strlen("collective=");
            bool flag = false;
            for (int i = 0; i < MPIR_CSEL_NUM_COLL_TYPES; i++) {
                if (!strcmp(s, MPIR_Coll_type_names[i])) {
                    tmp->u.collective.coll_type = i;
                    flag = true;
                    break;
                }
            }
            MPIR_Assert(flag);
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
        } else if (!strcmp(ckey, "comm_hierarchy=parent")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY;
            tmp->u.comm_hierarchy.val = true;
        } else if (!strcmp(ckey, "comm_hierarchy=node_roots")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY;
            tmp->u.comm_hierarchy.val = false;
        } else if (!strcmp(ckey, "comm_hierarchy=node")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY;
            tmp->u.comm_hierarchy.val = false;
        } else if (!strcmp(ckey, "comm_hierarchy=flat")) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COMM_HIERARCHY;
            tmp->u.comm_hierarchy.val = false;
        } else if (key_is_any(ckey)) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__ANY;
        } else {
            fprintf(stderr, "unknown key %s\n", key);
            fflush(stderr);
            MPIR_Assert(0);
        }
    }

    return node;
}

int MPIR_Csel_create_from_buf(const char *json, void **csel_)
{
    struct json_object *tree;
    tree = json_tokener_parse(json);
    if (tree == NULL)
        goto fn_exit;

    csel_node_s *csel_root = parse_json_tree(tree);
    MPIR_Assert(csel_root);

    json_object_put(tree);

  fn_exit:
    *csel_ = csel_root;
    return 0;
}

int MPIR_Csel_create_from_file(const char *json_file, void **csel_)
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

    MPIR_Csel_create_from_buf(json, csel_);

  fn_fail:
    return mpi_errno;
}

static void free_tree(csel_node_s * node)
{
    if (node->type == CSEL_NODE_TYPE__CONTAINER) {
        MPL_free(node->u.cnt.container);
        MPL_free(node);
    } else {
        if (node->success)
            free_tree(node->success);
        if (node->failure)
            free_tree(node->failure);
        MPL_free(node);
    }
}

int MPIR_Csel_free(void *csel_root)
{
    int mpi_errno = MPI_SUCCESS;

    if (csel_root) {
        free_tree(csel_root);
    }

    return mpi_errno;
}

void *MPIR_Csel_search(void *csel_, MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Comm *comm_ptr = coll_sig->comm_ptr;

    MPIR_Assert(csel_);

    csel_node_s *node = csel_;
    while (node) {
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
                if (node->u.collective.coll_type == coll_sig->coll_type)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LE:
                if (MPIR_Csel_avg_msg_size(coll_sig) <= node->u.avg_msg_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__AVG_MSG_SIZE_LT:
                if (MPIR_Csel_avg_msg_size(coll_sig) < node->u.avg_msg_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LE:
                if (MPIR_Csel_total_msg_size(coll_sig) <= node->u.total_msg_size_le.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__TOTAL_MSG_SIZE_LT:
                if (MPIR_Csel_total_msg_size(coll_sig) < node->u.total_msg_size_lt.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__COUNT_LT_POW2:
                if (!MPIR_Csel_count_ge_pof2(coll_sig))
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_COMMUTATIVE:
                if (MPIR_Csel_op_is_commutative(coll_sig) == node->u.is_commutative.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_SBUF_INPLACE:
                if (MPIR_Csel_sendbuf_inplace(coll_sig) == node->u.is_sbuf_inplace.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_OP_BUILT_IN:
                if (MPIR_Csel_op_is_builtin(coll_sig) == node->u.is_op_built_in.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_BLOCK_REGULAR:
                if (MPIR_Csel_block_regular(coll_sig) == node->u.is_block_regular.val)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__IS_NODE_CONSECUTIVE:
                if (MPII_Comm_is_node_consecutive(coll_sig->comm_ptr) ==
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
