/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
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
    int tree_idx;
    UT_hash_handle hh;
};

/* what a subtree's name says: a restriction set of coll_type, or a free subtree */
struct csel_set {
    int coll_type;              /* -1: a free name, reached only by call= */
    unsigned mask;
    unsigned variant_mask;      /* the -xxx variants: tuned under the restrictions xxx_coll */
};

struct csel_subtree {
    MPIR_Csel_node_s *node;
    const MPII_Csel_json_s *json_obj;   /* valid only while loading */
    struct csel_set set;
};

/* a dictionary maps subtree name to index into the utarray csel_subtree */
struct csel_subtree_hash_item *csel_subtree_hash;
/* a dynamic array of (struct csel_subtree) subtrees */
UT_array *csel_subtrees = NULL;

/* ---- csel subtree table per coll_type */
struct csel_tree_entry {
    int tree_idx;
    unsigned mask;
    unsigned variant_mask;
};

struct csel_lookup {
    /* aggregate mask (via OR) over all trees in the tables */
    unsigned aggregate_mask;
    /* local trees satisfies some local restrictions, e.g. gpu_local */
    int num_local;
    /* trees with no local restrictions */
    int num_other;
    struct csel_tree_entry *local;
    struct csel_tree_entry *other;
    struct csel_tree_entry tables[];
};

static struct csel_lookup *csel_lookups[MPIR_CSEL_NUM_COLL_TYPES];

static int build_lookups(void);

int MPII_Csel_dispatch(MPIR_Csel_coll_sig_s * coll_sig, unsigned drop_mask,
                       struct csel_dispatch_state *state)
{
    const struct csel_lookup *l = csel_lookups[coll_sig->coll_type];
    if (!l) {
        return -1;
    }

    /* build m if needed */
    if (!state->have_m) {
        state->m = MPIR_Csel_restriction_mask(coll_sig, l->aggregate_mask);
        /* a _coll and a _local restriction exclude each other */
        if (state->m & MPIR_CSEL_COLL_RESTRICTIONS) {
            state->m &= ~MPIR_CSEL_LOCAL_RESTRICTIONS;
        }
        state->have_m = true;
    }
    /* apply drop_mask */
    state->v |= state->m & drop_mask & MPIR_CSEL_COLL_RESTRICTIONS;
    state->m &= ~drop_mask;

    /* find the first tree whose restrictions hold, and whose variants are among v */
    for (int k = 0; k < l->num_local; k++) {
        /* skip if its mask contains bits that cur state does not satisfy */
        if (!(l->local[k].mask & ~state->m)) {
            return l->local[k].tree_idx;
        }
    }
    for (int k = 0; k < l->num_other; k++) {
        /* also skip if its variant is not in cur state's variant */
        if (!(l->other[k].mask & ~state->m) && !(l->other[k].variant_mask & ~state->v)) {
            return l->other[k].tree_idx;
        }
    }
    return -1;
}

static void free_lookups(void)
{
    for (int t = 0; t < MPIR_CSEL_NUM_COLL_TYPES; t++) {
        MPL_free(csel_lookups[t]);
        csel_lookups[t] = NULL;
    }
}

static int compare_trees(const void *a, const void *b);

static int build_lookups(void)
{
    int mpi_errno = MPI_SUCCESS;

    free_lookups();

    int n = utarray_len(csel_subtrees);
    for (int t = 0; t < MPIR_CSEL_NUM_COLL_TYPES; t++) {
        unsigned aggregate_mask = 0;
        int num_local = 0, num_other = 0;
        for (int i = 0; i < n; i++) {
            if (MPII_Csel_tree_is_coll_type(i, t)) {
                aggregate_mask |= MPII_Csel_tree_mask(i);
                if (MPII_Csel_tree_is_local(i)) {
                    num_local++;
                } else {
                    num_other++;
                }
            }
        }
        if (num_local + num_other == 0) {
            continue;
        }

        struct csel_lookup *l = MPL_malloc(sizeof(*l) + (num_local + num_other) *
                                           sizeof(struct csel_tree_entry), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!l, mpi_errno, MPI_ERR_OTHER, "**nomem");
        csel_lookups[t] = l;
        l->aggregate_mask = aggregate_mask;
        l->num_local = num_local;
        l->num_other = num_other;
        l->local = l->tables;
        l->other = l->tables + num_local;

        int k_local = 0, k_other = 0;
        for (int i = 0; i < n; i++) {
            if (MPII_Csel_tree_is_coll_type(i, t)) {
                struct csel_tree_entry *p = MPII_Csel_tree_is_local(i) ?
                    &l->local[k_local++] : &l->other[k_other++];
                p->tree_idx = i;
                p->mask = MPII_Csel_tree_mask(i);
                p->variant_mask = MPII_Csel_tree_variant_mask(i);
            }
        }
        qsort(l->local, num_local, sizeof(struct csel_tree_entry), compare_trees);
        qsort(l->other, num_other, sizeof(struct csel_tree_entry), compare_trees);
    }

  fn_fail:
    return mpi_errno;
}

