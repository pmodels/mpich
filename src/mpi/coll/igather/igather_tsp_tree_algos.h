/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/* Header protection (i.e., IGATHER_TSP_TREE_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "treealgo.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a tree based gather */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Igathert_sched_intra_tree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Igather_sched_intra_tree(const void *sendbuf, int sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                      MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                      int k, MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int size, rank, lrank;
    int i, j, tag, is_inplace = false;
    size_t sendtype_lb, sendtype_extent, sendtype_true_extent;
    size_t recvtype_lb, recvtype_extent, recvtype_true_extent;
    int dtcopy_id, *recv_id = NULL;
    void *tmp_buf = NULL;
    const void *data_buf = NULL;
    int tree_type;
    MPII_Treealgo_tree_t my_tree, parents_tree;
    int next_child, num_children, *child_subtree_size = NULL, *child_data_offset = NULL;
    int offset, recv_size, num_dependencies;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IGATHER_SCHED_INTRA_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IGATHER_SCHED_INTRA_TREE);


    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    lrank = (rank - root + size) % size;        /* logical rank when root is non-zero */

    if (rank == root)
        is_inplace = (sendbuf == MPI_IN_PLACE); /* For gather, MPI_IN_PLACE is significant only at root */

    tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* currently only tree_type=MPIR_TREE_TYPE_KNOMIAL_1 is supported for gather */
    mpi_errno = MPII_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    num_children = my_tree.num_children;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (rank == root && is_inplace) {
        sendtype = recvtype;
        sendcount = recvcount;
    } else if (rank != root) {  /* all non root ranks will forward data in recvtype format */
        recvtype = sendtype;
        recvcount = sendcount;
    }

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Type_get_true_extent_impl(sendtype, &sendtype_lb, &sendtype_true_extent);
    sendtype_extent = MPL_MAX(sendtype_extent, sendtype_true_extent);

    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);
    MPIR_Type_get_true_extent_impl(recvtype, &recvtype_lb, &recvtype_true_extent);
    recvtype_extent = MPL_MAX(recvtype_extent, recvtype_true_extent);

    num_children = my_tree.num_children;
    child_subtree_size = MPL_malloc(sizeof(int) * num_children, MPL_MEM_COLL);  /* to store size of subtree of each child */
    child_data_offset = MPL_malloc(sizeof(int) * num_children, MPL_MEM_COLL);   /* to store the offset of the data to be sent to each child  */

    /* calculate size of subtree of each child */

    /* get tree information of the parent */
    if (my_tree.parent != -1) {
        MPII_Treealgo_tree_create(my_tree.parent, size, tree_type, k, root, &parents_tree);
    } else {    /* initialize an empty children array */
        utarray_new(parents_tree.children, &ut_int_icd, MPL_MEM_COLL);
        parents_tree.num_children = 0;
    }

    recv_size = 1;      /* total size of the data to be received from the parent.
                         * 1 is to count yourself, now add the size of each child's subtree */
    for (i = 0; i < num_children; i++) {
        int current_child = (*(int *) utarray_eltptr(my_tree.children, i) - root + size) % size;

        if (i < num_children - 1) {
            next_child = (*(int *) utarray_eltptr(my_tree.children, i + 1) - root + size) % size;
        } else {
            next_child = size;
            for (j = 0; j < parents_tree.num_children; j++) {
                int sibling =
                    (*(int *) utarray_eltptr(parents_tree.children, j) - root + size) % size;
                if (sibling > lrank) {  /* This is the first sibling larger than lrank */
                    next_child = sibling;
                    break;
                }
            }
        }

        child_subtree_size[i] = next_child - current_child;
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "i:%d rank:%d current_child:%d, next_child:%d, child_subtree_size[i]:%d, recv_size:%d\n",
                         i, rank, current_child, next_child, child_subtree_size[i], recv_size));
        recv_size += child_subtree_size[i];
    }

    MPII_Treealgo_tree_free(&parents_tree);

    recv_size *= (lrank == 0) ? recvcount : sendcount;
    offset = (lrank == 0) ? recvcount : sendcount;

    for (i = 0; i < num_children; i++) {
        child_data_offset[i] = offset;
        offset += (child_subtree_size[i] * ((lrank == 0) ? recvcount : sendcount));
    }


    if (root != 0 && lrank == 0) {
        tmp_buf = MPIR_TSP_sched_malloc(recv_size * recvtype_extent, sched);
    } else if (root == 0 && lrank == 0) {
        tmp_buf = (void *) recvbuf;
    } else if (num_children > 0 && lrank != 0) {        /* intermediate ranks in the tree */
        tmp_buf = MPIR_TSP_sched_malloc(recv_size * sendtype_extent, sched);
    } else {    /* leaf ranks in the tree */
        tmp_buf = (void *) sendbuf;
    }

    recv_id = MPL_malloc(sizeof(int) * num_children, MPL_MEM_COLL);
    /* Leaf nodes send to parent */
    if (num_children == 0) {
        MPIR_TSP_sched_isend(tmp_buf, sendcount, sendtype, my_tree.parent,
                             tag, comm, sched, 0, NULL);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv\n", rank));
    } else {
        num_dependencies = 0;
        if (tmp_buf != recvbuf || (lrank == 0 && !is_inplace)) {        /* copy data into tmp buf unless tmp_buf is recvbuf and inplace
                                                                         * in which case data is already located in the correct position */
            if (lrank == 0 && root != 0 && is_inplace)
                data_buf = (char *) recvbuf + root * recvcount * recvtype_extent;
            else
                data_buf = (void *) sendbuf;

            dtcopy_id =
                MPIR_TSP_sched_localcopy(data_buf, sendcount, sendtype,
                                         tmp_buf, recvcount, recvtype, sched, 0, NULL);
            num_dependencies++;
        }

        for (i = 0; i < num_children; i++) {
            int child = *(int *) utarray_eltptr(my_tree.children, i);
            recv_id[i] =
                MPIR_TSP_sched_irecv((char *) tmp_buf + child_data_offset[i] * recvtype_extent,
                                     child_subtree_size[i] * recvcount, recvtype, child, tag, comm,
                                     sched, num_dependencies, &dtcopy_id);
        }

        if (my_tree.parent != -1)
            MPIR_TSP_sched_isend(tmp_buf, recv_size, recvtype, my_tree.parent,
                                 tag, comm, sched, num_children, recv_id);

    }

    if (lrank == 0 && root != 0) {      /* root puts data in recvbuf */
        dtcopy_id = MPIR_TSP_sched_localcopy(tmp_buf, (size - root) * recvcount, recvtype,
                                             (char *) recvbuf + root * recvcount * recvtype_extent,
                                             (size - root) * recvcount, recvtype, sched,
                                             num_children, recv_id);

        MPIR_TSP_sched_localcopy((char *) tmp_buf + (size - root) * recvcount * recvtype_extent,
                                 root * recvcount, recvtype, recvbuf,
                                 root * recvcount, recvtype, sched, 1, &dtcopy_id);

    }

    MPII_Treealgo_tree_free(&my_tree);

  fn_exit:
    MPL_free(child_subtree_size);
    MPL_free(child_data_offset);
    MPL_free(recv_id);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IGATHER_SCHED_INTRA_TREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking tree based gather */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Igather_intra_tree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Igather_intra_tree(const void *sendbuf, int sendcount,
                                MPI_Datatype sendtype, void *recvbuf, int recvcount,
                                MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                MPIR_Request ** req, int k)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IGATHER_INTRA_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IGATHER_INTRA_TREE);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* schedule tree algo */
    mpi_errno = MPIR_TSP_Igather_sched_intra_tree(sendbuf, sendcount, sendtype,
                                                  recvbuf, recvcount, recvtype,
                                                  root, comm, k, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IGATHER_INTRA_TREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
