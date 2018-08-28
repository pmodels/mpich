/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Header protection (i.e., IBCAST_TSP_SCATTERV_ALLGATHERV_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "tsp_namespace_def.h"
#include "../iallgatherv/iallgatherv_tsp_recexch_algos_prototypes.h"

/* Routine to schedule a scatter followed by recursive exchange based broadcast */
int MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(void *buffer, int count,
                                                    MPI_Datatype datatype, int root,
                                                    MPIR_Comm * comm, int scatterv_k,
                                                    int allgatherv_k, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    size_t extent, type_size;
    MPI_Aint true_lb, true_extent;
    int size, rank, tag;
    int i, j, x, is_contig;
    void *tmp_buf = NULL;
    int *cnts, *displs;
    size_t nbytes;
    int tree_type;
    MPIR_Treealgo_tree_t my_tree, parents_tree;
    int current_child, next_child, lrank, total_count, recv_id, sink_id;
    int num_children, *child_subtree_size = NULL;
    int recv_size, num_send_dependencies;
    MPIR_CHKLMEM_DECL(3);

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTERV_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTERV_ALLGATHERV);

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
    MPIR_CHKLMEM_MALLOC(cnts, int *, sizeof(int) * size, mpi_errno, "cnts", MPL_MEM_COLL);      /* to store counts of each rank */
    MPIR_CHKLMEM_MALLOC(displs, int *, sizeof(int) * size, mpi_errno, "displs", MPL_MEM_COLL);  /* to store displs of each rank */

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
        tmp_buf = (char *) buffer + true_lb;
    } else {
        tmp_buf = (void *) MPIR_TSP_sched_malloc(nbytes, sched);

        if (rank == root) {
            mpi_errno =
                MPIR_TSP_sched_localcopy(buffer, count, datatype, tmp_buf, nbytes, MPI_BYTE, sched,
                                         0, NULL);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_TSP_sched_fence(sched);
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
        recv_id =
            MPIR_TSP_sched_irecv((char *) tmp_buf + displs[rank], recv_size, MPI_BYTE,
                                 my_tree.parent, tag, comm, sched, 0, NULL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv", rank));
    }

    /* send data to children */
    for (i = 0; i < num_children; i++) {
        int child = *(int *) utarray_eltptr(my_tree.children, i);
        if (num_children > 0 && lrank != 0)
            num_send_dependencies = 1;
        else
            num_send_dependencies = 0;

        MPIR_TSP_sched_isend((char *) tmp_buf + displs[child],
                             child_subtree_size[i], MPI_BYTE,
                             child, tag, comm, sched, num_send_dependencies, &recv_id);
    }


    MPIR_Treealgo_tree_free(&my_tree);
    MPIR_TSP_sched_fence(sched);        /* wait for scatter to complete */

    /* Schedule Allgatherv */
    mpi_errno =
        MPIR_TSP_Iallgatherv_sched_intra_recexch(MPI_IN_PLACE, cnts[rank], MPI_BYTE, tmp_buf, cnts,
                                                 displs, MPI_BYTE, comm, 0, allgatherv_k, sched);
    MPIR_ERR_CHECK(mpi_errno);


    if (!is_contig) {
        if (rank != root) {
            sink_id = MPIR_TSP_sched_sink(sched);       /* wait for allgather to complete */

            mpi_errno =
                MPIR_TSP_sched_localcopy(tmp_buf, nbytes, MPI_BYTE, buffer, count, datatype, sched,
                                         1, &sink_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_SCHED_INTRA_SCATTERV_ALLGATHERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking scatter followed by recursive exchange allgather  based broadcast */
int MPIR_TSP_Ibcast_intra_scatterv_allgatherv(void *buffer, int count, MPI_Datatype datatype,
                                              int root, MPIR_Comm * comm, int scatterv_k,
                                              int allgatherv_k, MPIR_Request ** req)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTERV_ALLGATHERV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTERV_ALLGATHERV);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched, false);

    /* schedule scatter followed by recursive exchange allgather algo */
    mpi_errno =
        MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root, comm,
                                                        scatterv_k, allgatherv_k, sched);
    MPIR_ERR_CHECK(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IBCAST_INTRA_SCATTERV_ALLGATHERV);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
