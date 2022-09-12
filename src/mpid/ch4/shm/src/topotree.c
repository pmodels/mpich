/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"
#include "topotree_types.h"
#include "topotree_util.h"
#include "topotree.h"

static int calc_parent(int i, int k)
{
    int parent = i / k;
    if (i % k == 0) {
        parent--;
    }
    return parent;
}

static int calc_pow(int k, int n)
{
    int result = k;
    for (int i = 1; i < n; i++) {
        result *= k;
    }
    return result;
}

/* This function allocates and generates a template_tree based on k_val, tree_type, and
 * skewness for max_ranks.
 * */
int MPIDI_SHM_create_template_tree(MPIDI_SHM_topotree_t * template_tree, int k_val, int tree_type,
                                   bool right_skewed, int max_ranks, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, j, child_id, child_idx;

    mpi_errno = MPIDI_SHM_topotree_allocate(template_tree, max_ranks, k_val);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    if (tree_type == MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY) {
        /* Notice that skewness matters only for kary trees */
        for (i = 0; i < max_ranks; ++i) {
            MPIDI_SHM_TOPOTREE_PARENT(template_tree, i) = calc_parent(i, k_val);
            MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i) = 0;
            if (!right_skewed) {
                for (j = 0; j < k_val; ++j) {
                    child_id = i * k_val + 1 + j;
                    if (child_id < max_ranks) {
                        child_idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i)++;
                        MPIDI_SHM_TOPOTREE_CHILD(template_tree, i, child_idx) = child_id;
                    }
                }
            } else if (right_skewed) {
                for (j = k_val - 1; j >= 0; --j) {
                    child_id = i * k_val + 1 + j;
                    if (child_id < max_ranks) {
                        child_idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i)++;
                        MPIDI_SHM_TOPOTREE_CHILD(template_tree, i, child_idx) = child_id;
                    }
                }
            }
        }
    } else {
        /* Handle knomial_2 tree case, as it is not implemented */
        if (tree_type == MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2) {
            if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_error) {
                /* No fallback */
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**nosupport");
            } else if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_print) {
                /* Fallback with print */
                fprintf(stderr, "Intra-node topology aware trees does not have support for \
                        knomial_2 trees. Falling back to knomial_1 trees.\n");
            }
        }
        /* Generate knomial_1 tree */
        int crank, rank, tmp;
        int maxtime = 0;
        const int root = 0;
        for (tmp = max_ranks - 1; tmp; tmp /= k_val)
            maxtime++;
        for (rank = 0; rank < max_ranks; ++rank) {
            int lrank = (rank + (max_ranks - root)) % max_ranks;
            int time = 0;
            int parent = -1;    /* root has no parent */
            int current_rank = 0;       /* start at root of the tree */
            int running_rank = current_rank + 1;

            for (time = 0;; time++) {
                /* desired rank found */
                if (lrank == current_rank)
                    break;
                /* check if rank lies in this range */
                for (j = 1; j < k_val; j++) {
                    if (lrank >= running_rank &&
                        lrank < running_rank + calc_pow(k_val, maxtime - time - 1)) {
                        /* move to the corresponding subtree */
                        parent = current_rank;
                        current_rank = running_rank;
                        running_rank = current_rank + 1;
                        break;
                    }
                    running_rank += calc_pow(k_val, maxtime - time - 1);
                }
            }
            parent = parent == -1 ? -1 : (parent + root) % max_ranks;
            /* set the children */
            crank = lrank + 1;  /* crank stands for child rank */
            for (i = time; i < maxtime; i++) {
                for (j = 1; j < k_val; j++) {
                    if (crank < max_ranks) {
                        child_idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, rank)++;
                        child_id = (crank + root) % max_ranks;
                        MPIDI_SHM_TOPOTREE_CHILD(template_tree, rank, child_idx) = child_id;
                        MPIDI_SHM_TOPOTREE_PARENT(template_tree, child_id) = rank;
                        if (MPIDI_SHM_TOPOTREE_DEBUG)
                            fprintf(stderr, "Rank=%d, AddChild=%d, Index=%d\n", rank,
                                    (crank + root) % max_ranks, child_idx);
                    }
                    crank += calc_pow(k_val, maxtime - i - 1);
                }
            }
        }
    }
    /* Mark the parent of root (rank 0) as -1 */
    MPIDI_SHM_TOPOTREE_PARENT(template_tree, 0) = -1;

    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        fprintf(stderr, "TemplateTree, %d\n", max_ranks);
        MPIDI_SHM_print_topotree("TemplateTree", template_tree);
        for (i = 0; i < max_ranks; ++i) {
            fprintf(stderr, "TemplateR, %d, P=%d, C=%d, [", i,
                    MPIDI_SHM_TOPOTREE_PARENT(template_tree, i),
                    MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i));
            for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(template_tree, i); ++j) {
                fprintf(stderr, "%d, ", MPIDI_SHM_TOPOTREE_CHILD(template_tree, i, j));
            }
            fprintf(stderr, "]\n");
        }
    }
    /* template tree is ready here */

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* This function copies the tree present in shared_region into the my_tree data structure for rank.
 * Doesn't perform any direct allocation, but utarray_new is called to allocate space for children
 * in my_tree.
 * */
