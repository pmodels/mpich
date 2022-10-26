/*
 * Copyright (C) by Argonne National Laboratory.
 * 	See COPYRIGHT in top-level directory.
 *
 */

#include "mpiimpl.h"
#include "release_gather.h"
#include "nb_bcast_release_gather.h"
#include "nb_reduce_release_gather.h"
#ifdef HAVE_HWLOC
#include "topotree.h"
#include "topotree_util.h"
#include "mpir_hwtopo.h"
#endif


/* Initialize the data structures and allocate the shared memory (flags, bcast buffer) */
int MPIDI_POSIX_nb_release_gather_comm_init(MPIR_Comm * comm_ptr,
                                            const MPIDI_POSIX_release_gather_opcode_t operation)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    int rank, num_ranks;
    bool initialize_tree = false, initialize_ibcast_buf = false, initialize_ireduce_buf = false;
    int ibcast_flags_num_pages, ireduce_flags_num_pages, fallback = 0;
    size_t ibcast_flags_shm_size = 0, ireduce_flags_shm_size = 0;
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

    int i;
    MPIDI_POSIX_release_gather_comm_t *nb_release_gather_info_ptr = NULL;

    if (NB_RELEASE_GATHER_FIELD(comm_ptr, is_initialized) == 0) {
        /* release_gather based nb collectives have not been used before on this comm */
        initialize_tree = true;
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST) {
            initialize_ibcast_buf = true;
        } else if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE) {
            initialize_ireduce_buf = true;
        }
    } else {
        /* at least one release_gather based nb collective was used on this comm */
        if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_BCAST &&
            NB_RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr) == NULL) {
            initialize_ibcast_buf = true;
        } else if (operation == MPIDI_POSIX_RELEASE_GATHER_OPCODE_REDUCE &&
                   NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr) == NULL) {
            initialize_ireduce_buf = true;
        }
    }

    if (initialize_ibcast_buf || initialize_ireduce_buf) {
        /* Calculate the amount of shm to be created */
        size_t tmp_shm_counter;
        size_t memory_to_be_allocated = 0;

        /* Layout of the shm region for flags: for each cell, a gather flag for each rank, followed
         * by a release flag for each rank */
        /* Calculate the amount of memory that would be allocated for flags */
        if (initialize_ibcast_buf) {
            ibcast_flags_shm_size = MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK * num_ranks *
                MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS;
            /* Reset flags_shm_size so that the data buffers are aligned to the system pages */
            ibcast_flags_num_pages = (int) (ibcast_flags_shm_size / pg_sz);
            if (ibcast_flags_shm_size % pg_sz != 0) {
                ibcast_flags_num_pages++;
            }
            ibcast_flags_shm_size = ibcast_flags_num_pages * pg_sz;
        } else if (initialize_ireduce_buf) {
            ireduce_flags_shm_size = MPIDI_POSIX_RELEASE_GATHER_FLAG_SPACE_PER_RANK * num_ranks *
                MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS;
            /* Reset flags_shm_size so that the data buffers are aligned to the system pages */
            ireduce_flags_num_pages = (int) (ireduce_flags_shm_size / pg_sz);
            if (ireduce_flags_shm_size % pg_sz != 0) {
                ireduce_flags_num_pages++;
            }
            ireduce_flags_shm_size = ireduce_flags_num_pages * pg_sz;
        }

        /* Update the amount of memory to be allocated */
        memory_to_be_allocated += ibcast_flags_shm_size + ireduce_flags_shm_size;

        if (initialize_ibcast_buf) {
            memory_to_be_allocated += MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE;
        }
        if (initialize_ireduce_buf) {
            memory_to_be_allocated += (num_ranks * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE);
        }

        if (rank == 0) {
            /* rank 0 decides if more memory can be created and broadcasts the decision to other
             * ranks */
            tmp_shm_counter =
                (size_t) MPL_atomic_acquire_load_uint64(MPIDI_POSIX_shm_limit_counter);

            /* Check if it is allowed to create more shm on this node */
            if ((tmp_shm_counter + memory_to_be_allocated) >
                (MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE * 1024)) {
                /* cannot create more shm, fallback to MPIR level algorithms, and broadcast the
                 * decision to other ranks */
                if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_print) {
                    fprintf(stderr,
                            "Intra-node collectives about to allocate more shared memory than \
                            the specified limit through MPIR_CVAR_COLL_SHM_LIMIT_PER_NODE. Fallback \
                            to other algorithms.\n");
                }
                fallback = 1;
                MPIR_Bcast_impl(&fallback, 1, MPI_INT, 0, comm_ptr, &errflag);
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_NO_MEM, "**nomem");
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
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_NO_MEM, "**nomem");
            }
        }
    }

    if (initialize_tree) {
        /* Initialize the release_gather struct (tree, buffers) and attach to the communicator */
        NB_RELEASE_GATHER_FIELD(comm_ptr, is_initialized) = 1;
        nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);

