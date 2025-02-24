/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "utarray.h"
#include "treealgo_types.h"
#include "treeutil.h"
#include "mpiimpl.h"

/* Required declaration*/
static void tree_topology_dump_hierarchy(UT_array hierarchy[], int myrank,
                                         FILE * outfile) ATTRIBUTE((unused));
static void tree_topology_print_heaps_built(heap_vector * minHeaps) ATTRIBUTE((unused));

static int tree_add_child(MPIR_Treealgo_tree_t * t, int rank)
{
    int mpi_errno = MPI_SUCCESS;

    utarray_push_back(t->children, &rank, MPL_MEM_COLL);
    t->num_children++;

    return mpi_errno;
}

/* Routine to calculate log_k of an integer. Specific to tree based calculations */
static int tree_ilog(int k, int number)
{
    int i = 1, p = k - 1;

    for (; p - 1 < number; i++)
        p *= k;

    return i;
}

int MPII_Treeutil_tree_kary_init(int rank, int nranks, int k, int root, MPIR_Treealgo_tree_t * ct)
{
    int lrank, child;
    int mpi_errno = MPI_SUCCESS;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    lrank = (rank + (nranks - root)) % nranks;

    ct->parent = (lrank == 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;

        if (val >= nranks)
            break;

        val = (val + root) % nranks;
        mpi_errno = tree_add_child(ct, val);
        MPIR_ERR_CHECK(mpi_errno);
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Create a special kary tree to build topology-aware tree. The first level of the kary tree has
 * branching factor k0 and the rest levels of the kary tree has branching factor k1.
 * k0 needs to be less than k1.
 */
int MPII_Treeutil_tree_kary_init_topo_aware(int rank, int nranks, int k0, int k1, int root,
                                            MPIR_Treealgo_tree_t * ct)
{
    /* fallback to MPII_Treeutil_tree_kary_init when nranks <= 2 */
    if (nranks <= 2 || k0 >= k1) {
        return MPII_Treeutil_tree_kary_init(rank, nranks, k1, root, ct);
    }
    int lrank, child;
    int mpi_errno = MPI_SUCCESS;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    lrank = (rank + (nranks - root)) % nranks;

    ct->parent = (lrank == 0) ? -1 : (((lrank + k1 - k0 - 1) / k1) + root) % nranks;

    if (lrank == 0) {
        for (child = 1; child <= k0; child++) {
            int val = child;

            if (val >= nranks)
                break;

            val = (val + root) % nranks;
            mpi_errno = tree_add_child(ct, val);
            MPIR_ERR_CHECK(mpi_errno);
        }
    } else {
        for (child = 1; child <= k1; child++) {
            int val = k0 + (lrank - 1) * k1 + child;

            if (val >= nranks)
                break;

            val = (val + root) % nranks;
            mpi_errno = tree_add_child(ct, val);
            MPIR_ERR_CHECK(mpi_errno);
        }

    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Some examples of knomial_1 tree */
/*     4 ranks                8 ranks
 *       0                      0
 *     /  \                 /   |   \
 *    1   3               1     5    7
 *    |                 /   \   |
 *    2                2     4  6
 *                     |
 *                     3
 */
int MPII_Treeutil_tree_knomial_1_init(int rank, int nranks, int k, int root,
                                      MPIR_Treealgo_tree_t * ct)
{
    int lrank, i, j, maxstep, tmp, step, parent, current_rank, running_rank, crank;
    int mpi_errno = MPI_SUCCESS;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    /* maximum number of steps while generating the knomial tree */
    maxstep = 0;
    for (tmp = nranks - 1; tmp; tmp /= k)
        maxstep++;

    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;
    step = 0;
    parent = -1;        /* root has no parent */
    current_rank = 0;   /* start at root of the tree */
    running_rank = current_rank + 1;    /* used for calculation below
                                         * start with first child of the current_rank */

    for (step = 0;; step++) {
        MPIR_Assert(step <= nranks);    /* actually, should not need more steps than log_k(nranks) */

        /* desired rank found */
        if (lrank == current_rank)
            break;

        /* check if rank lies in this range */
        for (j = 1; j < k; j++) {
            if (lrank >= running_rank && lrank < running_rank + MPL_ipow(k, maxstep - step - 1)) {
                /* move to the corresponding subtree */
                parent = current_rank;
                current_rank = running_rank;
                running_rank = current_rank + 1;
                break;
            }

            running_rank += MPL_ipow(k, maxstep - step - 1);
        }
    }

    /* set the parent */
    if (parent == -1)
        ct->parent = -1;
    else
        ct->parent = (parent + root) % nranks;

    /* set the children */
    crank = lrank + 1;  /* crank stands for child rank */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "parent of rank %d is %d, total ranks = %d (root=%d)", rank,
                     ct->parent, nranks, root));
    for (i = step; i < maxstep; i++) {
        for (j = 1; j < k; j++) {
            if (crank < nranks) {
                MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                                (MPL_DBG_FDEST, "adding child %d to rank %d",
                                 (crank + root) % nranks, rank));
                mpi_errno = tree_add_child(ct, (crank + root) % nranks);
                MPIR_ERR_CHECK(mpi_errno);
            }
            crank += MPL_ipow(k, maxstep - i - 1);
        }
    }

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


/* Some examples of knomial_2 tree */
/*     4 ranks               8 ranks
 *       0                      0
 *     /  \                 /   |   \
 *    2    1              4     2    1
 *    |                  / \    |
 *    3                 6   5   3
 *                      |
 *                      7
 */
int MPII_Treeutil_tree_knomial_2_init(int rank, int nranks, int k, int root,
                                      MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;
    int lrank, i, j, depth;
    int *flip_bit, child;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->num_children = 0;
    ct->parent = -1;

    MPIR_Assert(nranks >= 0);
    if (nranks <= 0)
        return mpi_errno;

    lrank = (rank + (nranks - root)) % nranks;
    MPIR_Assert(k >= 2);

    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    /* Parent calculation */
    if (lrank <= 0)
        ct->parent = -1;
    else {
        depth = tree_ilog(k, nranks - 1);

        for (i = 0; i < depth; i++) {
            if (MPL_getdigit(k, lrank, i)) {
                ct->parent = (MPL_setdigit(k, lrank, i, 0) + root) % nranks;
                break;
            }
        }
    }

    /* Children calculation */
    depth = tree_ilog(k, nranks - 1);
    flip_bit = (int *) MPL_calloc(depth, sizeof(int), MPL_MEM_COLL);
    MPIR_ERR_CHKANDJUMP(!flip_bit, mpi_errno, MPI_ERR_OTHER, "**nomem");

    for (j = 0; j < depth; j++) {
        if (MPL_getdigit(k, lrank, j)) {
            break;
        }
        flip_bit[j] = 1;
    }

    for (j = depth - 1; j >= 0; j--) {
        if (flip_bit[j] == 1) {
            for (i = k - 1; i >= 1; i--) {
                child = MPL_setdigit(k, lrank, j, i);
                if (child < nranks)
                    tree_add_child(ct, (child + root) % nranks);
            }
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "parent of rank %d is %d, total ranks = %d (root=%d)", rank,
                     ct->parent, nranks, root));
    MPL_free(flip_bit);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

/* Topology aware */
/* Realizations of help functions for building topology aware */
static int *tree_ut_rank_ensure_fit(UT_array * a, int index)
{
    static const int defval = -1;
    while (index >= utarray_len(a)) {
        utarray_push_back_int(a, &defval, MPL_MEM_COLL);
    }
    return &ut_int_array(a)[index];
}

static int map_coord_to_index(const struct coord_t *coord, UT_array * level, int new_relative_idx)
{
    int i, len = utarray_len(level);
    const struct hierarchy_t *level_arr = ut_type_array(level, struct hierarchy_t *);
    for (i = 0; i < len; ++i) {
        if (memcmp(&level_arr[i].coord, coord, sizeof *coord) == 0)
            return i;
    }
    utarray_extend_back(level, MPL_MEM_COLL);
    memcpy(&tree_ut_hierarchy_back(level)->coord, coord, sizeof *coord);
    tree_ut_hierarchy_back(level)->relative_idx = new_relative_idx;
    return len;
}

static inline void set_level_rank_indices(struct hierarchy_t *level, int r, int myrank, int root,
                                          int idx)
{
    if (r == root) {
        level->root_idx = idx;
        if (level->myrank_idx == idx)
            level->myrank_idx = -1;
    }
    if (r == myrank)
        level->myrank_idx = idx;
}

/*Toplogy tree helpers*/

/* Swap two item in an UT_array with their indexes */
static void swap_idx(UT_array * array, int idx0, int idx1)
{
    int temp = tree_ut_int_elt(array, idx0);
    ut_int_array(array)[idx0] = tree_ut_int_elt(array, idx1);
    ut_int_array(array)[idx1] = temp;
}

/* Sort the array based on num_ranks */
static void sort_with_num_ranks(UT_array * hierarchy, int dim, struct hierarchy_t *level,
                                int start_idx)
{
    MPIR_Assert(dim >= 1);
    for (int idx0 = start_idx; idx0 < utarray_len(&level->sorted_idxs); idx0++) {
        for (int idx1 = idx0 + 1; idx1 < utarray_len(&level->sorted_idxs); idx1++) {
            int sort_idx0 = tree_ut_int_elt(&level->sorted_idxs, idx0);
            int child_idx0 = tree_ut_int_elt(&level->child_idxs, sort_idx0);
            struct hierarchy_t *lower_level0 =
                tree_ut_hierarchy_eltptr(&hierarchy[dim - 1], child_idx0);
            int sort_idx1 = tree_ut_int_elt(&level->sorted_idxs, idx1);
            int child_idx1 = tree_ut_int_elt(&level->child_idxs, sort_idx1);
            struct hierarchy_t *lower_level1 =
                tree_ut_hierarchy_eltptr(&hierarchy[dim - 1], child_idx1);
            if (lower_level0->num_ranks < lower_level1->num_ranks) {
                swap_idx(&level->sorted_idxs, idx0, idx1);
            }
        }
        /* Set up myrank_sorted_idx */
        if (tree_ut_int_elt(&level->sorted_idxs, idx0) == level->myrank_idx) {
            level->myrank_sorted_idx = idx0;
        }
    }
}

/* Setup num_ranks and initial value for myrank_sorted_idx, root_sorted_idx, sorted_idx
 * myrank_sorted_idx = myrank_idx, root_sorted_idx = root_idx, sorted_idx = [0, 1, 2, 3...]
 */
static void MPII_Treeutil_hierarchy_reorder_init(UT_array * hierarchy)
{
    /* To reach world level use 0 for element ptr */
    struct hierarchy_t *wr_level = tree_ut_hierarchy_eltptr(&hierarchy[2], 0);
    MPIR_Assert(wr_level != NULL);
    wr_level->root_sorted_idx = wr_level->root_idx;
    wr_level->myrank_sorted_idx = wr_level->myrank_idx;
    /* Setup num_ranks and and init sorted_idxs */
    for (int gr_idx = 0; gr_idx < utarray_len(&wr_level->child_idxs); gr_idx++) {
        int gr_child_idx = tree_ut_int_elt(&wr_level->child_idxs, gr_idx);
        struct hierarchy_t *gr_level = tree_ut_hierarchy_eltptr(&hierarchy[1], gr_child_idx);
        MPIR_Assert(gr_level != NULL);
        gr_level->root_sorted_idx = gr_level->root_idx;
        gr_level->myrank_sorted_idx = gr_level->myrank_idx;
        for (int sw_idx = 0; sw_idx < utarray_len(&gr_level->ranks); sw_idx++) {
            int sw_child_idx = tree_ut_int_elt(&gr_level->child_idxs, sw_idx);
            struct hierarchy_t *sw_level = tree_ut_hierarchy_eltptr(&hierarchy[0], sw_child_idx);
            sw_level->num_ranks = utarray_len(&sw_level->ranks);
            gr_level->num_ranks += sw_level->num_ranks;
            wr_level->num_ranks += sw_level->num_ranks;
            utarray_push_back_int(&gr_level->sorted_idxs, &sw_idx, MPL_MEM_COLL);
        }
        utarray_push_back_int(&wr_level->sorted_idxs, &gr_idx, MPL_MEM_COLL);
    }
}

/* Reorder the wr_level and gr_level of the hierarchy */
static void MPII_Treeutil_hierarchy_reorder(UT_array * hierarchy, int rank)
{
    struct hierarchy_t *wr_level = tree_ut_hierarchy_eltptr(&hierarchy[2], 0);
    MPIR_Assert(wr_level != NULL);
    /* Switch root to the beginning of sorted_idxs */
    if (wr_level->root_idx != 0) {
        swap_idx(&wr_level->sorted_idxs, 0, wr_level->root_idx);
    }
    /* If the current rank is the root */
    if (wr_level->myrank_idx == wr_level->root_idx) {
        wr_level->myrank_sorted_idx = 0;
    }
    wr_level->root_sorted_idx = 0;
    /* Sort the ranks after the root */
    sort_with_num_ranks(hierarchy, 2, wr_level, 1);
    for (int gr_idx = 0; gr_idx < utarray_len(&wr_level->child_idxs); gr_idx++) {
        int gr_child_idx = tree_ut_int_elt(&wr_level->child_idxs, gr_idx);
        struct hierarchy_t *gr_level = tree_ut_hierarchy_eltptr(&hierarchy[1], gr_child_idx);
        MPIR_Assert(gr_level != NULL);
        if (gr_level->root_idx != 0) {
            swap_idx(&gr_level->sorted_idxs, 0, gr_level->root_idx);
        }
        if (gr_level->myrank_idx == gr_level->root_idx) {
            gr_level->myrank_sorted_idx = 0;
        }
        int sort_start_idx = 0;
        if (gr_level->has_root) {
            sort_start_idx = 1;
        }
        gr_level->root_sorted_idx = 0;
        sort_with_num_ranks(hierarchy, 1, gr_level, sort_start_idx);
    }

    /* Update myrank_idx and myrank_sorted_idx since the ranks array on the wr_level would be
     * modified.
     */
    wr_level->myrank_idx = -1;
    wr_level->myrank_sorted_idx = -1;
    for (int gr_idx = 0; gr_idx < utarray_len(&wr_level->child_idxs); gr_idx++) {
        int gr_child_idx = tree_ut_int_elt(&wr_level->child_idxs, gr_idx);
        struct hierarchy_t *gr_level = tree_ut_hierarchy_eltptr(&hierarchy[1], gr_child_idx);
        int sw_idx = tree_ut_int_elt(&gr_level->sorted_idxs, 0);
        int sw_leader = tree_ut_int_elt(&gr_level->ranks, sw_idx);
        /* Update the group leaders in the wr_level */
        ut_int_array(&wr_level->ranks)[gr_idx] = sw_leader;
        if (rank == sw_leader) {
            wr_level->myrank_idx = gr_idx;
            /* setup myrank_sorted_idx at wr_level */
            for (int gr_sorted_idx = 0; gr_sorted_idx < utarray_len(&wr_level->sorted_idxs);
                 gr_sorted_idx++) {
                if (tree_ut_int_elt(&wr_level->sorted_idxs, gr_sorted_idx) == wr_level->myrank_idx) {
                    wr_level->myrank_sorted_idx = gr_sorted_idx;
                }
            }
        }
        /* Update the root_idx in the gr_level */
        gr_level->root_idx = sw_idx;
    }
}

/* tree init function is for building hierarchy of MPIR_Process::coords_dims */
static int MPII_Treeutil_hierarchy_populate(MPIR_Comm * comm, int rank, int nranks, int root,
                                            bool enable_reorder, UT_array * hierarchy)
{
    int mpi_errno = MPI_SUCCESS;

    /* MPI_Spawn's fallback: If COMM_WORLD's new nranks number
     * is greater than it used to be, it falls back. */
    if (nranks > MPIR_Process.size) {
        goto fn_fail;
    }

    /* Fallback if coords_dims is not 3 or the coords are not ready */
    if (MPIR_Process.coords_dims != 3 || MPIR_Process.coords == NULL) {
        goto fn_fail;
    }

    /* One extra level 'world' is needed */
    MPIR_Assert(MPIR_Process.coords_dims <= MAX_HIERARCHY_DEPTH);

    /* Initialization for world level */
    int dim = MPIR_Process.coords_dims - 1;
    utarray_extend_back(&hierarchy[dim], MPL_MEM_COLL);

    for (int r = 0; r < nranks; ++r) {
        struct hierarchy_t *upper_level =
            tree_ut_hierarchy_back(&hierarchy[MPIR_Process.coords_dims - 1]);
        upper_level->has_root = 1;
        int level_idx = 0;
        int upper_level_root = root;
        MPIR_Assert(upper_level != NULL);

        /* Get wrank from the communicator as the coords are stored with wrank */
        MPIR_Lpid temp = 0;
        MPID_Comm_get_lpid(comm, r, &temp, FALSE);
        int wrank = (int) temp;
        if (wrank < 0)
            goto fn_fail;
        MPIR_Assert(0 <= wrank && wrank < MPIR_Process.size);

        for (dim = MPIR_Process.coords_dims - 2; dim >= 0; --dim) {
            struct coord_t lvl_coord =
                { MPIR_Process.coords[wrank * MPIR_Process.coords_dims + dim + 1], level_idx };

            if (lvl_coord.id < 0)
                goto fn_fail;

            level_idx =
                map_coord_to_index(&lvl_coord, &hierarchy[dim], utarray_len(&upper_level->ranks));
            struct hierarchy_t *cur_level = tree_ut_hierarchy_eltptr(&hierarchy[dim], level_idx);
            MPIR_Assert(cur_level != NULL);

            int *root_ptr = tree_ut_rank_ensure_fit(&upper_level->ranks, cur_level->relative_idx);
            if (r == root || *root_ptr == -1) {
                *root_ptr = r;
                set_level_rank_indices(upper_level, r, rank, upper_level_root,
                                       cur_level->relative_idx);
            }

            if (r == root) {
                cur_level->has_root = 1;
            }

            /* Set up child_idxs */
            int *cur_ptr =
                tree_ut_rank_ensure_fit(&upper_level->child_idxs, cur_level->relative_idx);
            *cur_ptr = level_idx;

            upper_level_root = *root_ptr;
            upper_level = cur_level;
        }

        set_level_rank_indices(upper_level, r, rank, upper_level_root,
                               utarray_len(&upper_level->ranks));
        utarray_push_back_int(&upper_level->ranks, &r, MPL_MEM_COLL);
    }

    MPII_Treeutil_hierarchy_reorder_init(hierarchy);
    if (enable_reorder)
        MPII_Treeutil_hierarchy_reorder(hierarchy, rank);

  fn_exit:
    /* Dump hierarchy for debugging */
    if (MPIR_CVAR_HIERARCHY_DUMP) {
        char outfile_name[PATH_MAX];
        sprintf(outfile_name, "%s%d", "hierarchy", rank);
        FILE *outfile = fopen(outfile_name, "w");
        tree_topology_dump_hierarchy(hierarchy, rank, outfile);
        fclose(outfile);
    }

    return mpi_errno;

  fn_fail:
    mpi_errno = MPI_ERR_TOPOLOGY;
    goto fn_exit;
}

/* Some examples of topology aware tree */
/* configure file content:
 *# rank: group_id switch_id port_number
 *  0        68        0         -1
 *  1        68        0         -1
 *  2        68        1         -1
 *  3        69        0         -1
 *  4        69        1         -1
 *  5        69        1         -1
 *  6        70        1         -1
 *  7        70        0         -1
 *
 * g.id|     68        69       70
 *          /  \      /  \      / \
 * s.id|    0   1    0    1    0   1
 *         / \  |    |   / \   |   |
 * r.id|  0   1 2    3  4   5  7   6
 *
 * [0]leaders: [0, 1, 2] [3, 4, 5] [7, 6] switch
 * [1]leaders:   [1, 2]   [3, 5]   [7, 6] group
 * [2]leaders:     [1]      [3]      [6]  world
 */

/* Implementation of 'Topology aware' algorithm */

/* Important: The initialization of Topology Aware breaks
 * and falls back on the Kary tree building.
 * (1)When it is the MPI_Spawn and (2)not being able to
 * build the hierarchy of the topology-aware tree.
 * For the mentioned cases  see tags 'goto fn_fallback;'. */

int MPII_Treeutil_tree_topology_aware_init(MPIR_Comm * comm, int k, int root, bool enable_reorder,
                                           MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm->rank;
    int nranks = comm->local_size;

    UT_array hierarchy[MAX_HIERARCHY_DEPTH];
    int dim = MPIR_Process.coords_dims - 1;
    for (dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim)
        tree_ut_hierarchy_init(&hierarchy[dim]);

    if (k <= 0 ||
        0 != MPII_Treeutil_hierarchy_populate(comm, rank, nranks, root, enable_reorder, hierarchy))
        goto fn_fallback;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    ct->num_children = 0;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);

    for (dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim) {
        int cur_level_count = utarray_len(&hierarchy[dim]);

        for (int level_idx = 0; level_idx < cur_level_count; ++level_idx) {
            const struct hierarchy_t *level = tree_ut_hierarchy_eltptr(&hierarchy[dim], level_idx);
            if (level->myrank_idx == -1)
                continue;
            MPIR_Assert(level->root_idx != -1);

            MPIR_Treealgo_tree_t tmp_tree;
            if (dim >= 1) {
                mpi_errno =
                    MPII_Treeutil_tree_kary_init(level->myrank_sorted_idx,
                                                 utarray_len(&level->sorted_idxs),
                                                 k, level->root_sorted_idx, &tmp_tree);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                mpi_errno =
                    MPII_Treeutil_tree_kary_init(level->myrank_idx, utarray_len(&level->ranks),
                                                 k, level->root_idx, &tmp_tree);
                MPIR_ERR_CHECK(mpi_errno);
            }

            int children_number = utarray_len(tmp_tree.children);
            int *children = ut_int_array(tmp_tree.children);
            for (int i = 0; i < children_number; ++i) {
                int r = 0;
                if (dim >= 1) {
                    int sorted_idx = tree_ut_int_elt(&level->sorted_idxs, children[i]);
                    r = tree_ut_int_elt(&level->ranks, sorted_idx);
                } else {
                    r = tree_ut_int_elt(&level->ranks, children[i]);
                }
                mpi_errno = tree_add_child(ct, r);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (tmp_tree.parent != -1) {
                MPIR_Assert(ct->parent == -1);
                int parent = 0;
                if (dim >= 1) {
                    int sorted_idx = tree_ut_int_elt(&level->sorted_idxs, tmp_tree.parent);
                    parent = tree_ut_int_elt(&level->ranks, sorted_idx);
                } else {
                    parent = tree_ut_int_elt(&level->ranks, tmp_tree.parent);

                }
                ct->parent = parent;
            }

            MPIR_Treealgo_tree_free(&tmp_tree);
        }
    }

  fn_exit:
    /* free memory */
    for (dim = 0; dim <= MPIR_Process.coords_dims - 1; ++dim)
        utarray_done(&hierarchy[dim]);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

  fn_fallback:
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "due to falling out of the topology-aware initialization, "
                     "it falls back on the kary tree building"));
    mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, 1, root, ct);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

/* Implementation of 'Topology aware' algorithm with the branching factor k */
int MPII_Treeutil_tree_topology_aware_k_init(MPIR_Comm * comm, int k, int root, bool enable_reorder,
                                             MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm->rank;
    int nranks = comm->local_size;

    /* fall back to MPII_Treeutil_tree_topology_aware_init if k is less or equal to 2 */
    if (k <= 2) {
        return MPII_Treeutil_tree_topology_aware_init(comm, k, root, enable_reorder, ct);
    }

    int *num_childrens = NULL;
    num_childrens = (int *) MPL_malloc(sizeof(int) * nranks, MPL_MEM_BUFFER);
    MPIR_Assert(num_childrens != NULL);
    for (int i = 0; i < nranks; i++) {
        num_childrens[i] = 0;
    }

    UT_array hierarchy[MAX_HIERARCHY_DEPTH];
    int dim = MPIR_Process.coords_dims - 1;
    for (dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim)
        tree_ut_hierarchy_init(&hierarchy[dim]);

    if (0 != MPII_Treeutil_hierarchy_populate(comm, rank, nranks, root, enable_reorder, hierarchy))
        goto fn_fallback;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    ct->num_children = 0;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);

    for (dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim) {
        int cur_level_count = utarray_len(&hierarchy[dim]);
        for (int level_idx = 0; level_idx < cur_level_count; ++level_idx) {
            const struct hierarchy_t *level = tree_ut_hierarchy_eltptr(&hierarchy[dim], level_idx);
            if (level->myrank_idx == -1)
                continue;
            MPIR_Assert(level->root_idx != -1);

            MPIR_Treealgo_tree_t tmp_tree;
            if (dim == 2) {
                /* group level - build a tree on the sorted_idxs */
                mpi_errno =
                    MPII_Treeutil_tree_kary_init(level->myrank_sorted_idx,
                                                 utarray_len(&level->sorted_idxs),
                                                 k - 2, level->root_sorted_idx, &tmp_tree);
                MPIR_ERR_CHECK(mpi_errno);
            } else if (dim == 1) {
                /* switch level - build a tree on the sorted_idxs */
                mpi_errno =
                    MPII_Treeutil_tree_kary_init_topo_aware(level->myrank_sorted_idx,
                                                            utarray_len(&level->sorted_idxs),
                                                            1,
                                                            k - 1,
                                                            level->root_sorted_idx, &tmp_tree);
                MPIR_ERR_CHECK(mpi_errno);
            } else {
                /* rank level - build a tree on the ranks */
                /* Do an allgather to know the current num_children on each rank */
                MPIR_Errflag_t errflag = MPIR_ERR_NONE;
                MPIR_Allgather_impl(&(ct->num_children), 1, MPI_INT, num_childrens, 1, MPI_INT,
                                    comm, errflag);
                if (mpi_errno) {
                    goto fn_fail;
                }
                int switch_leader = tree_ut_int_elt(&level->ranks, level->root_idx);
                mpi_errno =
                    MPII_Treeutil_tree_kary_init_topo_aware(level->myrank_idx,
                                                            utarray_len(&level->ranks),
                                                            k - num_childrens[switch_leader], k,
                                                            level->root_idx, &tmp_tree);
                MPIR_ERR_CHECK(mpi_errno);
            }

            int children_number = utarray_len(tmp_tree.children);
            int *children = ut_int_array(tmp_tree.children);
            for (int i = 0; i < children_number; ++i) {
                int r = 0;
                if (dim >= 1) {
                    int sorted_idx = tree_ut_int_elt(&level->sorted_idxs, children[i]);
                    r = tree_ut_int_elt(&level->ranks, sorted_idx);
                } else {
                    r = tree_ut_int_elt(&level->ranks, children[i]);
                }
                mpi_errno = tree_add_child(ct, r);
                MPIR_ERR_CHECK(mpi_errno);
            }

            if (tmp_tree.parent != -1) {
                MPIR_Assert(ct->parent == -1);
                int parent = 0;
                if (dim >= 1) {
                    int sorted_idx = tree_ut_int_elt(&level->sorted_idxs, tmp_tree.parent);
                    parent = tree_ut_int_elt(&level->ranks, sorted_idx);
                } else {
                    parent = tree_ut_int_elt(&level->ranks, tmp_tree.parent);

                }
                ct->parent = parent;
            }

            MPIR_Treealgo_tree_free(&tmp_tree);
        }
    }

  fn_exit:
    /* free memory */
    for (dim = 0; dim <= MPIR_Process.coords_dims - 1; ++dim)
        utarray_done(&hierarchy[dim]);
    if (num_childrens != NULL) {
        MPL_free(num_childrens);
        num_childrens = NULL;
    }

    return mpi_errno;

  fn_fail:
    goto fn_exit;

  fn_fallback:
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "due to falling out of the topology-aware initialization, "
                     "it falls back on the kary tree building"));
    mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, 1, root, ct);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;
}