/* ---- */

static void add_named_tree(const char *name, const MPII_Csel_json_s * obj);
static void replace_named_tree(int tree_idx, const MPII_Csel_json_s * obj);
static bool is_tree_name_valid(const char *name);
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

/* "dispatch(<mask>)": select the restriction set of the call again, leaving out the
 * restrictions in mask */
static int parse_dispatch(const MPII_Csel_json_s * obj, MPIR_Csel_node_s ** node_out)
{
    const char *hex = obj->u.str + strlen("dispatch(");
    size_t h = strspn(hex, "0123456789abcdef");
    if (h == 0 || h > 8 || hex[0] == '0' || strcmp(hex + h, ")") != 0) {
        return MPII_Csel_json_error(obj, "expect dispatch(<mask>), the restrictions to leave "
                                    "out, nonzero, in lowercase hex without leading zeros");
    }
    unsigned long drop_mask = strtoul(hex, NULL, 16);
    if (drop_mask >> MPIR_CSEL_NUM_RESTRICTIONS) {
        return MPII_Csel_json_error(obj, "dispatch(<mask>) leaves out a bit that is not a "
                                    "restriction");
    }

    MPIR_Csel_node_s *tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);
    tmp->type = CSEL_NODE_TYPE__OPERATOR__DISPATCH;
    tmp->u.dispatch.drop_mask = (unsigned) drop_mask;
    *node_out = tmp;
    return MPI_SUCCESS;
}

