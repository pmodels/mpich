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

static bool key_is_any(const char *ckey)
{
    if (strcmp(ckey, "any") == 0) {
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
            /* TODO: process error */
            MPIR_Assert(cnt->id != MPIR_CSEL_NUM_ALGORITHMS);
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
            bool flag = false;
            for (int i = 0; i < MPIR_CSEL_NUM_COLL_TYPES; i++) {
                if (!strcmp(s, MPIR_Coll_type_names[i])) {
                    tmp->u.collective.coll_type = i;
                    flag = true;
                    break;
                }
            }
            MPIR_Assertp(flag);
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

int MPIR_Csel_create_from_buf(const char *json, MPIR_Csel_node_s ** csel)
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
    *csel = csel_root;
    return 0;
}

int MPIR_Csel_create_from_file(const char *json_file, MPIR_Csel_node_s ** csel)
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

    MPIR_Csel_create_from_buf(json, csel);

  fn_fail:
    return mpi_errno;
}

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

int MPIR_Csel_free(MPIR_Csel_node_s * csel_root)
{
    int mpi_errno = MPI_SUCCESS;

    if (csel_root) {
        free_tree(csel_root);
    }

    return mpi_errno;
}

MPII_Csel_container_s *MPIR_Csel_search(MPIR_Csel_node_s * csel, MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Assert(csel);

    MPIR_Csel_node_s *node = csel;
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