/* Implementation of 'Topology wave' algorithm */

/* Implementation of a min-heap */
static minHeap *initMinHeap(void)
{
    minHeap *hp = MPL_malloc(sizeof(minHeap) * 1, MPL_MEM_BUFFER);
    if (hp)
        hp->size = 0;
    return hp;
}

static void swap(pair * n1, pair * n2)
{
    pair temp = *n1;
    *n1 = *n2;
    *n2 = temp;
}

static void heapify(minHeap * hp, int i)
{
    int smallest = (LCHILD(i) < hp->size &&
                    hp->elem[LCHILD(i)].reach_time < hp->elem[i].reach_time) ? LCHILD(i) : i;
    if (RCHILD(i) < hp->size && hp->elem[RCHILD(i)].reach_time < hp->elem[smallest].reach_time) {
        smallest = RCHILD(i);
    }
    if (smallest != i) {
        swap(&(hp->elem[i]), &(hp->elem[smallest]));
        heapify(hp, smallest);
    }
}

static void insertNode(minHeap * hp, pair * data)
{
    if (hp->size) {
        hp->elem = MPL_realloc(hp->elem, (hp->size + 1) * sizeof(pair), MPL_MEM_BUFFER);
    } else {
        hp->elem = MPL_malloc(sizeof(pair) * (hp->size + 1), MPL_MEM_BUFFER);
    }
    MPIR_Assert(hp->elem != NULL);
    pair nd;
    nd.rank = data->rank;
    nd.reach_time = data->reach_time;

    int i = (hp->size)++;
    while (i && nd.reach_time < hp->elem[PARENT(i)].reach_time) {
        hp->elem[i] = hp->elem[PARENT(i)];
        i = PARENT(i);
    }
    hp->elem[i] = nd;
}

