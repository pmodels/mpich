/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"
#include "topotree_types.h"
#include "topotree_util.h"
#include "topotree.h"

#include <math.h>

#define MAX_TOPO_DEPTH 7

static int create_template_tree(MPIDI_SHM_topotree_t * template_tree, int k_val,
                                bool right_skewed, int max_ranks, MPIR_Errflag_t * eflag);

static void copy_tree(int *shared_region, int num_ranks, int rank,
                      MPIR_Treealgo_tree_t * my_tree, int *topotree_fail);

static int topotree_get_package_level(int *num_packages, int num_ranks, int *bind_map);

static void gen_package_tree(int num_packages, int k_val, MPIDI_SHM_topotree_t * package_tree,
                             int *package_leaders);

static void gen_tree_sharedmemory(int *shared_region, MPIDI_SHM_topotree_t * tree,
                                  MPIDI_SHM_topotree_t * package_tree, int *package_leaders,
                                  int num_packages, int num_ranks, int k_val,
                                  bool package_leaders_first);

static int gen_tree(int k_val, int *shared_region, int num_packages,
                    int **ranks_per_package, int max_ranks_per_package, int *package_ctr,
                    int package_level, int num_ranks, bool package_leaders_first,
                    bool right_skewed, MPIR_Errflag_t * eflag);

/* This function allocates and generates a template_tree based on k_val and right_skewed for
 * max_ranks.
 * */
static int create_template_tree(MPIDI_SHM_topotree_t * template_tree, int k_val,
                                bool right_skewed, int max_ranks, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int child_id, child_idx;

    mpi_errno = MPIDI_SHM_topotree_allocate(template_tree, max_ranks, k_val);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    for (int i = 0; i < max_ranks; ++i) {
        MPIDI_SHM_TOPOTREE_PARENT(template_tree, i) = ceilf(i / (float) (k_val)) - 1;
        MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i) = 0;
        if (!right_skewed) {
            for (int j = 0; j < k_val; ++j) {
                child_id = i * k_val + 1 + j;
                if (child_id < max_ranks) {
                    child_idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i)++;
                    MPIDI_SHM_TOPOTREE_CHILD(template_tree, i, child_idx) = child_id;
                }
            }
        } else if (right_skewed) {
            for (int j = k_val - 1; j >= 0; --j) {
                child_id = i * k_val + 1 + j;
                if (child_id < max_ranks) {
                    child_idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i)++;
                    MPIDI_SHM_TOPOTREE_CHILD(template_tree, i, child_idx) = child_id;
                }
            }
        }
    }
    MPIDI_SHM_TOPOTREE_PARENT(template_tree, 0) = -1;

    /* template tree is ready here */
    return mpi_errno;
}

/* This function copies the tree present in shared_region into the my_tree data structure for rank.
 * Doesn't perform any direct allocation, but utarray_new is called to allocate space for children
 * in my_tree.
 * */
static void copy_tree(int *shared_region, int num_ranks, int rank,
                      MPIR_Treealgo_tree_t * my_tree, int *topotree_fail)
{
    int *parent_ptr = shared_region;
    int *child_ctr = &shared_region[num_ranks];
    int *children = &shared_region[num_ranks + num_ranks + 1];
    int parent = parent_ptr[rank];
    int num_children = child_ctr[rank + 1] - child_ctr[rank];
    int *my_children = &children[child_ctr[rank]];

    *topotree_fail = 0;
    my_tree->parent = parent;
    my_tree->num_children = 0;
    my_tree->rank = rank;
    my_tree->nranks = num_ranks;
    utarray_new(my_tree->children, &ut_int_icd, MPL_MEM_COLL);
    utarray_reserve(my_tree->children, num_children, MPL_MEM_COLL);
    for (int c = 0; c < num_children; ++c) {
        utarray_push_back(my_tree->children, &my_children[c], MPL_MEM_COLL);
        if (my_children[c] == 0) {
            *topotree_fail = 1;
        }
        my_tree->num_children++;
    }
}

/* This function returns the topology level where we will break the tree into a package_leaders
 * and a per_package tree. If needed MPIDI_SHM_TOPOTREE_CUTOFF can be modified to the level where this cutoff
 * should happen. Note, this function also output num_packages at the package_level
 * functions.
 * */
