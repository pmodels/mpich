/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE
      category    : COLLECTIVE
      type        : int
      default     : 65536
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum shared memory created per node for optimized intra-node collectives (in KB)

    - name        : MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 32768
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Total size of the bcast buffer (in bytes)

    - name        : MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of cells the bcast buffer is divided into

    - name        : MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE
      category    : COLLECTIVE
      type        : int
      default     : 32768
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Total size of the reduce buffer per rank (in bytes)

    - name        : MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Number of cells the reduce buffer is divided into, for each rank

    - name        : MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 64
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for the kary/knomial tree for intra-node bcast

    - name        : MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for intra-node bcast tree
        kary      - kary tree type
        knomial_1 - knomial_1 tree type (ranks are added in order from the left side)
        knomial_2 - knomial_2 tree type (ranks are added in order from the right side)

    - name        : MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL
      category    : COLLECTIVE
      type        : int
      default     : 4
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        K value for the kary/knomial tree for intra-node reduce

    - name        : MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE
      category    : COLLECTIVE
      type        : string
      default     : kary
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Tree type for intra-node reduce tree
        kary      - kary tree type
        knomial_1 - knomial_1 tree type (ranks are added in order from the left side)
        knomial_2 - knomial_2 tree type (ranks are added in order from the right side)

    - name        : MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES
      category    : COLLECTIVE
      type        : int
      default     : 1
      class       : device
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
        respectively for bast and reduce. But of as now topology aware trees are only kary. knomial
        is to be implemented.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "release_gather.h"
#ifdef HAVE_HWLOC
#include "topotree.h"
#include "topotree_util.h"
#endif

#define COMM_FIELD(comm, field)                   \
    MPIDI_POSIX_COMM(comm)->release_gather->field


MPIDI_POSIX_release_gather_tree_type_t MPIDI_POSIX_Bcast_tree_type, MPIDI_POSIX_Reduce_tree_type;

/* Initialize the release_gather struct to NULL */
int MPIDI_POSIX_mpi_release_gather_comm_init_null(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT_NULL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT_NULL);

    MPIDI_POSIX_COMM(comm_ptr)->release_gather = NULL;

    if (0 == strcmp(MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE, "kary"))
        MPIDI_POSIX_Bcast_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE, "knomial_1"))
        MPIDI_POSIX_Bcast_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_BCAST_INTRANODE_TREE_TYPE, "knomial_2"))
        MPIDI_POSIX_Bcast_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2;
    else
        MPIDI_POSIX_Bcast_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;

    if (0 == strcmp(MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE, "kary"))
        MPIDI_POSIX_Reduce_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE, "knomial_1"))
        MPIDI_POSIX_Reduce_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_1;
    else if (0 == strcmp(MPIR_CVAR_REDUCE_INTRANODE_TREE_TYPE, "knomial_2"))
        MPIDI_POSIX_Reduce_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KNOMIAL_2;
    else
        MPIDI_POSIX_Reduce_tree_type = MPIDI_POSIX_RELEASE_GATHER_TREE_TYPE_KARY;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT_NULL);
    return MPI_SUCCESS;
}

/* Initialize the data structures and allocate the shared memory (flags, bcast buffer and reduce
 * buffer) */