static void deleteMinHeap(minHeap * hp)
{
    MPL_free(hp->elem);
}

/* Implementation of a vector for min-heaps */
static void heap_vector_init(heap_vector * v)
{
    v->capacity = VECTOR_INIT_CAPACITY;
    v->total = 0;
    v->heap = MPL_malloc(sizeof(minHeap) * v->capacity, MPL_MEM_BUFFER);
}

static void heap_vector_resize(heap_vector * v, int capacity)
{
    minHeap *heap = MPL_realloc(v->heap, sizeof(minHeap) * capacity, MPL_MEM_BUFFER);
    if (heap) {
        v->heap = heap;
        v->capacity = capacity;
    }
}

static void heap_vector_add(heap_vector * v, minHeap * item)
{
    if (v->capacity == v->total)
        heap_vector_resize(v, v->capacity * 2);
    v->heap[v->total].elem = item->elem;
    v->heap[v->total].size = item->size;
    v->total++;
}

static void heap_vector_free(heap_vector * v)
{
    MPL_free(v->heap);
}

static int latency(int unv_rank, int v_rank, int lat_diff_groups, int lat_diff_switches,
                   int lat_same_switches)
{
    /* latency of but different groups */
    if (MPIR_Process.coords[unv_rank * MPIR_Process.coords_dims + 2] !=
        MPIR_Process.coords[v_rank * MPIR_Process.coords_dims + 2])
        return lat_diff_groups;

    /* latency of the same groups, but different switch */
    if (MPIR_Process.coords[unv_rank * MPIR_Process.coords_dims + 1] !=
        MPIR_Process.coords[v_rank * MPIR_Process.coords_dims + 1])
        return lat_diff_switches;

    /* latency of the same switch */
    return lat_same_switches;
}

