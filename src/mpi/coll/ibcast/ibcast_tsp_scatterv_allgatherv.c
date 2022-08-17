/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"

/* Routine to schedule a scatter followed by recursive exchange based broadcast */
int MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(void *buffer, MPI_Aint count,
                                                    MPI_Datatype datatype, int root,
                                                    MPIR_Comm * comm, int allgatherv_algo,
                                                    int scatterv_k, int allgatherv_k,
                                                    MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    size_t extent, type_size;
    MPI_Aint true_lb, true_extent;
    int size, rank, tag;
    int i, j, x, is_contig;
    void *tmp_buf = NULL;
    MPI_Aint *cnts, *displs;
    size_t nbytes;
    int tree_type, vtx_id, recv_id;
    MPIR_Treealgo_tree_t my_tree, parents_tree;
    int current_child, next_child, lrank, total_count, sink_id;
    int num_children, *child_subtree_size = NULL;
    int num_send_dependencies;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(3);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_ENTER;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranks, root=%d\n",
                     MPIR_Comm_size(comm), root));

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    lrank = (rank - root + size) % size;        /* logical rank when root is non-zero */

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_is_contig(datatype, &is_contig);
    extent = MPL_MAX(extent, true_extent);

    nbytes = type_size * count;
    MPIR_CHKLMEM_MALLOC(cnts, MPI_Aint *, sizeof(MPI_Aint) * size, mpi_errno, "cnts", MPL_MEM_COLL);    /* to store counts of each rank */
    MPIR_CHKLMEM_MALLOC(displs, MPI_Aint *, sizeof(MPI_Aint) * size, mpi_errno, "displs", MPL_MEM_COLL);        /* to store displs of each rank */

    total_count = 0;
    for (i = 0; i < size; i++)
        cnts[i] = 0;
    for (i = 0; i < size; i++) {
        cnts[i] = (nbytes + size - 1) / size;
        if (total_count + cnts[i] > nbytes) {
            cnts[i] = nbytes - total_count;
            break;
        }
        total_count += cnts[i];
    }
    displs[0] = 0;
    for (i = 1; i < size; i++) {
        displs[i] = displs[i - 1] + cnts[i - 1];
    }


    if (is_contig) {
        /* contiguous. no need to pack. */
        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        tmp_buf = (void *) MPIR_TSP_sched_malloc(nbytes, sched);

        if (rank == root) {
            mpi_errno =
                MPIR_TSP_sched_localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, sched,
                                         0, NULL, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_TSP_sched_fence(sched);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }
    }

    /* knomial scatter for bcast */
    tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* currently only tree_type=MPIR_TREE_TYPE_KNOMIAL_1 is supported for scatter */
    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, scatterv_k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    MPIR_CHKLMEM_MALLOC(child_subtree_size, int *, sizeof(int) * num_children, mpi_errno, "child_subtree_size buffer", MPL_MEM_COLL);   /* to store size of subtree of each child */
    /* calculate size of subtree of each child */

    /* get tree information of the parent */
    if (my_tree.parent != -1) {
        mpi_errno =
            MPIR_Treealgo_tree_create(my_tree.parent, size, tree_type, scatterv_k, root,
                                      &parents_tree);
        MPIR_ERR_CHECK(mpi_errno);
    } else {    /* initialize an empty children array */
        parents_tree.num_children = 0;
    }

    MPI_Aint recv_size;
    recv_size = cnts[rank];
    /* total size of the data to be received from the parent. */
    for (i = 0; i < num_children; i++) {
        current_child = ((*(int *) utarray_eltptr(my_tree.children, i)) - root + size) % size;  /* Dereference and calculate the current child */

        if (i < num_children - 1) {
            next_child = ((*(int *) utarray_eltptr(my_tree.children, i + 1)) - root + size) % size;     /* Dereference and calculate next child */
        } else {
            next_child = size;
            for (j = 0; j < parents_tree.num_children; j++) {
                int sibling =
                    ((*(int *) utarray_eltptr(parents_tree.children, j)) - root + size) % size;
                if (sibling > lrank) {  /* This is the first sibling larger than lrank */
                    next_child = sibling;
                    break;
                }
            }
        }
        child_subtree_size[i] = 0;
        for (x = current_child; x < next_child; x++) {
            child_subtree_size[i] += cnts[x];
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "i:%d rank:%d current_child:%d, next_child:%d, child_subtree_size[i]:%d, recv_size:%d",
                         i, rank, current_child, next_child, child_subtree_size[i], recv_size));
        recv_size += child_subtree_size[i];
    }
    if (my_tree.parent != -1)
        MPIR_Treealgo_tree_free(&parents_tree);

    /* receive data from the parent */
    if (my_tree.parent != -1) {
        mpi_errno =
            MPIR_TSP_sched_irecv((char *) tmp_buf + displs[rank], recv_size, MPI_BYTE,
                                 my_tree.parent, tag, comm, sched, 0, NULL, &recv_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv", rank));

    }

    /* send data to children */
    for (i = 0; i < num_children; i++) {
        int child = *(int *) utarray_eltptr(my_tree.children, i);
        if (num_children > 0 && lrank != 0)
            num_send_dependencies = 1;
        else
            num_send_dependencies = 0;

        mpi_errno = MPIR_TSP_sched_isend((char *) tmp_buf + displs[child],
                                         child_subtree_size[i], MPI_BYTE,
                                         child, tag, comm, sched, num_send_dependencies, &recv_id,
                                         &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }


    MPIR_Treealgo_tree_free(&my_tree);
    mpi_errno = MPIR_TSP_sched_fence(sched);    /* wait for scatter to complete */
    MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

    if (allgatherv_algo == MPIR_CVAR_IALLGATHERV_INTRA_ALGORITHM_tsp_ring)
        /* Schedule Allgatherv ring */
        mpi_errno =
            MPIR_TSP_Iallgatherv_sched_intra_ring(MPI_IN_PLACE, cnts[rank], MPI_BYTE, tmp_buf,
                                                  cnts, displs, MPI_BYTE, comm, sched);
    else
        /* Schedule Allgatherv recexch */
        mpi_errno =
            MPIR_TSP_Iallgatherv_sched_intra_recexch(MPI_IN_PLACE, cnts[rank], MPI_BYTE, tmp_buf,
                                                     cnts, displs, MPI_BYTE, comm, 0, allgatherv_k,
                                                     sched);
    MPIR_ERR_CHECK(mpi_errno);

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_TSP_sched_sink(sched, &sink_id);   /* wait for allgather to complete */
            if (mpi_errno)
                MPIR_ERR_POP(mpi_errno);

            mpi_errno =
                MPIR_TSP_sched_localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, sched,
                                         1, &sink_id, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
