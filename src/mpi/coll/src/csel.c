/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpl.h"
#include "coll_csel.h"
#include <errno.h>
#include <limits.h>

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
    const MPII_Csel_json_s *json_obj;   /* valid only while loading */
};

/* a dictionary maps subtree name to index into the utarray csel_subtree */
struct csel_subtree_hash_item *csel_subtree_hash;
/* a dynamic array of (struct csel_subtree) subtrees */
UT_array *csel_subtrees = NULL;
/* index to the main tree */
int csel_main_idx = -1;

static int csel_name_to_idx(const char *name);
static MPIR_Csel_node_s *csel_get_tree_by_idx(int idx);
static void add_named_tree(const char *name, const MPII_Csel_json_s * obj);
static void replace_named_tree(int idx, const MPII_Csel_json_s * obj);
static int parse_named_trees(void);
static void free_tree(MPIR_Csel_node_s * node);

static bool key_is_any(const char *ckey)
{
    if (strcmp(ckey, "any") == 0) {
        return true;
    } else {
        return false;
    }
}

/* Parse the threshold that follows a condition name. "(N)" means the value is
 * at most N. The others are aliases: cond<=N is cond(N), cond<N is cond(N-1),
 * cond>N is !cond(N), and cond>=N is !cond(N-1). Return false if the threshold
 * is malformed or does not fit in an int. */
static bool parse_thresh(const char *s, MPIR_Csel_node_s * node)
{
    bool paren = false;
    int adjust = 0;
    if (s[0] == '(') {
        paren = true;
        s += 1;
    } else if (s[0] == '<' && s[1] == '=') {
        s += 2;
    } else if (s[0] == '<') {
        adjust = -1;
        s += 1;
    } else if (s[0] == '>' && s[1] == '=') {
        adjust = -1;
        node->u.condition.negate = true;
        s += 2;
    } else if (s[0] == '>') {
        node->u.condition.negate = true;
        s += 1;
    } else {
        return false;
    }

    char *end;
    errno = 0;
    long long val = strtoll(s, &end, 10);
    if (end == s || errno == ERANGE) {
        return false;
    }
    if (paren) {
        if (*end != ')') {
            return false;
        }
        end++;
    }
    val += adjust;
    if (*end != '\0' || val < INT_MIN || val > INT_MAX) {
        return false;
    }

    node->u.condition.thresh = (int) val;
    return true;
}

/* parse a condition key, e.g. "pof2", "!pof2", or "comm_size<8", into node */
static int parse_condition(const MPII_Csel_json_s * obj, MPIR_Csel_node_s * node)
{
    const char *ckey = obj->key;
    if (ckey[0] == '!') {
        node->u.condition.negate = true;
        ckey++;
    } else {
        node->u.condition.negate = false;
    }

    for (int i = 0; i < MPIR_CSEL_NUM_CONDITIONS; i++) {
        const char *name = MPIR_Csel_condition_names[i];
        if (MPIR_Csel_condition_has_thresh[i]) {
            int n = strlen(name);
            if (strncmp(ckey, name, n) == 0) {
                node->type = i;
                /* "!cond<N" is ambiguous to read, so "!" only goes with "(N)" */
                if (node->u.condition.negate && (ckey[n] == '<' || ckey[n] == '>')) {
                    return MPII_Csel_json_error(obj, "'!' cannot be used with <, <=, >, or >=");
                }
                if (!parse_thresh(ckey + n, node)) {
                    return MPII_Csel_json_error(obj,
                                                "bad threshold, expect (N), <N, <=N, >N, or >=N");
                }
                return MPI_SUCCESS;
            }
        } else if (strcmp(ckey, name) == 0) {
            node->type = i;
            return MPI_SUCCESS;
        }
    }

    return MPII_Csel_json_error(obj, "unknown condition");
}