static inline void take_children(const UT_array * hierarchy, int lead, UT_array * unv_set)
{
    /* take_children() finds children of the leader in the switch */
    struct hierarchy_t *sw_level = tree_ut_hierarchy_eltptr(&hierarchy[0], lead);
    MPIR_Assert(sw_level != NULL);
    for (int r = 0; r < utarray_len(&sw_level->ranks); r++) {
        if (r == 0) {
            /* Skip the lead of the switch because
             * it's already found and added */
            continue;
        }
        pair p;
        p.rank = tree_ut_int_elt(&sw_level->ranks, r);
        p.reach_time = -1;
        utarray_push_back(unv_set, &p, MPL_MEM_COLL);
    }
}

static int find_leader(const UT_array * hierarchy, UT_array * unv_set, int root_gr_sorted_idx,
                       int root_sw_sorted_idx, int *group_offset, int *switch_offset,
                       heap_vector * minHeaps)
{
    /* find_leader() finds a switch leader and its children, filling array of heaps
     * and unvisited set. Maintain two vars 'group_offset' and 'switch_offset', they saves
     * relative coordinates, where the search of a leader stops, and continue the next call of
     * find_leader() with the saved coordinates. */

    /* To reach world level use 0 for element ptr */
    struct hierarchy_t *wr_level = tree_ut_hierarchy_eltptr(&hierarchy[2], 0);
    MPIR_Assert(wr_level != NULL);
    for (int gr_sorted_offset = *group_offset;
         gr_sorted_offset < utarray_len(&wr_level->sorted_idxs); gr_sorted_offset++) {
        int gr_sorted_idx =
            (root_gr_sorted_idx + gr_sorted_offset) % utarray_len(&wr_level->sorted_idxs);
        int gr_idx = tree_ut_int_elt(&wr_level->sorted_idxs, gr_sorted_idx);
        int gr_child_idx = tree_ut_int_elt(&wr_level->child_idxs, gr_idx);
        struct hierarchy_t *gr_level = tree_ut_hierarchy_eltptr(&hierarchy[1], gr_child_idx);
        MPIR_Assert(gr_level != NULL);
        for (int sw_sorted_offset = *switch_offset;
             sw_sorted_offset < utarray_len(&gr_level->sorted_idxs); sw_sorted_offset++) {
            int sw_sorted_idx =
                (root_sw_sorted_idx + sw_sorted_offset) % utarray_len(&gr_level->sorted_idxs);
            int sw_idx = tree_ut_int_elt(&gr_level->sorted_idxs, sw_sorted_idx);
            int sw_child_idx = tree_ut_int_elt(&gr_level->child_idxs, sw_idx);
            struct hierarchy_t *sw_level = tree_ut_hierarchy_eltptr(&hierarchy[0], sw_child_idx);
            if (!sw_level->has_root) {
                int cur_rank = tree_ut_int_elt(&gr_level->ranks, sw_idx);
                pair p;
                p.rank = cur_rank;
                p.reach_time = -1;
                utarray_push_back(unv_set, &p, MPL_MEM_COLL);

                minHeap *heap = initMinHeap();
                heap_vector_add(minHeaps, heap);
                MPL_free(heap);

                /* Take children of the lead in the switch */
                take_children(hierarchy, sw_child_idx, unv_set);

                /* When there is not any switch inside the current group,
                 * shift to the next group and start with the 1st switch */
                if (utarray_len(&gr_level->sorted_idxs) == sw_sorted_offset) {
                    *group_offset = gr_sorted_offset + 1;
                    *switch_offset = 0;
                } else {
                    /* If there're more switches in the current group,
                     * shift to the next switch and keep the current group id */
                    *group_offset = gr_sorted_offset;
                    *switch_offset = sw_sorted_offset + 1;
                }
                return 0;
            }
        }
        *switch_offset = 0;
    }
    *group_offset = 0;
    return -1;
}

