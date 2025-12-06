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

#include "uthash.h"
#include "utarray.h"

#define CSEL_MAX_NAME 50
struct csel_subtree_hash_item {
    char name[CSEL_MAX_NAME];   /* key */
    int idx;
    UT_hash_handle hh;
};

struct csel_subtree {
    MPIR_Csel_node_s *node;
    struct json_object *json_obj;
};

/* a dictionary maps subtree name to index into the utarray csel_subtree */
struct csel_subtree_hash_item *csel_subtree_hash;
/* a dynamic array of (struct csel_subtree) subtrees */
UT_array *csel_subtrees = NULL;
/* index to the main tree */
int csel_main_idx = -1;

static int csel_name_to_idx(const char *name);
static MPIR_Csel_node_s *csel_get_tree_by_idx(int idx);
static void add_named_tree(const char *name, struct json_object *obj);
static void replace_named_tree(int idx, struct json_object *obj);
static int parse_named_trees(void);
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
    enum json_type type;
    type = json_object_get_type(obj);

    if (type == json_type_string) {
        const char *ckey = json_object_get_string(obj);

        MPIR_Csel_node_s *tmp;
        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);

        if (!strncmp(ckey, "call=", 5)) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__CALL;
            tmp->u.call.idx = csel_name_to_idx(ckey + 5);
            MPIR_Assert(tmp->u.call.idx >= 0);
            return tmp;
        } else {
            MPIR_Assert(0);
            return NULL;
        }
    }

    MPIR_Assert(type == json_type_object);
    MPIR_Csel_node_s *prevnode = NULL, *node = NULL;

    json_object_object_foreach(obj, key, val) {
        const char *ckey = key;

        MPIR_Csel_node_s *tmp;
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

/* parse the json_object assuming it is a list of "name=xxx": subtree.
 * Otherwise, parse it as a single tree and set the name to "main".
 */
static void csel_subtree_dtor(void *item)
{
    struct csel_subtree *p = item;
    MPIR_Assert(p->json_obj == NULL);
    if (p->node) {
        free_tree(p->node);
        p->node = NULL;
    }
}

static int parse_json_names(struct json_object *obj)
{
    int mpi_errno = MPI_SUCCESS;

    if (!csel_subtrees) {
        static UT_icd csel_subtree_icd =
            { sizeof(struct csel_subtree), NULL, NULL, csel_subtree_dtor };
        utarray_new(csel_subtrees, &csel_subtree_icd, MPL_MEM_COLL);
    }

    int num_names = 0;
    bool unexpected = false;
    json_object_object_foreach(obj, key, val) {
        if (strncmp(key, "name=", 5) == 0) {
            const char *name = key + 5;

            int idx = csel_name_to_idx(name);
            if (idx >= 0) {
                replace_named_tree(idx, json_object_get(val));
            } else {
                add_named_tree(name, json_object_get(val));
            }
            num_names++;
        } else {
            unexpected = true;
            break;
        }
    }
    if (unexpected) {
        MPIR_Assert(num_names == 0);
        /* pass the whole tree as "name=main" */
        add_named_tree("main", json_object_get(obj));
    }

    MPIR_Assert(csel_main_idx >= 0);
    /* FIXME: error handling */

    mpi_errno = parse_named_trees();
    MPIR_ERR_CHECK(mpi_errno);

  fn_fail:
    return mpi_errno;
}

int MPIR_Csel_load_buf(const char *json_str)
{
    int mpi_errno = MPI_SUCCESS;

    struct json_object *tree;
    tree = json_tokener_parse(json_str);
    if (tree == NULL)
        goto fn_exit;

    mpi_errno = parse_json_names(tree);

    json_object_put(tree);

  fn_exit:
    return mpi_errno;
}

int MPIR_Csel_load_file(const char *json_file)
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

    mpi_errno = MPIR_Csel_load_buf(json);

  fn_fail:
    return mpi_errno;
}

int MPIR_Csel_free(void)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_clear(csel_subtrees);
    utarray_free(csel_subtrees);
    csel_subtrees = NULL;

    return mpi_errno;
}

MPII_Csel_container_s *MPIR_Csel_search(void *csel_, MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Assert(csel_);
    MPIR_Csel_node_s *node = csel_;
    while (node) {
        switch (node->type) {
            case CSEL_NODE_TYPE__OPERATOR__CALL:
                MPIR_Csel_node_s * tree = csel_get_tree_by_idx(node->u.call.idx);
                if (!tree) {
                    goto fn_fail;
                }
                return MPIR_Csel_search(tree, coll_sig);

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

  fn_fail:
    MPIR_Assert(0 && "MPIR_Csel_search failed to find an algorithm");
    return NULL;
}

MPIR_Csel_node_s *MPIR_Csel_get_tree(const char *name)
{
    int idx = csel_name_to_idx(name);
    if (idx >= 0) {
        return csel_get_tree_by_idx(idx);
    }
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

static int csel_name_to_idx(const char *name)
{
    struct csel_subtree_hash_item *h;
    HASH_FIND_STR(csel_subtree_hash, name, h);
    if (h) {
        return h->idx;
    } else {
        return -1;
    }
}

static MPIR_Csel_node_s *csel_get_tree_by_idx(int idx)
{
    struct csel_subtree *p = (void *) utarray_eltptr(csel_subtrees, idx);
    MPIR_Assert(p);
    MPIR_Assert(p->node);
    return p->node;
}

static void add_named_tree(const char *name, struct json_object *obj)
{
    struct csel_subtree item = { NULL, obj };
    utarray_push_back(csel_subtrees, &item, MPL_MEM_COLL);

    /* map the name to idx */
    struct csel_subtree_hash_item *h;
    h = MPL_malloc(sizeof(*h), MPL_MEM_OTHER);
    strncpy(h->name, name, CSEL_MAX_NAME - 1);
    h->idx = utarray_len(csel_subtrees) - 1;
    HASH_ADD_STR(csel_subtree_hash, name, h, MPL_MEM_COLL);

    /* update csel_main_idx if name == "main" */
    if (strcmp(name, "main") == 0) {
        csel_main_idx = h->idx;
    }
}

static void replace_named_tree(int idx, struct json_object *obj)
{
    struct csel_subtree *p = (void *) utarray_eltptr(csel_subtrees, idx);
    MPIR_Assert(p);
    p->json_obj = obj;
    free_tree(p->node);
    p->node = NULL;
}

static int parse_named_trees(void)
{
    int mpi_errno = MPI_SUCCESS;

    struct csel_subtree *p;
    for (p = (struct csel_subtree *) utarray_front(csel_subtrees); p;
         p = (struct csel_subtree *) utarray_next(csel_subtrees, p)) {
        if (!p->node && p->json_obj) {
            p->node = parse_json_tree(p->json_obj);
            MPIR_Assert(p->node);
            json_object_put(p->json_obj);
            p->json_obj = NULL;
        }
    }
    return mpi_errno;
}
