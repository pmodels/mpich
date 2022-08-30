/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    MPI_Aint extent, type_size;
    MPI_Aint lb, true_extent;
    int size;
    int rank;
    int recv_id;
    int num_children;
    MPIR_Treealgo_tree_t my_tree;
    int tag, vtx_id;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;

    MPIR_FUNC_ENTER;

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);

    /* calculate chunking information for pipelining */
    MPIR_Algo_calculate_pipeline_chunk_info(chunk_size, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    /* do pipelined tree broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined broadcast, for example, buffer+offset
     * if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        MPI_Aint msgsize = (i == 0) ? chunk_size_floor : chunk_size_ceil;
#ifdef HAVE_ERROR_CHECKING
        struct MPII_Ibcast_state *ibcast_state = NULL;

        ibcast_state = MPIR_TSP_sched_malloc(sizeof(struct MPII_Ibcast_state), sched);
        if (ibcast_state == NULL)
            MPIR_ERR_POP(mpi_errno);
        ibcast_state->n_bytes = msgsize * type_size;
#endif

        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        /* Receive message from parent */
        if (my_tree.parent != -1) {
#ifdef HAVE_ERROR_CHECKING
            mpi_errno =
                MPIR_TSP_sched_irecv_status((char *) buffer + offset * extent, msgsize,
                                            datatype, my_tree.parent, tag, comm,
                                            &ibcast_state->status, sched, 0, NULL, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            MPIR_TSP_sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, sched, 1, &recv_id,
                              &vtx_id);
#else
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) buffer + offset * extent, msgsize, datatype,
                                     my_tree.parent, tag, comm, sched, 0, NULL, &recv_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
#endif
        }

        if (num_children) {
            /* Multicast data to the children */
            mpi_errno = MPIR_TSP_sched_imcast((char *) buffer + offset * extent, msgsize, datatype,
                                              ut_int_array(my_tree.children), num_children, tag,
                                              comm, sched, (my_tree.parent != -1) ? 1 : 0, &recv_id,
                                              &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
        offset += msgsize;
    }

    MPIR_Treealgo_tree_free(&my_tree);

  fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
