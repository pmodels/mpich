/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE
      category    : COLLECTIVE
      type        : int
      default     : 65536
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum shared memory created per node for optimized intra-node collectives (in KB)

    - name        : MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Total size of the bcast buffer (in bytes)

    - name        : MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of cells the bcast buffer is divided into

    - name        : MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 32768
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Total size of the reduce buffer per rank (in bytes)

    - name        : MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of cells the reduce buffer is divided into, for each rank

    - name        : MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 64
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for the kary/knomial tree for intra-node bcast

    - name        : MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for intra-node bcast tree
        kary      - kary tree type
        knomial_1 - knomial_1 tree type (ranks are added in order from the left side)
        knomial_2 - knomial_2 tree type (ranks are added in order from the right side)
        	    knomial_2 is only supported with non topology aware trees.

    - name        : MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for the kary/knomial tree for intra-node reduce

    - name        : MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for intra-node reduce tree
        kary      - kary tree type
        knomial_1 - knomial_1 tree type (ranks are added in order from the left side)
        knomial_2 - knomial_2 tree type (ranks are added in order from the right side)
        	    knomial_2 is only supported with non topology aware trees.

    - name        : MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES
      category    : COLLECTIVE
      type        : int
      default     : 1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Enable collective specific intra-node trees which leverage the memory hierarchy of a machine.
        Depends on hwloc to extract the binding information of each rank. Pick a leader rank per
        package (socket), then create a per_package tree for ranks on a same package, package leaders
        tree for package leaders.
        For Bcast - Assemble the per_package and package_leaders tree in such a way that leaders
        interact among themselves first before interacting with package local ranks. Both the
        package_leaders and per_package trees are left skewed (children are added from left to right,
        first child to be added is the first one to be processed in traversal)
        For Reduce - Assemble the per_package and package_leaders tree in such a way that a leader
        rank interacts with its package local ranks first, then with the other package leaders. Both
        the per_package and package_leaders tree is right skewed (children are added in reverse
        order, first child to be added is the last one to be processed in traversal)
        The tree radix and tree type of package_leaders and per_package tree is
        MPIR_CVAR_BCAST{REDUCE}_INTRANODE_TREE_KVAL and MPIR_CVAR_BCAST{REDUCE}_INTRANODE_TREE_TYPE
        respectively for bast and reduce. But of as now topology aware trees are only kary and knomial_1.
        knomial_2 is not implemented.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "release_gather.h"
#include "topotree.h"
#include "topotree_util.h"


MPIDI_POSIX_release_gather_tree_type_t MPIDI_POSIX_Bcast_tree_type, MPIDI_POSIX_Reduce_tree_type;

static int get_tree_type(const char *tree_type_name)
{
    if (0 == strcmp(tree_type_name, "kary"))
        return MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;
    else if (0 == strcmp(tree_type_name, "knomial_1"))
        return MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(tree_type_name, "knomial_2"))
        return MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2;
    else
        return MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;
}

