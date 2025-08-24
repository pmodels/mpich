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
#include <json.h>

static void free_tree(MPIR_Csel_node_s * node);

static bool key_is_any(const char *ckey)
{
    int len = strlen(ckey);

    if (strcmp(ckey, "any") == 0 || strcmp(ckey + len - 5, "(any)") == 0) {
        return true;
    } else {
        return false;
    }
}

static MPIR_Csel_node_s *parse_json_tree(struct json_object *obj)
{
    enum json_type type ATTRIBUTE((unused));
    MPIR_Csel_node_s *prevnode = NULL, *tmp, *node = NULL;

    json_object_object_foreach(obj, key, val) {
        type = json_object_get_type(val);
        MPIR_Assert(type == json_type_object);

        const char *ckey = key;

        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);

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
            if (cnt->id == MPIR_CSEL_NUM_ALGORITHMS) {
                printf("parse_json_tree: unrecognized algorithm %s\n", s);
                return NULL;
            }

            MPII_Csel_parse_container_params(val, cnt);

            tmp->type = CSEL_NODE_TYPE__CONTAINER;
            tmp->u.container = cnt;
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

        if (!strncmp(ckey, "collective=", strlen("collective="))) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__COLLECTIVE;
            const char *s = ckey + strlen("collective=");
            tmp->u.collective.coll_type = MPIR_CSEL_COLL_TYPE__END;
            for (int i = 0; i < MPIR_CSEL_NUM_COLL_TYPES; i++) {
                if (!strcmp(s, MPIR_Coll_type_names[i])) {
                    tmp->u.collective.coll_type = i;
                    break;
                }
            }
            MPIR_Assert(tmp->u.collective.coll_type != MPIR_CSEL_COLL_TYPE__END);
        } else if (key_is_any(ckey)) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__ANY;
        } else {
            int mpi_errno = MPII_Csel_parse_operator(ckey, tmp);
            if (mpi_errno != MPI_SUCCESS) {
                printf("parse_json_tree: unknown key %s\n", ckey);
                MPIR_Assert(0);
            }
        }
    }

    return node;
}

int MPIR_Csel_create_from_buf(const char *json, void **csel_)
{
    MPIR_Csel_node_s *csel_root = NULL;

    struct json_object *tree;
    tree = json_tokener_parse(json);
    if (tree == NULL)
        goto fn_exit;

    csel_root = parse_json_tree(tree);
    MPIR_Assert(csel_root);

    json_object_put(tree);

  fn_exit:
    if (0 && MPIR_Process.rank == 0) {
        printf("====\n");
        MPIR_Csel_print_tree(csel_root, 0);
    }
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

int MPIR_Csel_free(void *csel_root)
{
    int mpi_errno = MPI_SUCCESS;

    if (csel_root) {
        free_tree(csel_root);
    }

    return mpi_errno;
}

MPII_Csel_container_s *MPIR_Csel_search(void *csel_, MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Assert(csel_);
    MPIR_Csel_node_s *node = csel_;
    while (node) {
        switch (node->type) {
            case CSEL_NODE_TYPE__OPERATOR__COLLECTIVE:
                if (node->u.collective.coll_type == coll_sig->coll_type)
                    node = node->success;
                else
                    node = node->failure;
                break;

            case CSEL_NODE_TYPE__OPERATOR__ANY:
                node = node->success;
                break;

            case CSEL_NODE_TYPE__CONTAINER:
                return node->u.container;

            default:
                node = MPII_Csel_run_condition(node, coll_sig);
                MPIR_Assert(node);
                break;
        }
    }

    MPIR_Assert(0 && "MPIR_Csel_search failed to find an algorithm");
    return NULL;
}

/* -- internal static routines -- */

static void free_tree(MPIR_Csel_node_s * node)
{
    if (node->type == CSEL_NODE_TYPE__CONTAINER) {
        MPL_free(node->u.container);
        MPL_free(node);
    } else {
        if (node->success)
            free_tree(node->success);
        if (node->failure)
            free_tree(node->failure);
        MPL_free(node);
    }
}

void MPIR_Csel_print_tree(MPIR_Csel_node_s * node, int level)
{
    for (int i = 0; i < level; i++) {
        printf("    ");
    }
    if (node->type == CSEL_NODE_TYPE__CONTAINER) {
        printf("Algorithm: %s\n", MPIR_Coll_algo_names[node->u.container->id]);
    } else if (node->type == CSEL_NODE_TYPE__OPERATOR__COLLECTIVE) {
        printf("Collective: %s\n", MPIR_Coll_type_names[node->u.collective.coll_type]);
    } else if (node->type == CSEL_NODE_TYPE__OPERATOR__ANY) {
        printf("ANY\n");
    } else {
        if (!node->u.condition.negate) {
            if (!node->u.condition.thresh) {
                printf("condition: %s\n", MPIR_Csel_condition_names[node->type]);
            } else {
                printf("condition: %s(%d)\n", MPIR_Csel_condition_names[node->type],
                       node->u.condition.thresh);
            }
        } else {
            if (!node->u.condition.thresh) {
                printf("condition: !%s\n", MPIR_Csel_condition_names[node->type]);
            } else {
                printf("condition: !%s(%d)\n", MPIR_Csel_condition_names[node->type],
                       node->u.condition.thresh);
            }
        }
    }

    if (node->success) {
        MPIR_Csel_print_tree(node->success, level + 1);
    }
    if (node->failure) {
        MPIR_Csel_print_tree(node->failure, level + 1);
    }
}