static inline void take_earliest_time(UT_array * unvisited_set, const heap_vector * minHeaps,
                                      int *glob_reach_time, const int overhead,
                                      const int lat_diff_groups, const int lat_diff_switches,
                                      const int lat_same_switches, const int unvisited_node_idx,
                                      const int visited_sw_idx, int *best_v_sw_idx,
                                      int *best_u_nd_idx)
{
    MPIR_Assert(unvisited_node_idx < utarray_len(unvisited_set));
    if (minHeaps->heap[visited_sw_idx].size == 0)
        return;
    int new_reach_time = minHeaps->heap[visited_sw_idx].elem->reach_time +
        latency(pair_elt(unvisited_set, unvisited_node_idx)->rank,
                minHeaps->heap[visited_sw_idx].elem->rank, lat_diff_groups, lat_diff_switches,
                lat_same_switches) + 2 * overhead;
    if (pair_elt(unvisited_set, unvisited_node_idx)->reach_time == -1 ||
        pair_elt(unvisited_set, unvisited_node_idx)->reach_time > new_reach_time) {
        pair_elt(unvisited_set, unvisited_node_idx)->reach_time = new_reach_time;
    }
    if (*glob_reach_time > pair_elt(unvisited_set, unvisited_node_idx)->reach_time) {
        /* Update global reach time by current reach time of node */
        *glob_reach_time = pair_elt(unvisited_set, unvisited_node_idx)->reach_time;
        /* This is best current time, save indices */
        *best_v_sw_idx = visited_sw_idx;
        *best_u_nd_idx = unvisited_node_idx;
    }
}