static int parse_json_tree(const MPII_Csel_json_s * obj, MPIR_Csel_node_s ** node_out)
{
    int mpi_errno = MPI_SUCCESS;

    if (obj->type != MPII_CSEL_JSON_OBJECT) {
        if (obj->type != MPII_CSEL_JSON_STRING || strncmp(obj->u.str, "call=", 5) != 0) {
            return MPII_Csel_json_error(obj, "expect an object or a \"call=\" string");
        }

        int idx = csel_name_to_idx(obj->u.str + 5);
        if (idx < 0) {
            return MPII_Csel_json_error(obj, "unknown subtree name");
        }

        MPIR_Csel_node_s *tmp;
        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);
        tmp->type = CSEL_NODE_TYPE__OPERATOR__CALL;
        tmp->u.call.idx = idx;
        *node_out = tmp;
        return MPI_SUCCESS;
    }

    MPIR_Csel_node_s *prevnode = NULL, *node = NULL;

    MPII_CSEL_JSON_FOREACH(obj, val) {
        const char *ckey = val->key;

        MPIR_Csel_node_s *tmp;
        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);

        if (!strncmp(ckey, "algorithm=", strlen("algorithm="))) {
            const char *s = ckey + strlen("algorithm=");

            /* an algorithm is a leaf, so it cannot have siblings */
            if (val != obj->u.child || val->next) {
                MPL_free(tmp);
                mpi_errno = MPII_Csel_json_error(val, "an \"algorithm=\" key must be the only key "
                                                 "in its object");
                goto fn_fail;
            }

            MPII_Csel_container_s *cnt = MPL_calloc(sizeof(MPII_Csel_container_s), 1, MPL_MEM_COLL);
            tmp->type = CSEL_NODE_TYPE__CONTAINER;
            tmp->u.container = cnt;

            cnt->id = MPIR_CSEL_NUM_ALGORITHMS;
            for (int i = 0; i < MPIR_CSEL_NUM_ALGORITHMS; i++) {
                if (!strcmp(s, MPIR_Coll_algo_names[i])) {
                    cnt->id = i;
                    break;
                }
            }
            if (cnt->id == MPIR_CSEL_NUM_ALGORITHMS) {
                mpi_errno = MPII_Csel_json_error(val, "unknown algorithm");
            } else if (val->type != MPII_CSEL_JSON_OBJECT) {
                mpi_errno = MPII_Csel_json_error(val, "expect an object of parameters");
            } else {
                mpi_errno = MPII_Csel_parse_container_params(val, cnt);
            }
            if (mpi_errno) {
                free_tree(tmp);
                goto fn_fail;
            }

            node = tmp;
            goto fn_exit;
        }

        /* this node must be an operator type */
        mpi_errno = parse_json_tree(val, &tmp->success);
        if (mpi_errno) {
            MPL_free(tmp);
            goto fn_fail;
        }
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
            if (!flag) {
                mpi_errno = MPII_Csel_json_error(val, "unknown collective");
                goto fn_fail;
            }
        } else if (key_is_any(ckey)) {
            tmp->type = CSEL_NODE_TYPE__OPERATOR__ANY;
        } else {
            mpi_errno = parse_condition(val, tmp);
            if (mpi_errno != MPI_SUCCESS) {
                goto fn_fail;
            }
        }
    }

  fn_exit:
    *node_out = node;
    return mpi_errno;
  fn_fail:
    if (node) {
        free_tree(node);
    }
    node = NULL;
    goto fn_exit;
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

