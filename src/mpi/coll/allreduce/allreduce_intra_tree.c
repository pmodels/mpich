/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Copyright (C) by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */


#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

int MPIR_Allreduce_intra_tree(const void *sendbuf,
                              void *recvbuf,
                              MPI_Aint count,
                              MPI_Datatype datatype,
                              MPI_Op op, MPIR_Comm * comm_ptr,
                              int tree_type, int k, int chunk_size,
                              int buffer_per_child, MPIR_Errflag_t * errflag)
{
    int comm_size, rank;
    int mpi_errno = MPI_SUCCESS, mpi_errno_ret = MPI_SUCCESS;
    int is_commutative;
    MPI_Aint true_extent, type_lb;
    void **child_buffer = NULL; /* Buffer array in which data from children is received */
    void *reduce_buffer;        /* Buffer in which allreduced data is present */
    MPIR_Treealgo_tree_t my_tree;

    MPI_Aint num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    size_t extent, type_size;
    int num_children;
    int root = 0;
    bool is_tree_leaf;
    int i, j, tag;

    MPIR_Request **reqs;
    int num_reqs = 0;

    comm_size = MPIR_Comm_size(comm_ptr);
    rank = MPIR_Comm_rank(comm_ptr);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &type_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);

    MPIR_CHKLMEM_DECL(2);

    /* copy local data into recvbuf */
    if (sendbuf != MPI_IN_PLACE) {
        mpi_errno = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* calculate chunking information for pipelining */
    MPIR_Algo_calculate_pipeline_chunk_info(chunk_size, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);

    if (!is_commutative) {
        /* because kary and knomial_2 trees cannot handle non-commutative operations */
        if (tree_type == MPIR_TREE_TYPE_KNOMIAL_2 || tree_type == MPIR_TREE_TYPE_KARY) {
            tree_type = MPIR_TREE_TYPE_KNOMIAL_1;
        }
    }
    /* initialize the tree */
    mpi_errno = MPIR_Treealgo_tree_create(rank, comm_size, tree_type, k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    /* identify my location in the tree */
    is_tree_leaf = (num_children == 0) ? 1 : 0;

    if (!is_tree_leaf) {
        MPIR_CHKLMEM_MALLOC(child_buffer, void **, sizeof(void *) * num_children, mpi_errno,
                            "child_buffer", MPL_MEM_BUFFER);
        child_buffer[0] = MPL_malloc(extent * count, MPL_MEM_BUFFER);
        MPIR_ERR_CHKANDJUMP(!child_buffer[0], mpi_errno, MPI_ERR_OTHER, "**nomem");

        child_buffer[0] = (void *) ((char *) child_buffer[0] - type_lb);
        MPIR_ERR_CHKANDJUMP(!child_buffer[0], mpi_errno, MPI_ERR_OTHER, "**nomem");
        for (i = 1; i < num_children; i++) {
            if (buffer_per_child) {
                child_buffer[i] = MPL_malloc(extent * count, MPL_MEM_BUFFER);
                MPIR_ERR_CHKANDJUMP(!child_buffer[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
                child_buffer[i] = (void *) ((char *) child_buffer[i] - type_lb);
                MPIR_ERR_CHKANDJUMP(!child_buffer[i], mpi_errno, MPI_ERR_OTHER, "**nomem");
            } else {
                child_buffer[i] = child_buffer[0];
            }
        }
    }
    reduce_buffer = recvbuf;

    MPIR_CHKLMEM_MALLOC(reqs, MPIR_Request **,
                        (num_children * num_chunks + num_chunks + 10) * sizeof(MPIR_Request *),
                        mpi_errno, "reqs", MPL_MEM_BUFFER);

    for (j = 0; j < num_chunks; j++) {
        MPI_Aint msgsize = (j == 0) ? chunk_size_floor : chunk_size_ceil;
        void *reduce_address = (char *) reduce_buffer + offset * extent;
        MPIR_ERR_CHKANDJUMP(!reduce_address, mpi_errno, MPI_ERR_OTHER, "**nomem");

        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);
        MPIR_ERR_CHECK(mpi_errno);

        for (i = 0; i < num_children; i++) {
            void *recv_address = (char *) child_buffer[i] + offset * extent;
            MPIR_ERR_CHKANDJUMP(!recv_address, mpi_errno, MPI_ERR_OTHER, "**nomem");
            int child = *(int *) utarray_eltptr(my_tree.children, i);
            MPIR_ERR_CHKANDJUMP(!child, mpi_errno, MPI_ERR_OTHER, "**nomem");

            mpi_errno =
                MPIC_Recv(recv_address, msgsize, datatype, child, MPIR_ALLREDUCE_TAG, comm_ptr,
                          MPI_STATUS_IGNORE, errflag);
            /* for communication errors, just record the error but continue */
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);

            if (is_commutative) {
                mpi_errno = MPIR_Reduce_local(recv_address, reduce_address, msgsize, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);

            } else {
                mpi_errno = MPIR_Reduce_local(reduce_address, recv_address, msgsize, datatype, op);
                MPIR_ERR_CHECK(mpi_errno);

                mpi_errno =
                    MPIR_Localcopy(recv_address, msgsize, datatype, reduce_address, msgsize,
                                   datatype);
                MPIR_ERR_CHECK(mpi_errno);
            }

        }
        if (rank != root) {     /* send data to the parent */
            mpi_errno =
                MPIC_Isend(reduce_address, msgsize, datatype, my_tree.parent, MPIR_ALLREDUCE_TAG,
                           comm_ptr, &reqs[num_reqs++], errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        }

        if (my_tree.parent != -1) {
            mpi_errno = MPIC_Recv(reduce_address, msgsize,
                                  datatype, my_tree.parent, MPIR_ALLREDUCE_TAG, comm_ptr,
                                  MPI_STATUS_IGNORE, errflag);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
        }
        if (num_children) {
            for (i = 0; i < num_children; i++) {
                int child = *(int *) utarray_eltptr(my_tree.children, i);
                MPIR_ERR_CHKANDJUMP(!child, mpi_errno, MPI_ERR_OTHER, "**nomem");
                MPIR_Assert(child != 0);
                mpi_errno = MPIC_Isend(reduce_address, msgsize,
                                       datatype, child,
                                       MPIR_ALLREDUCE_TAG, comm_ptr, &reqs[num_reqs++], errflag);
                MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
            }
        }

        offset += msgsize;
    }

    if (num_reqs > 0) {
        mpi_errno = MPIC_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE, errflag);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, *errflag);
    }

    if (!is_tree_leaf) {
        MPL_free(child_buffer[0]);
        for (i = 1; i < num_children; i++) {
            if (buffer_per_child) {
                MPL_free(child_buffer[i]);
            }
        }
    }
    MPIR_Treealgo_tree_free(&my_tree);
    MPIR_CHKLMEM_FREEALL();

  fn_exit:
    if (mpi_errno_ret)
        mpi_errno = mpi_errno_ret;
    else if (*errflag != MPIR_ERR_NONE)
        MPIR_ERR_SET(mpi_errno, *errflag, "**coll_fail");
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
