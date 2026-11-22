/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"
#include "ibcast.h"

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
    int num_children;
    MPIR_Treealgo_tree_t my_tree;
    int tag;
    int *send_id = NULL;        /* the last send to each child */
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
        MPIR_CHKLMEM_MALLOC(send_id, num_children * sizeof(int));
    }

    /* do pipelined tree broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined broadcast, for example, buffer+offset
     * if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;
#ifdef HAVE_ERROR_CHECKING
        struct MPII_Ibcast_state *ibcast_state = NULL;

        ibcast_state = MPIR_TSP_sched_malloc(sizeof(struct MPII_Ibcast_state), sched);
        MPIR_ERR_CHKANDJUMP(ibcast_state == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
        ibcast_state->n_bytes = msgsize * type_size;
#endif

        tag = MPIR_TSP_sched_get_tag(sched);

        /* Receive message from parent */
        if (my_tree.parent != -1) {
#ifdef HAVE_ERROR_CHECKING
            mpi_errno =
                MPIR_TSP_sched_irecv_status((char *) buffer + offset * extent, msgsize,
                                            datatype, my_tree.parent, tag, comm,
                                            &ibcast_state->status, sched, 0, NULL, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);
            int vtx_id;
            MPIR_TSP_sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, sched, 1, &recv_id,
                              &vtx_id);
#else
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) buffer + offset * extent, msgsize, datatype,
                                     my_tree.parent, tag, comm, sched, 0, NULL, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);
#endif
        }

        /* Send the chunk to each child on its own, once it is received (except at the root),
         * and issued after the send of the previous chunk to that child, so that the chunks go
         * to each child in order. A multicast would make all children wait for the slowest. */
        for (int c = 0; c < num_children; c++) {
            int prev_send_id = send_id[c];
            mpi_errno = MPIR_TSP_sched_isend((char *) buffer + offset * extent, msgsize, datatype,
                                             ut_int_array(my_tree.children)[c], tag, comm, sched,
                                             (my_tree.parent != -1) ? 1 : 0, &recv_id, &send_id[c]);
            MPIR_ERR_CHECK(mpi_errno);
            if (i > 0) {
                mpi_errno = MPIR_TSP_sched_add_dep(sched, send_id[c], prev_send_id,
                                                   MPIR_TSP_DEP_ISSUE);
                MPIR_ERR_CHECK(mpi_errno);
            }
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