static int parse_json_tree(const MPII_Csel_json_s * obj, MPIR_Csel_node_s ** node_out)
{
    int mpi_errno = MPI_SUCCESS;

    /* "error" ends the search without an algorithm */
    if (obj->type == MPII_CSEL_JSON_STRING && strcmp(obj->u.str, "error") == 0) {
        MPIR_Csel_node_s *tmp;
        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);
        tmp->type = CSEL_NODE_TYPE__ERROR;
        *node_out = tmp;
        return MPI_SUCCESS;
    }

    if (obj->type == MPII_CSEL_JSON_STRING && strncmp(obj->u.str, "dispatch(", 9) == 0) {
        return parse_dispatch(obj, node_out);
    }

    if (obj->type != MPII_CSEL_JSON_OBJECT) {
        if (obj->type != MPII_CSEL_JSON_STRING || strncmp(obj->u.str, "call=", 5) != 0) {
            return MPII_Csel_json_error(obj, "expect an object, a \"call=\" string, or "
                                        "\"error\"");
        }

        int tree_idx = MPII_Csel_name_to_idx(obj->u.str + 5);
        if (tree_idx < 0) {
            return MPII_Csel_json_error(obj, "unknown subtree name");
        }
        if (MPII_Csel_tree_is_reserved(tree_idx)) {
            return MPII_Csel_json_error(obj, "call= names a free subtree; a restriction set is "
                                        "reached with dispatch(<mask>)");
        }

        MPIR_Csel_node_s *tmp;
        tmp = MPL_calloc(sizeof(MPIR_Csel_node_s), 1, MPL_MEM_COLL);
        tmp->type = CSEL_NODE_TYPE__OPERATOR__CALL;
        tmp->u.call.tree_idx = tree_idx;
        *node_out = tmp;
        return MPI_SUCCESS;
    }

    if (obj->u.child == NULL) {
        return MPII_Csel_json_error(obj, "expect a condition or an \"algorithm=\" key");
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
            } else if (MPIR_Coll_algo_table[cnt->id] == NULL) {
                mpi_errno = MPII_Csel_json_error(val, "disabled algorithm");
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

        /* "any" ends every list of cases, so a search never falls off the end */
        if (key_is_any(ckey) != (val->next == NULL)) {
            MPL_free(tmp);
            mpi_errno = MPII_Csel_json_error(val, val->next ? "\"any\" must be the last key" :
                                             "expect \"any\" as the last key");
            goto fn_fail;
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

static void csel_subtree_dtor(void *item)
{
    struct csel_subtree *p = item;
    MPIR_Assert(p->json_obj == NULL);
    if (p->node) {
        free_tree(p->node);
        p->node = NULL;
    }
}

/* parse a tuning file: a list of "name=xxx": subtree, the restriction sets and the free subtrees
 * they call */
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

    /* check the names before any tree is replaced */
    MPII_CSEL_JSON_FOREACH(obj, val) {
        if (strncmp(val->key, "name=", 5) != 0) {
            return MPII_Csel_json_error(val, "expect a \"name=\" key: a tuning file defines named "
                                        "subtrees, e.g. \"name=bcast-intra-0\"");
        }
        const char *name = val->key + 5;
        if (strlen(name) >= CSEL_MAX_NAME) {
            return MPII_Csel_json_error(val, "subtree name is too long");
        }
        if (strcmp(name, "auto") == 0) {
            return MPII_Csel_json_error(val, "name=auto is the legacy routing; define the "
                                        "restriction sets, e.g. \"name=bcast-intra-0\"");
        }
        if (strcmp(name, "main") == 0) {
            return MPII_Csel_json_error(val, "there is no main tree: every selection starts at the "
                                        "built-in dispatch, so define the restriction sets, e.g. "
                                        "\"name=bcast-intra-0\"");
        }
        if (!is_tree_name_valid(name)) {
            return MPII_Csel_json_error(val, "malformed restriction set name: the mask is in "
                                        "lowercase hex without leading zeros, and a variant "
                                        "-xxx names a restriction xxx_coll, e.g. "
                                        "bcast-intra-210-gpu");
        }
    }

    MPII_CSEL_JSON_FOREACH(obj, val) {
        const char *name = val->key + 5;

        int tree_idx = MPII_Csel_name_to_idx(name);
        if (tree_idx >= 0) {
            replace_named_tree(tree_idx, val);
        } else {
            add_named_tree(name, val);
        }
    }

    mpi_errno = parse_named_trees();
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPII_Csel_validate();
    MPIR_ERR_CHECK(mpi_errno);

    /* after the validation, which checks the restrictions in the set names */
    mpi_errno = build_lookups();
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

    free_lookups();

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
    } else if (node->type == CSEL_NODE_TYPE__ERROR) {
        printf("ERROR\n");
    } else if (node->type == CSEL_NODE_TYPE__OPERATOR__CALL) {
        printf("call: subtree %d\n", node->u.call.tree_idx);
    } else if (node->type == CSEL_NODE_TYPE__OPERATOR__DISPATCH) {
        printf("dispatch(%x)\n", node->u.dispatch.drop_mask);
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

int MPII_Csel_name_to_idx(const char *name)
{
    struct csel_subtree_hash_item *h;
    HASH_FIND_STR(csel_subtree_hash, name, h);
    if (h) {
        return h->tree_idx;
    } else {
        return -1;
    }
}

MPIR_Csel_node_s *MPII_Csel_get_tree_by_idx(int tree_idx)
{
    struct csel_subtree *p = (void *) utarray_eltptr(csel_subtrees, tree_idx);
    MPIR_Assert(p);
    MPIR_Assert(p->node);
    return p->node;
}

int MPII_Csel_num_trees(void)
{
    return utarray_len(csel_subtrees);
}

const char *MPII_Csel_tree_name(int tree_idx)
{
    struct csel_subtree_hash_item *h, *tmp;
    HASH_ITER(hh, csel_subtree_hash, h, tmp) {
        if (h->tree_idx == tree_idx) {
            return h->name;
        }
    }
    return "?";
}

static const struct csel_set *tree_set(int tree_idx)
{
    struct csel_subtree *p = (void *) utarray_eltptr(csel_subtrees, tree_idx);
    MPIR_Assert(p);
    return &p->set;
}

bool MPII_Csel_tree_is_reserved(int tree_idx)
{
    return tree_set(tree_idx)->coll_type >= 0;
}

bool MPII_Csel_tree_is_coll_type(int tree_idx, int coll_type)
{
    return tree_set(tree_idx)->coll_type == coll_type;
}

/* a local set is where the dispatch first goes when a _local restriction holds; it swaps the
 * buffers or leaves the local restrictions out with dispatch(<mask>) */
bool MPII_Csel_tree_is_local(int tree_idx)
{
    return MPII_Csel_tree_is_reserved(tree_idx) &&
        (tree_set(tree_idx)->mask & MPIR_CSEL_LOCAL_RESTRICTIONS);
}

unsigned MPII_Csel_tree_variant_mask(int tree_idx)
{
    return tree_set(tree_idx)->variant_mask;
}

unsigned MPII_Csel_tree_mask(int tree_idx)
{
    return tree_set(tree_idx)->mask;
}

/* the restriction xxx_coll of a variant -xxx, or -1 */
static int variant_bit(const char *variant, size_t len)
{
    for (int k = 0; k < MPIR_CSEL_NUM_RESTRICTIONS; k++) {
        const char *rname = MPIR_Csel_condition_names[k];
        if ((MPIR_CSEL_COLL_RESTRICTIONS & (1u << k)) && strncmp(rname, variant, len) == 0 &&
            strcmp(rname + len, "_coll") == 0) {
            return k;
        }
    }
    return -1;
}

/* Classify a subtree name. A name of the reserved form, <coll>-<intra|inter>-<hex digits>, alone
 * or followed by "-", is a restriction set, and each "-xxx" after the mask is a variant: a set
 * tuned under the restriction xxx_coll. It is malformed, and parse_set_name returns false, unless
 * the mask is in lowercase hex without leading zeros and the variants name _coll restrictions, in
 * the order of their bits. MPII_Csel_validate checks its restrictions. */
static bool parse_set_name(const char *name, struct csel_set *set)
{
    set->coll_type = -1;
    set->mask = 0;
    set->variant_mask = 0;

    for (int t = 0; t < MPIR_CSEL_NUM_COLL_TYPES; t++) {
        const char *tname = MPIR_Coll_type_names[t];
        size_t n = strlen(tname);
        if (strncmp(name, tname, n) != 0 || name[n] != '-') {
            continue;
        }
        const char *hex = name + n + 1;
        size_t h = strspn(hex, "0123456789abcdefABCDEF");
        const char *rest = hex + h;
        if (h == 0 || (*rest != '\0' && *rest != '-')) {
            /* a free name */
            return true;
        }

        char buf[100];
        unsigned long mask = h <= 8 ? strtoul(hex, NULL, 16) : 0;
        snprintf(buf, sizeof(buf), "%lx", mask);
        if (h > 8 || strlen(buf) != h || strncmp(buf, hex, h) != 0) {
            return false;
        }
        int last = -1;
        while (*rest) {
            rest++;
            size_t len = strcspn(rest, "-");
            int k = variant_bit(rest, len);
            if (k <= last) {
                return false;
            }
            set->variant_mask |= 1u << k;
            last = k;
            rest += len;
        }

        set->coll_type = t;
        set->mask = (unsigned) mask;
        return true;
    }

    return true;
}

/* a malformed restriction set name is an error, so that a typo cannot turn a set into a free
 * subtree that nothing reaches */
static bool is_tree_name_valid(const char *name)
{
    struct csel_set set;
    return parse_set_name(name, &set);
}

static void add_named_tree(const char *name, const MPII_Csel_json_s * obj)
{
    struct csel_set set;
    /* is_tree_name_valid has checked the name */
    parse_set_name(name, &set);
    struct csel_subtree item = { NULL, obj, set };
    utarray_push_back(csel_subtrees, &item, MPL_MEM_COLL);

    /* map the name to tree_idx */
    struct csel_subtree_hash_item *h;
    h = MPL_malloc(sizeof(*h), MPL_MEM_OTHER);
    MPL_strncpy(h->name, name, CSEL_MAX_NAME);
    h->tree_idx = utarray_len(csel_subtrees) - 1;
    HASH_ADD_STR(csel_subtree_hash, name, h, MPL_MEM_COLL);
}

static void replace_named_tree(int tree_idx, const MPII_Csel_json_s * obj)
{
    struct csel_subtree *p = (void *) utarray_eltptr(csel_subtrees, tree_idx);
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

/* compare functions for sorting subtrees in build_lookups() */

static int compare_trees(const void *a, const void *b)
{
    const struct csel_tree_entry *x = a, *y = b;
    int nx = MPL_popcount(x->variant_mask), ny = MPL_popcount(y->variant_mask);
    if (nx != ny) {
        return ny - nx;
    }
    nx = MPL_popcount(x->mask);
    ny = MPL_popcount(y->mask);
    if (nx != ny) {
        return ny - nx;
    }
    if (x->variant_mask != y->variant_mask) {
        return x->variant_mask > y->variant_mask ? -1 : 1;
    }
    if (x->mask != y->mask) {
        return x->mask > y->mask ? -1 : 1;
    }
    return 0;
}