int MPIDI_POSIX_mpi_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                             const MPIDI_POSIX_release_gather_opcode_t operation)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT);

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int rank, num_ranks;
    bool initialize_flags = false, initialize_bcast_buf = false, initialize_reduce_buf = false;
    int flags_num_pages, fallback = 0;
    size_t flags_shm_size = 0;
    const long pg_sz = sysconf(_SC_PAGESIZE);
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    bool mapfail_flag = false;
    int topotree_fail[2] = { 0, 0 };

    rank = MPIR_Comm_rank(comm_ptr);
    num_ranks = MPIR_Comm_size(comm_ptr);
    /* Layout of the shm region: 1 gather and release flag each per rank, followed by
     * bcast buffer (divided into multiple cells), followed by
     * reduce buffer (divided into multiple cells) per rank. */

    if (MPIDI_POSIX_COMM(comm_ptr)->release_gather == NULL) {
        /* release_gather based collectives have not been used before on this comm */
        initialize_flags = true;
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
            initialize_bcast_buf = true;
        } else if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
            initialize_reduce_buf = true;
        }
    } else {
        /* at least one release_gather based collective was used on this comm */
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST &&
            COMM_FIELD(comm_ptr, bcast_buf_addr) == NULL) {
            initialize_bcast_buf = true;
        } else if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE &&
                   COMM_FIELD(comm_ptr, reduce_buf_addr) == NULL) {
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
            flags_num_pages = flags_shm_size / (int) (pg_sz);
            if (flags_shm_size % pg_sz != 0) {
                flags_num_pages++;
            }
            flags_shm_size = flags_num_pages * pg_sz;

            /* Update the amount of memory to be allocated */
            memory_to_be_allocated += flags_shm_size;
        }

        if (initialize_bcast_buf) {
            memory_to_be_allocated += MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE;
        }
        if (initialize_reduce_buf) {
            memory_to_be_allocated += (num_ranks * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE);
        }

        if (rank == 0) {
            /* rank 0 decides if more memory can be created and broadcasts the decision to other ranks */
            tmp_shm_counter = zm_atomic_load(MPIDI_POSIX_shm_limit_counter, zm_memord_acquire);

            /* Check if it is allowed to create more shm on this node */
            if ((tmp_shm_counter + memory_to_be_allocated) >
                (MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE * 1024)) {
                /* cannot create more shm, fallback to MPIR level algorithms, and broadcast the decision to other ranks */
                fallback = 1;
                MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_NO_MEM, "**nomem");
            } else {
                /* More shm can be created, update the shared counter */
                zm_atomic_fetch_add(MPIDI_POSIX_shm_limit_counter, memory_to_be_allocated,
                                    zm_memord_seq_cst);
                fallback = 0;
                mpi_errno = MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            }
        } else {
            mpi_errno = MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
            if (fallback) {
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_NO_MEM, "**nomem");
            }
        }
    }

    if (initialize_flags) {
        /* Initialize the release_gather struct and allocate shm for flags */
        MPIDI_POSIX_release_gather_comm_t *release_gather_info_ptr;
        release_gather_info_ptr =
            MPL_malloc(sizeof(struct MPIDI_POSIX_release_gather_comm_t), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!release_gather_info_ptr, mpi_errno, MPI_ERR_OTHER, "**nomem");

        release_gather_info_ptr->flags_shm_size = flags_shm_size;

#ifdef HAVE_HWLOC
        /* Create bcast_tree and reduce_tree with root of the tree as 0 */
        if (MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES &&
            getenv("HYDRA_USER_PROVIDED_BINDING")) {
            if (hwloc_topology_load(MPIR_Process.hwloc_topology) == 0) {
                mpi_errno =
                    MPIDI_SHM_topology_tree_init(comm_ptr, 0, MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL,
                                                 &release_gather_info_ptr->bcast_tree,
                                                 &topotree_fail[0],
                                                 MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL,
                                                 &release_gather_info_ptr->reduce_tree,
                                                 &topotree_fail[1], &errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, errflag, "**fail");
                    MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
                }
            } else {
                /* Finalize was already called and MPIR_Process.hwloc_topology has been destroyed */
                topotree_fail[0] = -1;
                topotree_fail[1] = -1;
            }
            mpi_errno = MPIR_Allreduce_impl(MPI_IN_PLACE, topotree_fail, 2, MPI_INT,
                                            MPI_MAX, comm_ptr, &errflag);
        } else {
            topotree_fail[0] = -1;
            topotree_fail[1] = -1;
        }