int topotree_get_package_level(int *num_packages, int num_ranks, int *bind_map)
{
    int package_level;
    int max_entries_per_level[MAX_TOPO_DEPTH];

    for (int lvl = 0; lvl < MAX_TOPO_DEPTH; ++lvl) {
        int max = -1;
        for (int i = 0; i < num_ranks; ++i) {
            max = MPL_MAX(max, bind_map[i * MAX_TOPO_DEPTH + lvl] + 1);
        }
        max_entries_per_level[lvl] = max;
    }
    /* set package_level to first level that is more than one entries, or leave at level 0 */
    package_level = 0;
    for (int i = 0; i < MAX_TOPO_DEPTH; i++) {
        if (max_entries_per_level[i] > 1) {
            package_level = i;
            break;
        }
    }
    if (MPIDI_SHM_TOPOTREE_CUTOFF >= 0) {
        package_level = MPIDI_SHM_TOPOTREE_CUTOFF;
    }
    *num_packages = max_entries_per_level[package_level];
    return package_level;
}

/* This function generates a package level tree using the package leaders and k_val.
 * */
void gen_package_tree(int num_packages, int k_val, MPIDI_SHM_topotree_t * package_tree,
                      int *package_leaders)
{
    int parent_idx, child_id, idx;
    /* Generate a package (top-level) tree */
    for (int i = 0; i < num_packages; ++i) {
        if (i == 0) {
            MPIDI_SHM_TOPOTREE_PARENT(package_tree, i) = -1;
        } else {
            parent_idx = floor((i - 1) / (float) (k_val));
            MPIDI_SHM_TOPOTREE_PARENT(package_tree, i) = package_leaders[parent_idx];
        }
        MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, i) = 0;
        for (int j = 0; j < k_val; ++j) {
            child_id = i * k_val + 1 + j;
            if (child_id < num_packages && package_leaders[child_id] != -1) {
                idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, i)++;
                MPIDI_SHM_TOPOTREE_CHILD(package_tree, i, idx) = package_leaders[child_id];
            }
        }
    }
}

/* This function assembles the package leaders tree and per package tree into a single tree in
 * shared memory.
 * */
static void gen_tree_sharedmemory(int *shared_region, MPIDI_SHM_topotree_t * tree,
                                  MPIDI_SHM_topotree_t * package_tree, int *package_leaders,
                                  int num_packages, int num_ranks, int k_val,
                                  bool package_leaders_first)
{
    int is_package_leader;
    int *parent_ptr = shared_region;
    int *child_ctr = &shared_region[num_ranks];
    int *children = &shared_region[num_ranks + num_ranks + 1];
    memset(shared_region, 0, sizeof(int) * (3 * num_ranks) + 1);
    child_ctr[0] = 0;

    for (int i = 0; i < num_ranks; ++i) {
        parent_ptr[i] = i == 0 ? -1 : MPIDI_SHM_TOPOTREE_PARENT(tree, i);
        child_ctr[i + 1] = child_ctr[i];
        /* Package last as 0 means package leaders are added first before adding the per-package
         * tree */
        if (package_leaders_first) {
            /* Add children for package leaders */
            is_package_leader = -1;
            for (int pm = 0; pm < num_packages; ++pm) {
                if (i == package_leaders[pm]) {
                    is_package_leader = pm;
                }
            }
            if (is_package_leader != -1) {
                for (int j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, is_package_leader);
                     ++j) {
                    children[child_ctr[i + 1]] =
                        MPIDI_SHM_TOPOTREE_CHILD(package_tree, is_package_leader, j);
                    child_ctr[i + 1]++;
                }
                parent_ptr[i] = MPIDI_SHM_TOPOTREE_PARENT(package_tree, is_package_leader);
            }
        }

        for (int j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(tree, i); ++j) {
            children[child_ctr[i + 1]] = MPIDI_SHM_TOPOTREE_CHILD(tree, i, j);
            child_ctr[i + 1]++;
        }

        /* Package last as non-zero means package leaders are added after adding the per-package
         * tree */
        if (!package_leaders_first) {
            /* Add children for package leaders */
            is_package_leader = -1;
            for (int pm = 0; pm < num_packages; ++pm) {
                if (i == package_leaders[pm]) {
                    is_package_leader = pm;
                }
            }
            if (is_package_leader != -1) {
                for (int j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, is_package_leader);
                     ++j) {
                    children[child_ctr[i + 1]] =
                        MPIDI_SHM_TOPOTREE_CHILD(package_tree, is_package_leader, j);
                    child_ctr[i + 1]++;
                }
                parent_ptr[i] = MPIDI_SHM_TOPOTREE_PARENT(package_tree, is_package_leader);
            }
        }
    }
}