static int init_root_switch(const UT_array * hierarchy, heap_vector * minHeaps, UT_array * unv_set,
                            const int root, int *root_gr_sorted_idx, int *root_sw_sorted_idx)
{
    /* init_root_switch() adds the root's switch in a heap vector (array of heaps).
     * Originally, inserts ROOT in the first created heap, then looks for ROOT's
     * children and also inserts them into the heap, push its children into the
     * unvisited set. If there are not ROOT's children, it falls back. */

    int mpi_errno = MPI_SUCCESS;
    /* Create the first heap for ROOT */
    minHeap *init_heap = initMinHeap();
    MPIR_ERR_CHKANDJUMP(!init_heap, mpi_errno, MPI_ERR_OTHER, "**nomem");
    /* Get the root's switch */
    struct hierarchy_t *wr_level = tree_ut_hierarchy_eltptr(&hierarchy[2], 0);
    struct hierarchy_t *gr_level = NULL;
    struct hierarchy_t *sw_level = NULL;
    MPIR_Assert(wr_level != NULL);
    bool break_now = false;
    for (int gr_idx = 0; gr_idx < utarray_len(&wr_level->child_idxs); gr_idx++) {
        if (break_now) {
            break;
        }
        int gr_child_idx = tree_ut_int_elt(&wr_level->child_idxs, gr_idx);
        gr_level = tree_ut_hierarchy_eltptr(&hierarchy[1], gr_child_idx);
        MPIR_Assert(gr_level != NULL);
        if (gr_level->has_root) {
            for (int sw_idx = 0; sw_idx < utarray_len(&gr_level->child_idxs); sw_idx++) {
                int sw_child_idx = tree_ut_int_elt(&gr_level->child_idxs, sw_idx);
                sw_level = tree_ut_hierarchy_eltptr(&hierarchy[0], sw_child_idx);
                if (sw_level->has_root) {
                    *root_gr_sorted_idx = wr_level->root_sorted_idx;
                    *root_sw_sorted_idx = gr_level->root_sorted_idx;
                    break_now = true;
                    break;
                }
            }
        }
    }

    /* Insert the ROOT's pair into the first heap of switches */
    pair p;
    p.rank = root;
    p.reach_time = 0;
    insertNode(init_heap, &p);

    /* Check if there are children (except ROOT) */
    if (utarray_len(&sw_level->ranks) > 1) {
        /*If yes, fill the ROOT's heap by children and push them into unvisited set */
        for (int r = 0; r < utarray_len(&sw_level->ranks); r++) {
            int cur_rank = tree_ut_int_elt(&sw_level->ranks, r);
            if (cur_rank == root)
                continue;
            pair p_ch;
            p_ch.rank = cur_rank;
            p_ch.reach_time = -1;
            utarray_push_back(unv_set, &p_ch, MPL_MEM_COLL);
        }
        /* Add the ROOT's filled heap into vector of heaps */
        heap_vector_add(minHeaps, init_heap);
    } else {    /* If no, it falls back */
        deleteMinHeap(init_heap);
        goto fn_fail;
    }

  fn_exit:
    if (init_heap)
        MPL_free(init_heap);
    return mpi_errno;
  fn_fail:
    mpi_errno =
        MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, __func__,
                             __LINE__, MPI_ERR_TOPOLOGY, "**nomem", 0);
    goto fn_exit;
}