#endif
        /* Non-topology aware trees */
        if (topotree_fail[0] != 0) {
            if (topotree_fail[0] == 1)
                MPIR_Treealgo_tree_free(&release_gather_info_ptr->bcast_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks, MPIDI_POSIX_Bcast_tree_type,
                                          MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL, 0,
                                          &release_gather_info_ptr->bcast_tree);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        if (topotree_fail[1] != 0) {
            if (topotree_fail[1] == 1)
                MPIR_Treealgo_tree_free(&release_gather_info_ptr->reduce_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks, MPIDI_POSIX_Reduce_tree_type,
                                          MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL, 0,
                                          &release_gather_info_ptr->reduce_tree);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, errflag, "**fail");
                MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
            }
        }

        release_gather_info_ptr->gather_state = release_gather_info_ptr->release_state
            = MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS + MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;

        release_gather_info_ptr->bcast_buf_addr = NULL;
        release_gather_info_ptr->reduce_buf_addr = NULL;
        release_gather_info_ptr->child_reduce_buf_addr = NULL;

        mpi_errno = MPIDIU_allocate_shm_segment(comm_ptr, flags_shm_size,
                                                &(release_gather_info_ptr->shm_flags_handle),
                                                (void **)
                                                &(release_gather_info_ptr->flags_addr),
                                                &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* Calculate gather and release flag address and initialize to the gather and release states */
        release_gather_info_ptr->gather_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_GATHER_FLAG_ADDR(rank);
        release_gather_info_ptr->release_flag_addr =
            MPIDI_POSIX_RELEASE_GATHER_RELEASE_FLAG_ADDR(rank);
        *(release_gather_info_ptr->gather_flag_addr) = (release_gather_info_ptr->gather_state);
        *(release_gather_info_ptr->release_flag_addr) = (release_gather_info_ptr->release_state);

        /* Make sure all the flags are set before ranks start reading each other's flags from shm */
        mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
        MPIDI_POSIX_COMM(comm_ptr)->release_gather = release_gather_info_ptr;
    }

    if (initialize_bcast_buf) {
        /* Allocate the shared memory for bcast buffer */
        mpi_errno =
            MPIDIU_allocate_shm_segment(comm_ptr, MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE,
                                        &(COMM_FIELD(comm_ptr, shm_bcast_buf_handle)),
                                        (void **) &(COMM_FIELD(comm_ptr, bcast_buf_addr)),
                                        &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (initialize_reduce_buf) {
        /* Allocate the shared memory for a reduce buffer per rank */
        int i;
        COMM_FIELD(comm_ptr, child_reduce_buf_addr) =
            MPL_malloc(num_ranks * sizeof(void *), MPL_MEM_COLL);

        mpi_errno =
            MPIDIU_allocate_shm_segment(comm_ptr,
                                        num_ranks *
                                        MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE,
                                        &(COMM_FIELD(comm_ptr, shm_reduce_buf_handle)),
                                        (void **) &(COMM_FIELD(comm_ptr, reduce_buf_addr)),
                                        &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        /* Store address of each of the children's reduce buffer */
        for (i = 0; i < COMM_FIELD(comm_ptr, reduce_tree.num_children); i++) {
            MPIR_ERR_CHKANDJUMP(!utarray_eltptr(COMM_FIELD(comm_ptr, reduce_tree.children), i),
                                mpi_errno, MPI_ERR_OTHER, "**nomem");
            COMM_FIELD(comm_ptr, child_reduce_buf_addr[i]) =
                (char *) COMM_FIELD(comm_ptr, reduce_buf_addr) +
                ((*utarray_eltptr(COMM_FIELD(comm_ptr, reduce_tree.children), i))
                 * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE);
        }
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Cleanup the release_gather data structures and free the allocated memory */
int MPIDI_POSIX_mpi_release_gather_comm_free(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_FREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_FREE);

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* Clean up is not required for NULL struct */
    if (MPIDI_POSIX_COMM(comm_ptr)->release_gather == NULL) {
        goto fn_exit;
    }

    /* destroy and detach shared memory used for flags */
    mpi_errno = MPL_shm_seg_detach(COMM_FIELD(comm_ptr, shm_flags_handle),
                                   (void **) &COMM_FIELD(comm_ptr, flags_addr),
                                   COMM_FIELD(comm_ptr, flags_shm_size));
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    mpi_errno = MPL_shm_hnd_finalize(&COMM_FIELD(comm_ptr, shm_flags_handle));
    if (mpi_errno) {
        /* for communication errors, just record the error but continue */
        errflag =
            MPIX_ERR_PROC_FAILED ==
            MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
        MPIR_ERR_SET(mpi_errno, errflag, "**fail");
        MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
    }

    if (COMM_FIELD(comm_ptr, bcast_buf_addr) != NULL) {
        /* destroy and detach shared memory used for bcast buffer */
        mpi_errno = MPL_shm_seg_detach(COMM_FIELD(comm_ptr, shm_bcast_buf_handle),
                                       (void **) &COMM_FIELD(comm_ptr, bcast_buf_addr),
                                       MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno = MPL_shm_hnd_finalize(&COMM_FIELD(comm_ptr, shm_bcast_buf_handle));
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }
    }

    if (COMM_FIELD(comm_ptr, reduce_buf_addr) != NULL) {
        /* destroy and detach shared memory used for reduce buffers */
        mpi_errno = MPL_shm_seg_detach(COMM_FIELD(comm_ptr, shm_reduce_buf_handle),
                                       (void **) &COMM_FIELD(comm_ptr, reduce_buf_addr),
                                       MPIR_Comm_size(comm_ptr)
                                       * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE);
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        mpi_errno = MPL_shm_hnd_finalize(&COMM_FIELD(comm_ptr, shm_reduce_buf_handle));
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno_ret, mpi_errno);
        }

        MPL_free(COMM_FIELD(comm_ptr, child_reduce_buf_addr));
    }

    MPIR_Treealgo_tree_free(&(COMM_FIELD(comm_ptr, bcast_tree)));
    MPIR_Treealgo_tree_free(&(COMM_FIELD(comm_ptr, reduce_tree)));
    MPL_free(MPIDI_POSIX_COMM(comm_ptr)->release_gather);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_POSIX_MPI_RELEASE_GATHER_COMM_FREE);
    return mpi_errno;
}