/* This is the main function which generates a tree in shared memory. The tree is parameterized
 * over the different data-structures:
 * k_val : the tree K-value
 * shared_region : the shared memory region where the tree will be generated
 * num_packages : the maximum number of packages at package_level
 * ranks_per_package : the different ranks at each level
 * max_ranks_per_package : the maximum ranks in any package
 * package_ctr : number of ranks in each package
 * package_level : the topology level where we cutoff the tree
 * num_ranks : the number of ranks
 * */
static int gen_tree(int k_val, int *shared_region, int num_packages,
                    int **ranks_per_package, int max_ranks_per_package, int *package_ctr,
                    int package_level, int num_ranks, bool package_leaders_first,
                    bool right_skewed, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int rank, idx;
    int package_count = 0;
    MPIDI_SHM_topotree_t package_tree, tree, template_tree;
    const int package_tree_sz = num_packages > num_ranks ? num_packages : num_ranks;
    int *package_leaders = NULL;

    MPIR_CHKPMEM_DECL(1);

    mpi_errno = MPIDI_SHM_topotree_allocate(&tree, num_ranks, k_val);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    mpi_errno = MPIDI_SHM_topotree_allocate(&package_tree, package_tree_sz, k_val);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    MPIR_CHKPMEM_CALLOC(package_leaders, int *, num_packages * sizeof(int), mpi_errno,
                        "intra_node_package_leaders", MPL_MEM_OTHER);

    /* We pick package leaders as the first rank in each package */
    for (int p = 0; p < num_packages; ++p) {
        package_leaders[p] = -1;
        if (package_ctr[p] > 0) {
            package_leaders[package_count++] = ranks_per_package[p][0];
        }
    }
    num_packages = package_count;

    /* STEP 4. Now use the template tree to generate the top level tree */
    gen_package_tree(num_packages, k_val, &package_tree, package_leaders);
    /* STEP 5. Create a template tree for the ranks */
    mpi_errno = create_template_tree(&template_tree, k_val, right_skewed,
                                     max_ranks_per_package, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* use the template tree to generate the tree for each rank */
    for (int p = 0; p < num_packages; ++p) {
        for (int r = 0; r < package_ctr[p]; ++r) {
            rank = ranks_per_package[p][r];
            if (MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r) == -1) {
                MPIDI_SHM_TOPOTREE_PARENT(&tree, rank) = -1;
            } else {
                MPIDI_SHM_TOPOTREE_PARENT(&tree, rank) =
                    ranks_per_package[p][MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r)];
            }
            for (int j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(&template_tree, r); ++j) {
                idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, rank);
                if (MPIDI_SHM_TOPOTREE_CHILD(&template_tree, r, j) < package_ctr[p]) {
                    MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, rank)++;
                    MPIDI_SHM_TOPOTREE_CHILD(&tree, rank, idx) =
                        ranks_per_package[p][MPIDI_SHM_TOPOTREE_CHILD(&template_tree, r, j)];
                }
            }
        }
    }
    /* Assemble the per package tree package leaders tree and copy it to shared memory region */
    gen_tree_sharedmemory(shared_region, &tree, &package_tree, package_leaders,
                          num_packages, num_ranks, k_val, package_leaders_first);
    MPL_free(tree.base);
    MPL_free(package_tree.base);
    MPL_free(template_tree.base);

  fn_exit:
    MPIR_CHKPMEM_REAP();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function produces topology aware trees for reduction and broadcasts, with different
 * K values. This is a heavy-weight function as it allocates shared memory, generates topology
 * information, builds a package-level tree (for package leaders), and a per-package tree.
 * These are combined in shared memory for other ranks to read out from.
 * */