void MPIDI_SHM_copy_tree(int *shared_region, int num_ranks, int rank,
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
    MPIR_Assert(my_tree->children);
    utarray_reserve(my_tree->children, num_children, MPL_MEM_COLL);
    char str[1024], tmp[128];
    sprintf(str, "----**Rank %d, Parent, %d, Child(%d)[", rank, parent, num_children);
    for (int c = 0; c < num_children; ++c) {
        utarray_push_back(my_tree->children, &my_children[c], MPL_MEM_COLL);
        if (my_children[c] == 0) {
            *topotree_fail = 1;
        }
        sprintf(tmp, "%d, ", my_children[c]);
        strcat(str, tmp);
        my_tree->num_children++;
    }
    if (MPIDI_SHM_TOPOTREE_DEBUG)
        fprintf(stderr, "%s]\n", str);
}

/* This function returns the topology level where we will break the tree into a package_leaders
 * and a per_package tree. If needed MPIDI_SHM_TOPOTREE_CUTOFF can be modified to the level where this cutoff
 * should happen. Note, this function also fills out max_entries_per_level, which is needed in other
 * functions.
 * */
int MPIDI_SHM_topotree_get_package_level(int topo_depth, int *max_entries_per_level, int num_ranks,
                                         int **bind_map)
{
    int lvl, i, socket_level;

    for (lvl = 0; lvl < topo_depth; ++lvl) {
        max_entries_per_level[lvl] = -1;
        for (i = 0; i < num_ranks; ++i) {
            max_entries_per_level[lvl] =
                (max_entries_per_level[lvl] >
                 bind_map[i][lvl] + 1) ? max_entries_per_level[lvl] : bind_map[i][lvl] + 1;
        }
    }
    /* STEP 3.3. Determine the package level based on first level (top-down) with #nodes >1 */
    socket_level = topo_depth - 1;
    {
        for (i = topo_depth - 1; i >= 0; --i) {
            if (max_entries_per_level[i] > 1) {
                socket_level = i;
                break;
            }
        }

    }
    /* indicates the package level in topology */
    if (MPIDI_SHM_TOPOTREE_DEBUG)
        fprintf(stderr, "Max entries per level :: %d\n", max_entries_per_level[socket_level]);

    return (MPIDI_SHM_TOPOTREE_CUTOFF == -1) ? socket_level : MPIDI_SHM_TOPOTREE_CUTOFF;
}

/* This function generates a package level tree using the package leaders and k_val.
 * */