/* 'Topology Wave' implementation */
int MPII_Treeutil_tree_topology_wave_init(MPIR_Comm * comm, int k, int root, bool enable_reorder,
                                          int overhead, int lat_diff_groups, int lat_diff_switches,
                                          int lat_same_switches, MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;
    int rank = comm->rank;
    int nranks = comm->local_size;
    int root_gr_sorted_idx = 0;
    int root_sw_sorted_idx = 0;
    int group_offset = 0;
    int switch_offset = 0;
    int size_comm = 0;
    UT_array hierarchy[MAX_HIERARCHY_DEPTH];
    UT_array *unv_set = NULL;

    heap_vector minHeaps;
    heap_vector_init(&minHeaps);

    /* To build hierarchy of ranks, swiches and groups */
    int dim = MPIR_Process.coords_dims - 1;
    for (dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim)
        tree_ut_hierarchy_init(&hierarchy[dim]);

    if (overhead <= 0 || lat_diff_groups <= 0 || lat_diff_switches <= 0 || lat_same_switches <= 0 ||
        0 != MPII_Treeutil_hierarchy_populate(comm, rank, nranks, root, enable_reorder, hierarchy))
        goto fn_fallback;

    UT_icd intpair_icd = { sizeof(pair), NULL, NULL, NULL };
    utarray_new(unv_set, &intpair_icd, MPL_MEM_COLL);

    if (init_root_switch
        (hierarchy, &minHeaps, unv_set, root, &root_gr_sorted_idx, &root_sw_sorted_idx))
        goto fn_fallback;

    ct->rank = rank;
    ct->nranks = nranks;
    ct->parent = -1;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    ct->num_children = 0;

    MPIR_Assert(nranks >= 0);

    if (nranks == 0)
        goto fn_exit;

    while (size_comm != nranks) {
        size_comm++;
        /* Index of best unvisited node */
        int best_u_nd_idx = -1;
        /* Index of best visited switch */
        int best_v_sw_idx = -1;
        /* Global reach time */
        int glob_reach_time = INT_MAX;
        /* For every node u in unvisited set */
        for (int i = 0; i < utarray_len(unv_set); i++) {
            pair_elt(unv_set, i)->reach_time = INT_MAX;
            /* For every visited switch */
            for (int j = 0; j < minHeaps.total; j++) {
                take_earliest_time(unv_set, &minHeaps, &glob_reach_time, overhead,
                                   lat_diff_groups, lat_diff_switches, lat_same_switches, i, j,
                                   &best_v_sw_idx, &best_u_nd_idx);
            }
        }

        MPIR_Assert(best_u_nd_idx < utarray_len(unv_set));
        /* Connect the rank with min reach_time from best_v_sw_idx to best_u_nd_idx */
        if (rank == pair_elt(unv_set, best_u_nd_idx)->rank) {
            MPIR_Assert(ct->parent == -1);
            ct->parent = minHeaps.heap[best_v_sw_idx].elem->rank;
        } else if (rank == minHeaps.heap[best_v_sw_idx].elem->rank) {
            mpi_errno = tree_add_child(ct, pair_elt(unv_set, best_u_nd_idx)->rank);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Update reach_time of visited node, adding overhead,
         * and heapify the main heap with the updated values */
        minHeaps.heap[best_v_sw_idx].elem->reach_time += overhead;
        heapify(&minHeaps.heap[best_v_sw_idx], 0);

        /* Update reach_time of unvisited node, then insert it into the current heap */
        pair_elt(unv_set, best_u_nd_idx)->reach_time = glob_reach_time;
        insertNode(&minHeaps.heap[minHeaps.total - 1], pair_elt(unv_set, best_u_nd_idx));

        /* Remove best_u_nd_idx from the list of unvisited list
         * and reduce size of unvisited set */
        utarray_erase(unv_set, best_u_nd_idx, 1);

        if (utarray_len(unv_set) == 0) {
            /* Find switch leaders and keep updating coordinates of
             * group/switch idx'es for next search */
            int ret = find_leader(hierarchy, unv_set, root_gr_sorted_idx, root_sw_sorted_idx,
                                  &group_offset, &switch_offset, &minHeaps);
            if (ret == -1 && utarray_len(unv_set) == 0) {
                /* If there is not any element to be added break the main while () */
                break;
            }
        }
    }

  fn_exit:
    /* free memory */
    for (dim = 0; dim <= MPIR_Process.coords_dims - 1; ++dim)
        utarray_done(&hierarchy[dim]);
    utarray_free(unv_set);
    for (int i = 0; i < minHeaps.total; i++)
        deleteMinHeap(&minHeaps.heap[i]);
    heap_vector_free(&minHeaps);
    return mpi_errno;

  fn_fail:
    goto fn_exit;

  fn_fallback:
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "due to falling out of the topology-aware initialization, "
                     "it falls back on the kary tree building"));
    mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, k, root, ct);
    MPIR_ERR_CHECK(mpi_errno);
    goto fn_exit;

}

