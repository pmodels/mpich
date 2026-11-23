/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"
#include "ibcast.h"

/* At most WINDOW chunks outstanding per peer, for sends and for receives: without the bound, every
 * chunk's operation would be issued at once, and each progress pass walks all the issued
 * operations. Every SYNC_INTERVAL-th send to a child would be synchronous (0: none). A child
 * receives only from its parent, in order, so the chunks waiting unmatched there are found at the
 * head of its queue and cost no search; bounding them would only cost acknowledgments. */
#define WINDOW 2
#define SYNC_INTERVAL 0

/* Routine to schedule a pipelined tree based broadcast */
int MPIR_TSP_Ibcast_sched_intra_tree(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm, int tree_type, int k, int chunk_size,
                                     MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    MPI_Aint extent, type_size;
    int size;
    int rank;
    int recv_id;
    int recv_ids[WINDOW + 1];   /* the last WINDOW receives (see MPIR_TSP_sched_add_window_deps) */
    int num_children;
    MPIR_Treealgo_tree_t my_tree;
    int *send_id = NULL;        /* the last WINDOW sends to each child, and its last synchronous one */
    MPIR_CHKLMEM_DECL();
    MPIR_FUNC_ENTER;

    MPIR_COMM_RANK_SIZE(comm, rank, size);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);

    /* calculate chunking information for pipelining */
    MPIR_Algo_calculate_pipeline_chunk_info(chunk_size, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;
    if (num_children > 0) {
        MPIR_CHKLMEM_MALLOC(send_id, (WINDOW + 1) * num_children * sizeof(int));
    }

    /* do pipelined tree broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined broadcast, for example, buffer+offset
     * if being calculated correctly */
    /* checking the size received in each chunk, only when error checking is on */
    bool check = MPIR_ERROR_CHECKS_ENABLED;
    for (i = 0; i < num_chunks; i++) {
        MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;

        /* Receive message from parent */
        if (my_tree.parent != -1) {
            if (check) {
                struct MPII_Ibcast_state *ibcast_state = NULL;

                ibcast_state = MPIR_TSP_sched_malloc(sizeof(struct MPII_Ibcast_state), sched);
                MPIR_ERR_CHKANDJUMP(ibcast_state == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
                ibcast_state->n_bytes = msgsize * type_size;
                mpi_errno = MPIR_TSP_sched_irecv_status((char *) buffer + offset * extent, msgsize,
                                                        datatype, my_tree.parent, comm,
                                                        &ibcast_state->status, sched, 0, NULL,
                                                        &recv_id);
                MPIR_ERR_CHECK(mpi_errno);
                int vtx_id;
                MPIR_TSP_sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, sched, 1,
                                  &recv_id, &vtx_id);
            } else {
                mpi_errno = MPIR_TSP_sched_irecv((char *) buffer + offset * extent, msgsize,
                                                 datatype, my_tree.parent, comm, sched, 0, NULL,
                                                 &recv_id);
                MPIR_ERR_CHECK(mpi_errno);
            }
            mpi_errno = MPIR_TSP_sched_add_window_deps(sched, i, recv_id, recv_ids, WINDOW, 0);
            MPIR_ERR_CHECK(mpi_errno);
        }

        /* Send the chunk to each child on its own, once it is received (except at the root). A
         * multicast would make all children wait for the slowest. */
        bool sync = MPIR_Algo_pipeline_sync(i, SYNC_INTERVAL);
        for (int c = 0; c < num_children; c++) {
            void *buf = (char *) buffer + offset * extent;
            int child = MPIR_Treealgo_tree_children(&my_tree)[c];
            int n_in = (my_tree.parent != -1) ? 1 : 0;
            int vtx_id;
            if (sync) {
                mpi_errno = MPIR_TSP_sched_issend(buf, msgsize, datatype, child, comm, sched, n_in,
                                                  &recv_id, &vtx_id);
            } else {
                mpi_errno = MPIR_TSP_sched_isend(buf, msgsize, datatype, child, comm, sched, n_in,
                                                 &recv_id, &vtx_id);
            }
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_TSP_sched_add_window_deps(sched, i, vtx_id, send_id + (WINDOW + 1) * c,
                                                       WINDOW, SYNC_INTERVAL);
            MPIR_ERR_CHECK(mpi_errno);
        }
        offset += msgsize;
    }

    MPIR_Treealgo_tree_free(&my_tree);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_TSP_Ibcast_sched_intra_ring(void *buffer, MPI_Aint count, MPI_Datatype datatype, int root,
                                     MPIR_Comm * comm, int chunk_size, MPIR_TSP_sched_t sched)
{
    return MPIR_TSP_Ibcast_sched_intra_tree(buffer, count, datatype, root, comm,
                                            MPIR_TREE_TYPE_KARY, 1, chunk_size, sched);
}