void MPIDI_SHM_gen_package_tree(int num_packages, int k_val, bool right_skewed,
                                MPIDI_SHM_topotree_t * package_tree, int *package_leaders)
{
    int i, j, parent_idx, child_id, idx;
    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        fprintf(stderr, "Package_leaders:[");
        for (i = 0; i < num_packages; ++i) {
            fprintf(stderr, "%d, ", package_leaders[i]);
        }
        fprintf(stderr, "]\n");
    }
    /* Generate a package (top-level) tree */
    for (i = 0; i < num_packages; ++i) {
        if (i == 0) {
            MPIDI_SHM_TOPOTREE_PARENT(package_tree, i) = -1;
        } else {
            parent_idx = calc_parent(i, k_val);
            MPIDI_SHM_TOPOTREE_PARENT(package_tree, i) = package_leaders[parent_idx];
        }
        MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, i) = 0;
        if (!right_skewed) {
            for (j = 0; j < k_val; ++j) {
                child_id = i * k_val + 1 + j;
                if (child_id < num_packages && package_leaders[child_id] != -1) {
                    idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, i)++;
                    MPIDI_SHM_TOPOTREE_CHILD(package_tree, i, idx) = package_leaders[child_id];
                }
            }
        } else {
            for (j = k_val - 1; j >= 0; --j) {
                child_id = i * k_val + 1 + j;
                if (child_id < num_packages && package_leaders[child_id] != -1) {
                    idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, i)++;
                    MPIDI_SHM_TOPOTREE_CHILD(package_tree, i, idx) = package_leaders[child_id];
                }
            }
        }
    }
    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        fprintf(stderr, "PackageTree for %d packages\n", num_packages);
        MPIDI_SHM_print_topotree("Package", package_tree);
    }
}

/* This function assembles the package leaders tree and per package tree into a single tree in
 * shared memory.
 * */
void MPIDI_SHM_gen_tree_sharedmemory(int *shared_region, MPIDI_SHM_topotree_t * tree,
                                     MPIDI_SHM_topotree_t * package_tree, int *package_leaders,
                                     int num_packages, int num_ranks, int k_val,
                                     bool package_leaders_first)
{
    int i, pm, j, is_package_leader;
    int *parent_ptr = shared_region;
    int *child_ctr = &shared_region[num_ranks];
    int *children = &shared_region[num_ranks + num_ranks + 1];
    memset(shared_region, 0, sizeof(int) * (3 * num_ranks) + 1);
    child_ctr[0] = 0;

    for (i = 0; i < num_ranks; ++i) {
        parent_ptr[i] = i == 0 ? -1 : MPIDI_SHM_TOPOTREE_PARENT(tree, i);
        child_ctr[i + 1] = child_ctr[i];
        /* Package last as 0 means package leaders are added first before adding the per-package
         * tree */
        if (package_leaders_first) {
            /* Add children for package leaders */
            is_package_leader = -1;
            for (pm = 0; pm < num_packages; ++pm) {
                if (i == package_leaders[pm]) {
                    is_package_leader = pm;
                }
            }
            if (is_package_leader != -1) {
                for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, is_package_leader); ++j) {
                    children[child_ctr[i + 1]] =
                        MPIDI_SHM_TOPOTREE_CHILD(package_tree, is_package_leader, j);
                    child_ctr[i + 1]++;
                }
                parent_ptr[i] = MPIDI_SHM_TOPOTREE_PARENT(package_tree, is_package_leader);
            }
        }

        for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(tree, i); ++j) {
            children[child_ctr[i + 1]] = MPIDI_SHM_TOPOTREE_CHILD(tree, i, j);
            child_ctr[i + 1]++;
        }

        /* Package last as non-zero means package leaders are added after adding the per-package
         * tree */
        if (!package_leaders_first) {
            /* Add children for package leaders */
            is_package_leader = -1;
            for (pm = 0; pm < num_packages; ++pm) {
                if (i == package_leaders[pm]) {
                    is_package_leader = pm;
                }
            }
            if (is_package_leader != -1) {
                for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(package_tree, is_package_leader); ++j) {
                    children[child_ctr[i + 1]] =
                        MPIDI_SHM_TOPOTREE_CHILD(package_tree, is_package_leader, j);
                    child_ctr[i + 1]++;
                }
                parent_ptr[i] = MPIDI_SHM_TOPOTREE_PARENT(package_tree, is_package_leader);
            }
        }
    }

    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        for (i = 0; i < num_ranks; ++i) {
            fprintf(stderr, "SRank, %d, Parent, %d, Children(%d)[", i, parent_ptr[i],
                    child_ctr[i + 1] - child_ctr[i]);
            for (j = child_ctr[i]; j < child_ctr[i + 1]; ++j) {
                fprintf(stderr, "%d, ", children[j]);
            }
            fprintf(stderr, "]\n");
        }
    }
}