#ifdef HAVE_HWLOC
        /* Create bcast_tree and reduce_tree with root of the tree as 0 */
        if (MPIR_CVAR_ENABLE_INTRANODE_TOPOLOGY_AWARE_TREES &&
            getenv("HYDRA_USER_PROVIDED_BINDING")) {
            /* Topology aware trees are created only when the user has specified process binding */
            if (MPIR_hwtopo_is_initialized()) {

                mpi_errno =
                    MPIDI_SHM_topology_tree_init(comm_ptr, 0,
                                                 MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL,
                                                 MPIDI_POSIX_Bcast_tree_type,
                                                 &nb_release_gather_info_ptr->bcast_tree,
                                                 &topotree_fail[0],
                                                 MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL,
                                                 MPIDI_POSIX_Reduce_tree_type,
                                                 &nb_release_gather_info_ptr->reduce_tree,
                                                 &topotree_fail[1], &errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
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
                MPIR_Treealgo_tree_free(&nb_release_gather_info_ptr->bcast_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks, MPIDI_POSIX_Bcast_tree_type,
                                          MPIR_CVAR_BCAST_INTRANODE_TREE_KVAL, 0,
                                          &nb_release_gather_info_ptr->bcast_tree);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        if (topotree_fail[1] != 0) {
            if (topotree_fail[1] == 1)
                MPIR_Treealgo_tree_free(&nb_release_gather_info_ptr->reduce_tree);
            mpi_errno =
                MPIR_Treealgo_tree_create(rank, num_ranks, MPIDI_POSIX_Reduce_tree_type,
                                          MPIR_CVAR_REDUCE_INTRANODE_TREE_KVAL, 0,
                                          &nb_release_gather_info_ptr->reduce_tree);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        nb_release_gather_info_ptr->bcast_buf_addr = NULL;
        nb_release_gather_info_ptr->reduce_buf_addr = NULL;
    }


    if (initialize_ibcast_buf) {
        /* Allocate and initialize the flags and buffer for non-blocking bcast */
        nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
        /* Array to keep track of last seq_no completed on each shm cell */
        nb_release_gather_info_ptr->ibcast_last_seq_no_completed =
            MPL_malloc(MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS * sizeof(int), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!nb_release_gather_info_ptr->ibcast_last_seq_no_completed, mpi_errno,
                            MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIDU_shm_alloc(comm_ptr, ibcast_flags_shm_size,
                                    (void **) &(nb_release_gather_info_ptr->ibcast_flags_addr),
                                    &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        /* Calculate gather and release flag address and initialize to the gather and release states */
        for (i = 0; i < MPIR_CVAR_BCAST_INTRANODE_NUM_CELLS; i++) {
            MPL_atomic_release_store_uint64(MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_GATHER_FLAG_ADDR
                                            (rank, i, num_ranks), -1);
            MPL_atomic_release_store_uint64(MPIDI_POSIX_RELEASE_GATHER_NB_IBCAST_RELEASE_FLAG_ADDR
                                            (rank, i, num_ranks), -1);
            nb_release_gather_info_ptr->ibcast_last_seq_no_completed[i] = -1;
        }
        /* Allocate the shared memory for ibcast buffer */
        mpi_errno = MPIDU_shm_alloc(comm_ptr, MPIR_CVAR_BCAST_INTRANODE_BUFFER_TOTAL_SIZE,
                                    (void **) &(NB_RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr)),
                                    &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    if (initialize_ireduce_buf) {
        /* Allocate and initialize the flags and buffer for non-blocking reduce */
        nb_release_gather_info_ptr = &MPIDI_POSIX_COMM(comm_ptr, nb_release_gather);
        /* Array to keep track of last seq_no completed on each shm cell */
        nb_release_gather_info_ptr->ireduce_last_seq_no_completed =
            MPL_malloc(MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS * sizeof(int), MPL_MEM_COLL);
        MPIR_ERR_CHKANDJUMP(!nb_release_gather_info_ptr->ireduce_last_seq_no_completed, mpi_errno,
                            MPI_ERR_OTHER, "**nomem");

        NB_RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr) =
            MPL_malloc(num_ranks * sizeof(void *), MPL_MEM_COLL);

        mpi_errno = MPIDU_shm_alloc(comm_ptr, ireduce_flags_shm_size,
                                    (void **) &(nb_release_gather_info_ptr->ireduce_flags_addr),
                                    &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        for (i = 0; i < MPIR_CVAR_REDUCE_INTRANODE_NUM_CELLS; i++) {
            MPL_atomic_release_store_uint64(MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_GATHER_FLAG_ADDR
                                            (rank, i, num_ranks), -1);
            MPL_atomic_release_store_uint64(MPIDI_POSIX_RELEASE_GATHER_NB_IREDUCE_RELEASE_FLAG_ADDR
                                            (rank, i, num_ranks), -1);
            nb_release_gather_info_ptr->ireduce_last_seq_no_completed[i] = -1;
        }
        /* Allocate the shared memory for ireduce buffer */
        mpi_errno = MPIDU_shm_alloc(comm_ptr,
                                    num_ranks * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE,
                                    (void **) &(NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr)),
                                    &mapfail_flag);
        if (mpi_errno || mapfail_flag) {
            /* for communication errors, just record the error but continue */
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        /* Store address of each of the children's reduce buffer */
        char *addr;
        addr = NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr);
        for (i = 0; i < NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_tree.num_children); i++) {
            int child_rank =
                *(int *) utarray_eltptr(NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_tree).children, i);
            NB_RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr[i]) =
                addr + (child_rank * MPIR_CVAR_REDUCE_INTRANODE_BUFFER_TOTAL_SIZE);
        }
    }

    if (initialize_ibcast_buf || initialize_ireduce_buf) {
        /* Make sure all the flags are set before ranks start reading each other's flags from shm */
        mpi_errno = MPIR_Barrier_impl(comm_ptr, &errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

  fn_exit:
    MPIR_FUNC_EXIT;
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

/* Cleanup the release_gather data structures and free the allocated memory */
int MPIDI_POSIX_nb_release_gather_comm_free(MPIR_Comm * comm_ptr)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret = MPI_SUCCESS;
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;

    /* Clean up is not required for NULL struct */
    if (NB_RELEASE_GATHER_FIELD(comm_ptr, is_initialized) == 0) {
        goto fn_exit;
    }

    if (NB_RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr) != NULL) {
        /* destroy and detach the flags and buffer for Ibcast */
        mpi_errno = MPIDU_shm_free(NB_RELEASE_GATHER_FIELD(comm_ptr, ibcast_flags_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        /* destroy and detach shared memory used for bcast buffer */
        mpi_errno = MPIDU_shm_free(NB_RELEASE_GATHER_FIELD(comm_ptr, bcast_buf_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_free(NB_RELEASE_GATHER_FIELD(comm_ptr, ibcast_last_seq_no_completed));
    }

    if (NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr) != NULL) {
        /* destroy and detach the flags and buffer for Ireduce */
        mpi_errno = MPIDU_shm_free(NB_RELEASE_GATHER_FIELD(comm_ptr, ireduce_flags_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        mpi_errno = MPIDU_shm_free(NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_buf_addr));
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        MPL_free(NB_RELEASE_GATHER_FIELD(comm_ptr, child_reduce_buf_addr));
        MPL_free(NB_RELEASE_GATHER_FIELD(comm_ptr, ireduce_last_seq_no_completed));
    }

    MPIR_Treealgo_tree_free(&(NB_RELEASE_GATHER_FIELD(comm_ptr, bcast_tree)));
    MPIR_Treealgo_tree_free(&(NB_RELEASE_GATHER_FIELD(comm_ptr, reduce_tree)));

  fn_exit:
    MPIR_FUNC_EXIT;
    /* --BEGIN ERROR HANDLING-- */
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, errflag, "**coll_fail");
    /* --END ERROR HANDLING-- */
    return mpi_errno;
}