/* Initialize the release_gather struct to NULL */
int MPIDI_POSIX_mpi_release_gather_comm_init_null(MPIR_Comm * comm_ptr)
{

    MPIR_FUNC_ENTER;

    RELEASE_GATHER_FIELD(comm_ptr, num_collective_calls) = 0;
    RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 0;

    NB_RELEASE_GATHER_FIELD(comm_ptr, num_collective_calls) = 0;
    NB_RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 0;

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

/* Initialize the data structures and allocate the shared memory (flags, bcast buffer and reduce
 * buffer) */
int MPIDI_POSIX_mpi_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                             const MPIDI_POSIX_release_gather_opcode_t operation)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int rank, num_ranks;
    bool initialize_flags = false, initialize_bcast_buf = false, initialize_reduce_buf = false;
    int flags_num_pages, fallback = 0;
    size_t flags_shm_size = 0;
    const long pg_sz = sysconf(_SC_PAGESIZE);
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    bool mapfail_flag = false;
    int topotree_fail[2] = { -1, -1 };  /* -1 means topo trees not created due to reasons like not
                                         * specifying binding, no hwloc etc. 0 means topo trees were
                                         * created successfully. 1 means topo trees were created
                                         * but there was a failure, so the tree needs to be freed
                                         * before creating the non-topo tree */

    rank = MPIR_Comm_rank(comm_ptr);
    num_ranks = MPIR_Comm_size(comm_ptr);
    /* Layout of the shm region: 1 gather and release flag each per rank, followed by
     * bcast buffer (divided into multiple cells), followed by
     * reduce buffer (divided into multiple cells) per rank. */

    if (RELEASE_GATHER_FIELD(comm_ptr, is_initialized) == 0) {
        RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 1;
        /* CVARs may get updated. Turn them into per-comm settings */
        RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_type) =
            get_tree_type(MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE);
        RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_kval) = MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL;
        RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size) =
            MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE;
        RELEASE_GATHER_FIELD(comm_ptr, bcast_num_cells) = MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
        RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_type) =
            get_tree_type(MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE);
        RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_kval) = MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL;
        RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size) =
            MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE;
        RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells) = MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
        /* release_gather based collectives have not been used before on this comm */
        initialize_flags = true;
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
            initialize_bcast_buf = true;
        } else if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE ||
                   operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE) {
            /* For allreduce, we initialize only reduce buffers and use reduce buffer of rank 0
             * to broadcast the reduced data in release step */
            initialize_reduce_buf = true;
        }
    } else {
        /* at least one release_gather based collective was used on this comm */
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST &&
            RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr) == NULL) {
            initialize_bcast_buf = true;
        } else if ((operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE ||
                    operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_ALLREDUCE) &&
                   RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr) == NULL) {
            /* When op is either reduce or allreduce and reduce buffers are not initialized for each
             * rank */
            initialize_reduce_buf = true;
        }
    }

    if (initialize_flags || initialize_bcast_buf || initialize_reduce_buf) {
        /* Calculate the amount of shm to be created */
        size_t tmp_shm_counter;
        size_t memory_to_be_allocated = 0;
        if (initialize_flags) {
            /* Calculate the amount of memory that would be allocated for flags */
            flags_shm_size = MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK * num_ranks;
            /* Reset flags_shm_size so that the data buffers are aligned to the system pages */
            flags_num_pages = (int) (flags_shm_size / pg_sz);
            if (flags_shm_size % pg_sz != 0) {
                flags_num_pages++;
            }
            flags_shm_size = flags_num_pages * pg_sz;

            /* Update the amount of memory to be allocated */
            memory_to_be_allocated += flags_shm_size;
        }

        if (initialize_bcast_buf) {
            memory_to_be_allocated += RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size);
        }
        if (initialize_reduce_buf) {
            memory_to_be_allocated += (num_ranks * RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size));
        }

        if (rank == 0) {
            /* rank 0 decides if more memory can be created and broadcasts the decision to other ranks */
            tmp_shm_counter =
                (size_t) MPL_atomic_acquire_load_uint64(MPIDI_POSIX_shm_limit_counter);

            /* Check if it is allowed to create more shm on this node */
            if ((tmp_shm_counter + memory_to_be_allocated) >
                (MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE * 1024)) {
                /* cannot create more shm, fallback to MPIR level algorithms, and broadcast the decision to other ranks */
                if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_print) {
                    fprintf(stderr,
                            "Intra-node collectives about to allocate more shared memory than \
                            the specified limit through MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE. Fallback \
                            to other algorithms.\n");
                }

                fallback = 1;
                MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
                MPIR_ERR_SETANDJUMP(mpi_errno_ret, MPI_ERR_NO_MEM, "**nomem");
            } else {
                /* More shm can be created, update the shared counter */
                MPL_atomic_fetch_add_uint64(MPIDI_POSIX_shm_limit_counter, memory_to_be_allocated);
                fallback = 0;
                mpi_errno = MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            }
        } else {
            mpi_errno = MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            if (fallback) {
                MPIR_ERR_SETANDJUMP(mpi_errno_ret, MPI_ERR_NO_MEM, "**nomem");
            }
        }
    }

    if (initialize_flags) {
        RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 1;
        /* Initialize the release_gather struct and allocate shm for flags */
        MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr =
            &MPIDI_POSIX_COMM(comm_ptr, release_gather);

        release_gather_info_ptr->flags_shm_size = flags_shm_size;

        /* Create bcast_tree and reduce_tree with root of the tree as 0 */
        if (MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES &&
            getenv("HYDRA_USER_PROVIDED_BINDING")) {
            /* Topology aware trees are created only when the user has specified process binding */
            if (MPIR_hwtopo_is_initialized()) {
                mpi_errno =
                    MPIDI_SHM_topology_tree_init(comm_ptr, 0,
                                                 RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_kval),
                                                 RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_type),
                                                 &release_gather_info_ptr->bcast_tree,
                                                 &topotree_fail[0],
                                                 RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_kval),
                                                 RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_type),
                                                 &release_gather_info_ptr->reduce_tree,
                                                 &topotree_fail[1], &errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            } else {
                /* Finalize was already called and MPIR_Process.hwloc_topology has been destroyed */
                topotree_fail[0] = -1;
                topotree_fail[1] = -1;
            }
            mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, topotree_fail, 2, MPI_INT,
                                            MPI_MAX, comm_ptr, &errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        } else {
            topotree_fail[0] = -1;
            topotree_fail[1] = -1;
        }

        /* Non-topology aware trees */
        if (topotree_fail[0] != 0) {
            if (topotree_fail[0] == 1)
                MPIR_Treealgo_tree_free(&release_gather_info_ptr->bcast_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks,
                                          RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_type),
                                          RELEASE_GATHER_FIELD(comm_ptr, bcast_tree_kval), 0,
                                          &release_gather_info_ptr->bcast_tree);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        if (topotree_fail[1] != 0) {
            if (topotree_fail[1] == 1)
                MPIR_Treealgo_tree_free(&release_gather_info_ptr->reduce_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks,
                                          RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_type),
                                          RELEASE_GATHER_FIELD(comm_ptr, reduce_tree_kval), 0,
                                          &release_gather_info_ptr->reduce_tree);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        }

        release_gather_info_ptr->gather_state = release_gather_info_ptr->release_state =
            RELEASE_GATHER_FIELD(comm_ptr, bcast_num_cells) +
            RELEASE_GATHER_FIELD(comm_ptr, reduce_num_cells);

        release_gather_info_ptr->bcast_buf_addr = NULL;
        release_gather_info_ptr->reduce_buf_addr = NULL;
        release_gather_info_ptr->child_reduce_buf_addr = NULL;

        mpi_errno =
            MPIDU_shm_alloc(comm_ptr, flags_shm_size,
                            (void **) &(release_gather_info_ptr->flags_addr), &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_ADD(mpi_errno_ret, MPIR_ERR_OTHER);
        }

        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno_ret, errflag);
        MPIR_ERR_CHECK(mpi_errno_ret);

        /* Calculate gather and release flag address and initialize to the gather and release states */
        release_gather_info_ptr->gather_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(rank);
        release_gather_info_ptr->release_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(rank);
        MPL_atomic_release_store_uint64((release_gather_info_ptr->gather_flag_addr),
                                        release_gather_info_ptr->gather_state);
        MPL_atomic_release_store_uint64((release_gather_info_ptr->release_flag_addr),
                                        release_gather_info_ptr->release_state);

        /* Make sure all the flags are set before ranks start reading each other's flags from shm */
        mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    if (initialize_bcast_buf) {
        if (rank == 0)
            MPL_atomic_fetch_add_uint64(MPIDI_POSIX_shm_limit_counter,
                                        RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size));

        /* Allocate the shared memory for bcast buffer */
        mpi_errno =
            MPIDU_shm_alloc(comm_ptr, RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size),
                            (void **) &(RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr)),
                            &mapfail_flag);
        if (mapfail_flag) {
            MPIR_ERR_ADD(mpi_errno_ret, MPIR_ERR_OTHER);
        }
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    if (initialize_reduce_buf) {
        /* Allocate the shared memory for a reduce buffer per rank */
        int i;
        RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr) =
            MPL_malloc(num_ranks * sizeof(void *), MPL_MEM_COLL);

        if (rank == 0)
            MPL_atomic_fetch_add_uint64(MPIDI_POSIX_shm_limit_counter,
                                        RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size));

        mpi_errno =
            MPIDU_shm_alloc(comm_ptr, num_ranks * RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size),
                            (void **) &(RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr)),
                            &mapfail_flag);
        if (mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_ADD(mpi_errno_ret, MPIR_ERR_OTHER);
        }
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno_ret, errflag);
        MPIR_ERR_CHECK(mpi_errno_ret);

        /* Store address of each of the children's reduce buffer */
        char *addr;
        addr = RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr);
        for (i = 0; i < RELEASE_GATHER_FIELD(comm_ptr, reduce_tree.num_children); i++) {
            int child_rank =
                *(int *) utarray_eltptr(RELEASE_GATHER_FIELD(comm_ptr, reduce_tree).children, i);
            RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr[i]) =
                addr + (child_rank * RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size));
        }
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    if (mpi_errno_ret != MPI_SUCCESS)
        RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 0;
    return mpi_errno_ret;
  fn_fail:
    goto fn_exit;
}