/* This is the main function which generates a tree in shared memory. The tree is parameterized
 * over the different data-structures:
 * k_val : the tree K-value
 * shared_region : the shared memory region where the tree will be generated
 * max_entries_per_level : the maximum number of ranks per level
 * ranks_per_package : the different ranks at each level
 * max_ranks_per_package : the maximum ranks in any package
 * package_ctr : number of ranks in each package
 * package_level : the topology level where we cutoff the tree
 * num_ranks : the number of ranks
 * */
int MPIDI_SHM_gen_tree(int k_val, int *shared_region, int *max_entries_per_level,
                       int **ranks_per_package, int max_ranks_per_package, int *package_ctr,
                       int package_level, int num_ranks, bool package_leaders_first,
                       bool right_skewed, int tree_type, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, j, p, r, rank, idx;
    int num_packages = max_entries_per_level[package_level];
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
    MPIR_ERR_CHKANDJUMP(!package_leaders, mpi_errno, MPI_ERR_OTHER, "**nomem");

    /* We pick package leaders as the first rank in each package */
    for (p = 0; p < max_entries_per_level[package_level]; ++p) {
        package_leaders[p] = -1;
        if (package_ctr[p] > 0) {
            package_leaders[package_count++] = ranks_per_package[p][0];
        }
    }
    num_packages = package_count;

    /* STEP 4. Now use the template tree to generate the top level tree */
    MPIDI_SHM_gen_package_tree(num_packages, k_val, right_skewed, &package_tree, package_leaders);
    /* STEP 5. Create a template tree for the ranks */
    mpi_errno =
        MPIDI_SHM_create_template_tree(&template_tree, k_val, tree_type, right_skewed,
                                       max_ranks_per_package, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        for (i = 0; i < max_entries_per_level[package_level]; ++i) {
            fprintf(stderr, "pre-Rank %d, parent %d, children=%d [", i,
                    MPIDI_SHM_TOPOTREE_PARENT(&tree, i), MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, i));
            for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, i); ++j) {
                fprintf(stderr, "%d, ", MPIDI_SHM_TOPOTREE_CHILD(&tree, i, j));
            }
            fprintf(stderr, "]\n");
        }
    }

    /* use the template tree to generate the tree for each rank */
    for (p = 0; p < max_entries_per_level[package_level]; ++p) {
        for (r = 0; r < package_ctr[p]; ++r) {
            rank = ranks_per_package[p][r];
            if (MPIDI_SHM_TOPOTREE_DEBUG)
                fprintf(stderr, "Rank=%d, p=%d, r=%d, opt1=%d, opt2=%d\n", rank, p, r,
                        MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r),
                        ranks_per_package[p][MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r)]);
            if (MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r) == -1) {
                MPIDI_SHM_TOPOTREE_PARENT(&tree, rank) = -1;
            } else {
                MPIDI_SHM_TOPOTREE_PARENT(&tree, rank) =
                    ranks_per_package[p][MPIDI_SHM_TOPOTREE_PARENT(&template_tree, r)];
            }
            for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(&template_tree, r); ++j) {
                idx = MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, rank);
                if (MPIDI_SHM_TOPOTREE_CHILD(&template_tree, r, j) < package_ctr[p]) {
                    MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, rank)++;
                    MPIDI_SHM_TOPOTREE_CHILD(&tree, rank, idx) =
                        ranks_per_package[p][MPIDI_SHM_TOPOTREE_CHILD(&template_tree, r, j)];
                }
            }
        }
    }
    if (MPIDI_SHM_TOPOTREE_DEBUG) {
        char str[1024], tmp[128];
        for (i = 0; i < num_ranks; ++i) {
            sprintf(str, "*BaseTreeRank %d, parent %d, children=%d [", i,
                    MPIDI_SHM_TOPOTREE_PARENT(&tree, i), MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, i));
            for (j = 0; j < MPIDI_SHM_TOPOTREE_NUM_CHILD(&tree, i); ++j) {
                sprintf(tmp, "%d, ", MPIDI_SHM_TOPOTREE_CHILD(&tree, i, j));
                strcat(str, tmp);
            }
            fprintf(stderr, "%s]\n", str);
        }
    }
    /* Assemble the per package tree package leaders tree and copy it to shared memory region */
    MPIDI_SHM_gen_tree_sharedmemory(shared_region, &tree, &package_tree, package_leaders,
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
int MPIDI_SHM_topology_tree_init(MPIR_Comm * comm_ptr, int root, int bcast_k, int bcast_tree_type,
                                 MPIR_Treealgo_tree_t * bcast_tree, int *bcast_topotree_fail,
                                 int reduce_k, int reduce_tree_type,
                                 MPIR_Treealgo_tree_t * reduce_tree, int *reduce_topotree_fail,
                                 MPIR_Errflag_t * errflag)
{
    int *shared_region;
    int num_ranks, rank;
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    size_t shm_size;
    int **bind_map = NULL;
    int *max_entries_per_level = NULL;
    int **ranks_per_package = NULL;
    int *package_ctr = NULL;
    int topo_depth = 0;
    int package_level = 0, i, max_ranks_per_package = 0;
    bool mapfail_flag = false;

    MPIR_FUNC_ENTER;

    num_ranks = MPIR_Comm_size(comm_ptr);
    rank = MPIR_Comm_rank(comm_ptr);

    /* Calculate the size of shared memory that would be needed */
    MPIR_hwtopo_gid_t gid = MPIR_hwtopo_get_leaf();
    topo_depth = MPIR_hwtopo_get_depth(gid) + 1;
    shm_size = sizeof(int) * topo_depth * num_ranks + sizeof(int) * 5 * num_ranks;

    /* STEP 1. Create shared memory region for exchanging topology information (root only) */
    mpi_errno = MPIDU_shm_alloc(comm_ptr, shm_size, (void **) &shared_region, &mapfail_flag);
    if (mpi_errno || mapfail_flag) {
        /* for communication errors, just record the error but continue */
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

    /* STEP 2. Every process fills affinity information in shared_region */
    int (*shared_region_ptr)[topo_depth] = (int (*)[topo_depth]) shared_region;
    int depth = 0;
    while (depth < topo_depth) {
        shared_region_ptr[rank][depth++] = MPIR_hwtopo_get_lid(gid);
        gid = MPIR_hwtopo_get_ancestor(gid, topo_depth - depth - 1);
    }
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* STEP 3. Root has all the bind_map information, now build tree */
    if (rank == root) {
        bind_map = (int **) MPL_malloc(num_ranks * sizeof(int *), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!bind_map, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (i = 0; i < num_ranks; ++i) {
            bind_map[i] = (int *) MPL_calloc(topo_depth, sizeof(int), MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!bind_map[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
            MPIR_Memcpy(bind_map[i], shared_region_ptr[i], topo_depth * sizeof(int));
        }
        /* Done building the topology information */

        /* STEP 3.1. Count the maximum entries at each level - used for breaking the tree into
         * intra/inter socket */
        max_entries_per_level = (int *) MPL_calloc(topo_depth, sizeof(size_t), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!max_entries_per_level, mpi_errno, MPI_ERR_OTHER, "**nomem");
        package_level =
            MPIDI_SHM_topotree_get_package_level(topo_depth, max_entries_per_level, num_ranks,
                                                 bind_map);
        if (MPIDI_SHM_TOPOTREE_DEBUG)
            fprintf(stderr, "Breaking topology at :: %d (default= %d)\n", package_level,
                    MPIDI_SHM_TOPOTREE_CUTOFF);

        /* STEP 3.2. allocate space for the entries that go in each package based on topology info */
        ranks_per_package =
            (int
             **) MPL_malloc(max_entries_per_level[package_level] * sizeof(int *), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!ranks_per_package, mpi_errno, MPI_ERR_OTHER, "**nomem");
        package_ctr =
            (int *) MPL_calloc(max_entries_per_level[package_level], sizeof(int), MPL_MEM_OTHER);
        MPIR_ERR_CHKANDJUMP(!package_ctr, mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (i = 0; i < max_entries_per_level[package_level]; ++i) {
            package_ctr[i] = 0;
            ranks_per_package[i] = (int *) MPL_calloc(num_ranks, sizeof(int), MPL_MEM_OTHER);
            MPIR_ERR_CHKANDJUMP(!ranks_per_package[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
        }
        /* sort the ranks into packages based on the binding information */
        for (i = 0; i < num_ranks; ++i) {
            int package = bind_map[i][package_level];
            ranks_per_package[package][package_ctr[package]++] = i;
        }
        max_ranks_per_package = 0;
        for (i = 0; i < max_entries_per_level[package_level]; ++i) {
            max_ranks_per_package = MPL_MAX(max_ranks_per_package, package_ctr[i]);
        }
        /* At this point we have done the common work in extracting topology information
         * and restructuring it to our needs. Now we generate the tree. */

        /* For Bcast, package leaders are added before the package local ranks, and the per_package
         * tree is left_skewed */
        mpi_errno = MPIDI_SHM_gen_tree(bcast_k, shared_region, max_entries_per_level,
                                       ranks_per_package, max_ranks_per_package, package_ctr,
                                       package_level, num_ranks, 1 /*package_leaders_first */ ,
                                       0 /*left_skewed */ , bcast_tree_type, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* Every rank copies their tree out from shared memory */
    MPIDI_SHM_copy_tree(shared_region, num_ranks, rank, bcast_tree, bcast_topotree_fail);
    if (MPIDI_SHM_TOPOTREE_DEBUG)
        MPIDI_SHM_print_topotree_file("BCAST", comm_ptr->context_id, rank, bcast_tree);

    /* Wait until shared memory is available */
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* Generate the reduce tree */
    /* For Reduce, package leaders are added after the package local ranks, and the per_package
     * tree is right_skewed (children are added in the reverse order */
    if (rank == root) {
        memset(shared_region, 0, shm_size);
        mpi_errno = MPIDI_SHM_gen_tree(reduce_k, shared_region, max_entries_per_level,
                                       ranks_per_package, max_ranks_per_package, package_ctr,
                                       package_level, num_ranks, 0 /*package_leaders_last */ ,
                                       1 /*right_skewed */ , reduce_tree_type, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

    /* each rank copy the reduce tree out */
    MPIDI_SHM_copy_tree(shared_region, num_ranks, rank, reduce_tree, reduce_topotree_fail);

    if (MPIDI_SHM_TOPOTREE_DEBUG)
        MPIDI_SHM_print_topotree_file("REDUCE", comm_ptr->context_id, rank, reduce_tree);
    /* Wait for all ranks to copy out the tree */
    mpi_errno = MPIR_Barrier_impl(comm_ptr, errflag);
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    /* Cleanup */
    if (rank == root) {
        for (i = 0; i < max_entries_per_level[package_level]; ++i) {
            MPL_free(ranks_per_package[i]);
        }
        MPL_free(ranks_per_package);
        MPL_free(package_ctr);
        if (MPIDI_SHM_TOPOTREE_DEBUG)
            for (i = 0; i < topo_depth; ++i) {
                fprintf(stderr, "Level :: %d, Max :: %d\n", i, max_entries_per_level[i]);
            }
        for (i = 0; i < num_ranks; ++i) {
            MPL_free(bind_map[i]);
        }
        MPL_free(max_entries_per_level);
        MPL_free(bind_map);
    }
    MPIDU_shm_free(shared_region);

  fn_exit:
    if (rank == root && MPIDI_SHM_TOPOTREE_DEBUG)
        fprintf(stderr, "Done creating tree for %d\n", num_ranks);
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
