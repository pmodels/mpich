/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule a pipelined tree based reduce */
int MPIR_TSP_Ireduce_sched_intra_tree(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                      MPI_Datatype datatype, MPI_Op op, int root,
                                      MPIR_Comm * comm, int tree_type, int k, int chunk_size,
                                      int buffer_per_child, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int i, j, t;
    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    size_t extent, type_size;
    MPI_Aint type_lb, true_extent;
    int is_commutative;
    int size;
    int rank, vtx_id;
    int num_children = 0;
    int tree_root;              /* Root of the tree over which reduction is performed.
                                 * This can be different from root of the collective operation.
                                 * When root is non-zero and op is non-commutative, the reduction
                                 * is first performed with rank 0 as the root of the tree and then
                                 * rank 0 sends the reduced data to the root of the ecollective op. */
    int is_tree_root, is_tree_leaf, is_tree_intermediate;       /* Variables to store location of this rank in the tree */
    int is_root;
    MPIR_Treealgo_tree_t my_tree;
    void **child_buffer = NULL; /* Buffer array in which data from children is received */
    void *reduce_buffer;        /* Buffer in which reduced data is present */
    int *vtcs = NULL, *recv_id = NULL, *reduce_id = NULL;       /* Arrays to store graph vertex ids */
    int dtcopy_id = -1;
    int nvtcs;
    int tag;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_ENTER;

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    is_root = (rank == root);

    /* main algorithm */
    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &type_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    /* calculate chunking information for pipelining */
    MPIR_Algo_calculate_pipeline_chunk_info(chunk_size, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    if (!is_commutative) {
        if (k > 1)
            tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* Force tree_type to be knomial_1 because kary trees cannot
                                                         * handle non-commutative operations correctly */
        tree_root = 0;  /* Force the tree root to be rank 0 */
    } else {
        tree_root = root;
    }

    /* initialize the tree */
    my_tree.children = NULL;
    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, k, tree_root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    /* identify my location in the tree */
    is_tree_root = (rank == tree_root) ? 1 : 0;
    is_tree_leaf = (num_children == 0) ? 1 : 0;
    is_tree_intermediate = (!is_tree_leaf && !is_tree_root);
    /* ensure exclusiveness */
    MPIR_Assert(!(is_tree_root && is_tree_leaf));

    /* Allocate buffers to receive data from children. Any memory required for execution
     * of the schedule, for example child_buffer, reduce_buffer below, is allocated using
     * MPIR_TSP_sched_malloc function. This function stores the allocated memory address
     * in the schedule and then frees it once the schedule completes execution. The
     * programmer need not free this memory. This is unlike the MPL_malloc function that
     * will be used for any memory required to generate the schedule and then freed by
     * the programmer once that memory is no longer required */
    if (!is_tree_leaf) {
        child_buffer = MPIR_TSP_sched_malloc(sizeof(void *) * num_children, sched);
        child_buffer[0] = MPIR_TSP_sched_malloc(extent * count, sched);
        child_buffer[0] = (void *) ((char *) child_buffer[0] - type_lb);
        for (i = 1; i < num_children; i++) {
            if (buffer_per_child) {
                child_buffer[i] = MPIR_TSP_sched_malloc(extent * count, sched);
                child_buffer[i] = (void *) ((char *) child_buffer[i] - type_lb);
            } else {
                child_buffer[i] = child_buffer[0];
            }
        }
    }

    /* Set reduce_buffer based on location in the tree */
    /* FIXME: This can also be pipelined along with rest of the reduction */
    if (is_tree_root && is_root) {
        reduce_buffer = recvbuf;
        if (sendbuf != MPI_IN_PLACE)
            mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype,
                                                 sched, 0, NULL, &dtcopy_id);
    } else if (is_tree_root && !is_root) {
        reduce_buffer = MPIR_TSP_sched_malloc(extent * count, sched);
        reduce_buffer = (void *) ((char *) reduce_buffer - type_lb);
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype, reduce_buffer, count,
                                             datatype, sched, 0, NULL, &dtcopy_id);
    } else if (is_tree_intermediate && is_root) {
        reduce_buffer = recvbuf;
        if (sendbuf != MPI_IN_PLACE)
            mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype, recvbuf, count, datatype,
                                                 sched, 0, NULL, &dtcopy_id);
    } else if (is_tree_intermediate && !is_root) {
        reduce_buffer = MPIR_TSP_sched_malloc(extent * count, sched);
        reduce_buffer = (void *) ((char *) reduce_buffer - type_lb);
        mpi_errno = MPIR_TSP_sched_localcopy(sendbuf, count, datatype, reduce_buffer, count,
                                             datatype, sched, 0, NULL, &dtcopy_id);
    } else if (is_tree_leaf && is_root) {
        if (sendbuf == MPI_IN_PLACE)
            reduce_buffer = recvbuf;
        else
            reduce_buffer = (void *) sendbuf;
    } else {    /* is_tree_leaf && !is_root */
        reduce_buffer = (void *) sendbuf;
    }

    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    /* initialize arrays to store graph vertex indices */
    MPIR_CHKLMEM_MALLOC(vtcs, int *, sizeof(int) * (num_children + 1),
                        mpi_errno, "vtcs buffer", MPL_MEM_COLL);
    if (num_children > 0) {
        MPIR_CHKLMEM_MALLOC(reduce_id, int *, sizeof(int) * num_children,
                            mpi_errno, "reduce_id buffer", MPL_MEM_COLL);
        MPIR_CHKLMEM_MALLOC(recv_id, int *, sizeof(int) * num_children,
                            mpi_errno, "recv_id buffer", MPL_MEM_COLL);
    }

    /* do pipelined reduce */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined reduce, for example, buffer+offset
     * if being calculated correctly */
    for (j = 0; j < num_chunks; j++) {
        MPI_Aint msgsize = (j == 0) ? chunk_size_floor : chunk_size_ceil;
        void *reduce_address = (char *) reduce_buffer + offset * extent;

        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        for (i = 0; i < num_children; i++) {
            void *recv_address = (char *) child_buffer[i] + offset * extent;
            int child = *(int *) utarray_eltptr(my_tree.children, i);

            /* Setup the dependencies for posting receive for child's data */
            if (buffer_per_child || i == 0) {   /* no dependency, just post the receive */
                if (dtcopy_id >= 0) {
                    vtcs[0] = dtcopy_id;
                    nvtcs = 1;
                } else {
                    nvtcs = 0;
                }
            } else {
                vtcs[0] = reduce_id[i - 1];
                nvtcs = 1;
            }

            mpi_errno = MPIR_TSP_sched_irecv(recv_address, msgsize, datatype, child, tag, comm,
                                             sched, nvtcs, vtcs, &recv_id[i]);

            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            /* Setup dependencies for reduction. Reduction depends on the corresponding recv to complete */
            vtcs[0] = recv_id[i];
            nvtcs = 1;

            if (is_commutative) {       /* reduction order does not matter */
                mpi_errno = MPIR_TSP_sched_reduce_local(recv_address, reduce_address, msgsize,
                                                        datatype, op, sched, nvtcs, vtcs,
                                                        &reduce_id[i]);
            } else {    /* wait for the previous reduce to complete */

                /* NOTE: Make sure that knomial tree is being constructed differently for reduce for optimal performance.
                 * In bcast, leftmost subtree is the largest while it should be the opposite in case of reduce */

                if (i > 0) {
                    vtcs[nvtcs] = reduce_id[i - 1];
                    nvtcs++;
                }
                mpi_errno =
                    MPIR_TSP_sched_reduce_local(reduce_address, recv_address, msgsize, datatype, op,
                                                sched, nvtcs, vtcs, &reduce_id[i]);
                mpi_errno =
                    MPIR_TSP_sched_localcopy(recv_address, msgsize, datatype, reduce_address,
                                             msgsize, datatype, sched, 1, &reduce_id[i], &vtx_id);
                reduce_id[i] = vtx_id;

            }
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        if (is_commutative && buffer_per_child) {       /* wait for all the reductions */
            nvtcs = num_children;
            for (t = 0; t < num_children; t++)
                vtcs[t] = reduce_id[t];
        } else {        /* wait for the last reduction */
            nvtcs = (num_children) ? 1 : 0;
            if (num_children)
                vtcs[0] = reduce_id[num_children - 1];
        }

        /* send data to the parent */
        if (!is_tree_root) {
            mpi_errno =
                MPIR_TSP_sched_isend(reduce_address, msgsize, datatype, my_tree.parent, tag, comm,
                                     sched, nvtcs, vtcs, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        /* send data to the root of the collective operation */
        if (tree_root != root) {
            if (is_tree_root) { /* tree_root sends data to root */
                mpi_errno =
                    MPIR_TSP_sched_isend(reduce_address, msgsize, datatype, root, tag, comm, sched,
                                         nvtcs, vtcs, &vtx_id);
            } else if (is_root) {       /* root receives data from tree_root */
                mpi_errno =
                    MPIR_TSP_sched_irecv((char *) recvbuf + offset * extent, msgsize, datatype,
                                         tree_root, tag, comm, sched, 0, NULL, &vtx_id);
            }
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
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