static void tree_topology_dump_hierarchy(UT_array hierarchy[], int myrank, FILE * outfile)
{
    fprintf(outfile, "{\"rank\": %d, \"hierarchy\": [\n", myrank);
    for (int dim = MPIR_Process.coords_dims - 1; dim >= 0; --dim) {
        fprintf(outfile, "    {\"dim\": %d, \"levels\": [\n", dim);
        for (int level_idx = 0; level_idx < utarray_len(&hierarchy[dim]); ++level_idx) {
            if (level_idx > 0)
                fprintf(outfile, ",\n");
            struct hierarchy_t *cur_level = tree_ut_hierarchy_eltptr(&hierarchy[dim], level_idx);
            fprintf(outfile, "        {\"coord\": {\"id\": %d, \"parent_idx\": %d}, ",
                    cur_level->coord.id, cur_level->coord.parent_idx);
            fprintf(outfile,
                    "\"relative_idx\": %d, \"has_root\": %d, \"root_idx\": %d, \"myrank_idx\": %d,"
                    " \"num_ranks\": %d, \"root_sorted_idx\": %d, \"myrank_sorted_idx\": %d,"
                    " \"ranks\": [",
                    cur_level->relative_idx, cur_level->has_root, cur_level->root_idx,
                    cur_level->myrank_idx, cur_level->num_ranks, cur_level->root_sorted_idx,
                    cur_level->myrank_sorted_idx);
            for (int i = 0; i < utarray_len(&cur_level->ranks); ++i) {
                if (i > 0)
                    fprintf(outfile, ", ");
                fprintf(outfile, "%d", tree_ut_int_elt(&cur_level->ranks, i));
            }
            fprintf(outfile, "], \"child_idxs\": [");
            for (int i = 0; i < utarray_len(&cur_level->child_idxs); ++i) {
                if (i > 0)
                    fprintf(outfile, ", ");
                fprintf(outfile, "%d", tree_ut_int_elt(&cur_level->child_idxs, i));
            }
            fprintf(outfile, "], \"sorted_idxs\": [");
            for (int i = 0; i < utarray_len(&cur_level->sorted_idxs); ++i) {
                if (i > 0)
                    fprintf(outfile, ", ");
                fprintf(outfile, "%d", tree_ut_int_elt(&cur_level->sorted_idxs, i));
            }
            fprintf(outfile, "]}");
        }
        fprintf(outfile, "\n     ]}");
        if (dim > 0)
            fprintf(outfile, ",");
        fprintf(outfile, "\n");
    }
    fprintf(outfile, "]}\n");
}

static void tree_topology_print_heaps_built(heap_vector * minHeaps)
{
    fprintf(stderr, "total of array heap: %d\n", minHeaps->total);
    for (int i = 0; i < minHeaps->total; i++) {
        for (int j = 0; j < minHeaps->heap[i].size; j++) {
            fprintf(stderr, "heap[%d]: %d %d\n", i, minHeaps->heap[i].elem[j].rank,
                    minHeaps->heap[i].elem[j].reach_time);
        }
        fprintf(stderr, "\n");
    }
}