int MPIDI_SHM_topology_tree_init(MPIR_Comm * comm_ptr, int root, int bcast_k,
                                 MPIR_Treealgo_tree_t * bcast_tree, int *bcast_topotree_fail,
                                 int reduce_k, MPIR_Treealgo_tree_t * reduce_tree,
                                 int *reduce_topotree_fail, MPIR_Errflag_t * errflag)
{
    int *shared_region;
    int num_ranks, rank;
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    size_t shm_size;
    int **ranks_per_package = NULL;
    int *package_ctr = NULL;
    int package_level = 0, max_ranks_per_package = 0;
    bool mapfail_flag = false;

    MPIR_FUNC_ENTER;

    num_ranks = MPIR_Comm_size(comm_ptr);
    rank = MPIR_Comm_rank(comm_ptr);

    /* FIXME: explain the magic number 5? */
    shm_size = sizeof(int) * MAX_TOPO_DEPTH * num_ranks + sizeof(int) * 5 * num_ranks;

    /* STEP 1. Create shared memory region for exchanging topology information (root only) */
    mpi_errno = MPIDU_shm_alloc(comm_ptr, shm_size, (void **) &shared_region, &mapfail_flag);
    if (mapfail_flag) {
        MPIR_ERR_ADD(mpi_errno_ret, MPI_ERR_OTHER);
    }
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* STEP 2. Every process fills affinity information in shared_region */
    MPIR_hwtopo_gid_t gid = MPIR_hwtopo_get_leaf();
    int topo_depth = MPIR_hwtopo_get_depth(gid) + 1;
    for (int depth = 0; depth < MAX_TOPO_DEPTH; depth++) {
        if (depth < topo_depth) {
            gid = MPIR_hwtopo_get_ancestor(gid, depth);
            shared_region[rank * MAX_TOPO_DEPTH + depth] = MPIR_hwtopo_get_lid(gid);
        } else {
            shared_region[rank * MAX_TOPO_DEPTH + depth] = 0;
        }
    }
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* STEP 3. Root has all the bind_map information, now build tree */
    int num_packages = 0;       /* init to avoid -Wmaybe-uninitialized */
    if (rank == root) {
        int *bind_map = shared_region;
        /* Done building the topology information */

        /* STEP 3.1. Count the maximum entries at each level - used for breaking the tree into
         * intra/inter socket */
        package_level = topotree_get_package_level(&num_packages, num_ranks, bind_map);

        /* STEP 3.2. allocate space for the entries that go in each package based on topology info */
        ranks_per_package = (int **) MPL_malloc(num_packages * sizeof(int *), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!ranks_per_package, mpi_errno, MPI_ERR_OTHER, "**nomem");
        package_ctr = (int *) MPL_calloc(num_packages, sizeof(int), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!package_ctr, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (int i = 0; i < num_packages; ++i) {
            package_ctr[i] = 0;
            ranks_per_package[i] = (int *) MPL_calloc(num_ranks, sizeof(int), MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!ranks_per_package[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        /* sort the ranks into packages based on the binding information */
        for (int i = 0; i < num_ranks; ++i) {
            int package = bind_map[i * MAX_TOPO_DEPTH + package_level];
            ranks_per_package[package][package_ctr[package]++] = i;
        }
        max_ranks_per_package = 0;
        for (int i = 0; i < num_packages; ++i) {
            max_ranks_per_package = MPL_MAX(max_ranks_per_package, package_ctr[i]);
        }
        /* At this point we have done the common work in extracting topology information
         * and restructuring it to our needs. Now we generate the tree. */

        /* For Bcast, package leaders are added before the package local ranks, and the per_package
         * tree is left_skewed */
        mpi_errno = gen_tree(bcast_k, shared_region, num_packages,
                             ranks_per_package, max_ranks_per_package, package_ctr,
                             package_level, num_ranks, 1 /*package_leaders_first */ ,
                             0 /*left_skewed */ , errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* Every rank copies their tree out from shared memory */
    copy_tree(shared_region, num_ranks, rank, bcast_tree, bcast_topotree_fail);

    /* Wait until shared memory is available */
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* Generate the reduce tree */
    /* For Reduce, package leaders are added after the package local ranks, and the per_package
     * tree is right_skewed (children are added in the reverse order */
    if (rank == root) {
        memset(shared_region, 0, shm_size);
        mpi_errno = gen_tree(reduce_k, shared_region, num_packages,
                             ranks_per_package, max_ranks_per_package, package_ctr,
                             package_level, num_ranks, 0 /*package_leaders_last */ ,
                             1 /*right_skewed */ , errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* each rank copy the reduce tree out */
    copy_tree(shared_region, num_ranks, rank, reduce_tree, reduce_topotree_fail);

    /* Wait for all ranks to copy out the tree */
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* Cleanup */
    if (rank == root) {
        for (int i = 0; i < num_packages; ++i) {
            MPL_free(ranks_per_package[i]);
        }
        MPL_free(ranks_per_package);
        MPL_free(package_ctr);
    }
    MPIDU_shm_free(shared_region);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