/* Cleanup the release_gather data structures and free the allocated memory */
int MPIDI_POSIX_mpi_release_gather_comm_free(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    /* Clean up is not required for NULL struct */
    if (RELEASE_GATHER_FIELD(comm_ptr, is_initialized) == 0) {
        goto fn_exit;
    }

    /* decrease shm memory limit counter */
    if (comm_ptr->rank == 0)
        MPL_atomic_fetch_sub_uint64(MPIDI_POSIX_shm_limit_counter,
                                    RELEASE_GATHER_FIELD(comm_ptr, flags_shm_size));

    /* destroy and detach shared memory used for flags */
    mpi_errno = MPIDU_shm_free(RELEASE_GATHER_FIELD(comm_ptr, flags_addr));
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    if (RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr) != NULL) {
        if (comm_ptr->rank == 0)
            MPL_atomic_fetch_sub_uint64(MPIDI_POSIX_shm_limit_counter,
                                        RELEASE_GATHER_FIELD(comm_ptr, bcast_shm_size));
        /* destroy and detach shared memory used for bcast buffer */
        mpi_errno = MPIDU_shm_free(RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    if (RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr) != NULL) {
        if (comm_ptr->rank == 0)
            MPL_atomic_fetch_sub_uint64(MPIDI_POSIX_shm_limit_counter,
                                        RELEASE_GATHER_FIELD(comm_ptr, reduce_shm_size));
        /* destroy and detach shared memory used for reduce buffers */
        mpi_errno = MPIDU_shm_free(RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        MPL_free(RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr));
    }

    MPIR_Treealgo_tree_free(&(RELEASE_GATHER_FIELD(comm_ptr, bcast_tree)));
    MPIR_Treealgo_tree_free(&(RELEASE_GATHER_FIELD(comm_ptr, reduce_tree)));

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno_ret;
}