static int parse_json_names(const MPII_Csel_json_s * obj)
{
    int mpi_errno = MPI_SUCCESS;

    /* the parser always makes the root an object */
    MPIR_Assert(obj->type == MPII_CSEL_JSON_OBJECT);

    if (!csel_subtrees) {
        static UT_icd csel_subtree_icd =
            { sizeof(struct csel_subtree), NULL, NULL, csel_subtree_dtor };
        utarray_new(csel_subtrees, &csel_subtree_icd, MPL_MEM_COLL);
    }

    /* the keys are either all "name=", or none is and the whole tree is "name=main" */
    bool has_names = false;
    MPII_CSEL_JSON_FOREACH(obj, val) {
        bool is_name = (strncmp(val->key, "name=", 5) == 0);
        if (val == obj->u.child) {
            has_names = is_name;
        } else if (is_name != has_names) {
            return MPII_Csel_json_error(val, has_names ? "expect a \"name=\" key" :
                                        "\"name=\" keys must not mix with other keys");
        }
        if (is_name && strlen(val->key + 5) >= CSEL_MAX_NAME) {
            return MPII_Csel_json_error(val, "subtree name is too long");
        }
    }

    if (has_names) {
        MPII_CSEL_JSON_FOREACH(obj, val) {
            const char *name = val->key + 5;

            int idx = csel_name_to_idx(name);
            if (idx >= 0) {
                replace_named_tree(idx, val);
            } else {
                add_named_tree(name, val);
            }
        }
    } else {
        add_named_tree("main", obj);
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

    MPII_Csel_json_s *tree;
    mpi_errno = MPII_Csel_json_parse(json_str, &tree);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = parse_json_names(tree);

    MPII_Csel_json_free(tree);

  fn_fail:
    return mpi_errno;
}

int MPIR_Csel_load_file(const char *json_file)
{
    int mpi_errno = MPI_SUCCESS;
    char *json = NULL;
    long size = -1;

    MPIR_Assert(strcmp(json_file, ""));

    FILE *f = fopen(json_file, "rb");
    MPIR_ERR_CHKANDJUMP1(f == NULL, mpi_errno, MPI_ERR_INTERN, "**opencolltuningfile",
                         "**opencolltuningfile %s", json_file);

    if (fseek(f, 0, SEEK_END) == 0)
        size = ftell(f);
    if (size >= 0 && fseek(f, 0, SEEK_SET) == 0)
        json = MPL_malloc(size + 1, MPL_MEM_COLL);
    size_t nread = json ? fread(json, 1, size, f) : 0;
    fclose(f);
    MPIR_ERR_CHKANDJUMP1(json == NULL || size == 0 || nread != (size_t) size, mpi_errno,
                         MPI_ERR_INTERN, "**readcolltuningfile", "**readcolltuningfile %s",
                         json_file);
    json[size] = '\0';

    mpi_errno = MPIR_Csel_load_buf(json);

  fn_exit:
    MPL_free(json);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Csel_free(void)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_clear(csel_subtrees);
    utarray_free(csel_subtrees);
    csel_subtrees = NULL;

    struct csel_subtree_hash_item *cur, *tmp;
    HASH_ITER(hh, csel_subtree_hash, cur, tmp) {
        HASH_DEL(csel_subtree_hash, cur);
        MPL_free(cur);
    }

    return mpi_errno;
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

MPII_Csel_container_s *MPIR_Csel_search(MPIR_Csel_node_s * csel, MPIR_Csel_coll_sig_s * coll_sig)
{
    MPIR_Assert(csel);

    MPIR_Csel_node_s *node = csel;
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

/* internal routines */

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

static void add_named_tree(const char *name, const MPII_Csel_json_s * obj)
{
    struct csel_subtree item = { NULL, obj };
    utarray_push_back(csel_subtrees, &item, MPL_MEM_COLL);

    /* map the name to idx */
    struct csel_subtree_hash_item *h;
    h = MPL_malloc(sizeof(*h), MPL_MEM_OTHER);
    MPL_strncpy(h->name, name, CSEL_MAX_NAME);
    h->idx = utarray_len(csel_subtrees) - 1;
    HASH_ADD_STR(csel_subtree_hash, name, h, MPL_MEM_COLL);

    /* update csel_main_idx if name == "main" */
    if (strcmp(name, "main") == 0) {
        csel_main_idx = h->idx;
    }
}

static void replace_named_tree(int idx, const MPII_Csel_json_s * obj)
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
            mpi_errno = parse_json_tree(p->json_obj, &p->node);
            p->json_obj = NULL;
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_Assert(p->node);
        }
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    /* the JSON tree is freed after loading, so drop the remaining references */
    for (p = (struct csel_subtree *) utarray_front(csel_subtrees); p;
         p = (struct csel_subtree *) utarray_next(csel_subtrees, p)) {
        p->json_obj = NULL;
    }
    goto fn_exit;
}
