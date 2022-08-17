/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "treealgo.h"

/* Routine to schedule a tree based gather */
int MPIR_TSP_Igather_sched_intra_tree(const void *sendbuf, MPI_Aint sendcount,
                                      MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                      MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                      int k, MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int size, rank, lrank;
    int i, j, tag, is_inplace = false;
    MPI_Aint sendtype_lb, sendtype_extent, sendtype_true_extent;
    MPI_Aint recvtype_lb, recvtype_extent, recvtype_true_extent;
    int dtcopy_id, *recv_id = NULL;
    void *tmp_buf = NULL;
    const void *data_buf = NULL;
    int tree_type, vtx_id;
    MPIR_Treealgo_tree_t my_tree, parents_tree;
    int next_child, num_children, *child_subtree_size = NULL;
    int num_dependencies;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPI_Aint *child_data_offset;
    MPIR_CHKLMEM_DECL(3);

    MPIR_FUNC_ENTER;


    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    lrank = (rank - root + size) % size;        /* logical rank when root is non-zero */

    if (rank == root)
        is_inplace = (sendbuf == MPI_IN_PLACE); /* For gather, MPI_IN_PLACE is significant only at root */

    tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* currently only tree_type=MPIR_TREE_TYPE_KNOMIAL_1 is supported for gather */
    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

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
    MPIR_CHKLMEM_MALLOC(child_subtree_size, int *, sizeof(int) * num_children, mpi_errno, "child_subtree_size buffer", MPL_MEM_COLL);   /* to store size of subtree of each child */
    MPIR_CHKLMEM_MALLOC(child_data_offset, MPI_Aint *, sizeof(MPI_Aint) * num_children, mpi_errno, "child_data_offset buffer", MPL_MEM_COLL);   /* to store the offset of the data to be sent to each child  */

    /* calculate size of subtree of each child */

    /* get tree information of the parent */
    if (my_tree.parent != -1) {
        MPIR_Treealgo_tree_create(my_tree.parent, size, tree_type, k, root, &parents_tree);
    } else {    /* initialize an empty children array */
        utarray_new(parents_tree.children, &ut_int_icd, MPL_MEM_COLL);
        parents_tree.num_children = 0;
    }

    MPI_Aint recv_size;
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

    MPIR_Treealgo_tree_free(&parents_tree);

    recv_size *= (lrank == 0) ? recvcount : sendcount;

    MPI_Aint offset;
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

    MPIR_CHKLMEM_MALLOC(recv_id, int *, sizeof(int) * num_children,
                        mpi_errno, "recv_id buffer", MPL_MEM_COLL);
    /* Leaf nodes send to parent */
    if (num_children == 0) {
        mpi_errno =
            MPIR_TSP_sched_isend(tmp_buf, sendcount, sendtype, my_tree.parent, tag, comm, sched, 0,
                                 NULL, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv\n", rank));
    } else {
        num_dependencies = 0;
        if (tmp_buf != recvbuf || (lrank == 0 && !is_inplace)) {        /* copy data into tmp buf unless tmp_buf is recvbuf and inplace
                                                                         * in which case data is already located in the correct position */
            if (lrank == 0 && root != 0 && is_inplace)
                data_buf = (char *) recvbuf + root * recvcount * recvtype_extent;
            else
                data_buf = (void *) sendbuf;

            mpi_errno =
                MPIR_TSP_sched_localcopy(data_buf, sendcount, sendtype,
                                         tmp_buf, recvcount, recvtype, sched, 0, NULL, &dtcopy_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
            num_dependencies++;
        }

        for (i = 0; i < num_children; i++) {
            int child = *(int *) utarray_eltptr(my_tree.children, i);
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) tmp_buf + child_data_offset[i] * recvtype_extent,
                                     child_subtree_size[i] * recvcount, recvtype, child, tag, comm,
                                     sched, num_dependencies, &dtcopy_id, &recv_id[i]);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

        if (my_tree.parent != -1) {
            mpi_errno = MPIR_TSP_sched_isend(tmp_buf, recv_size, recvtype, my_tree.parent,
                                             tag, comm, sched, num_children, recv_id, &vtx_id);
            MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        }

    }

    if (lrank == 0 && root != 0) {      /* root puts data in recvbuf */
        mpi_errno = MPIR_TSP_sched_localcopy(tmp_buf, (size - root) * recvcount, recvtype,
                                             (char *) recvbuf + root * recvcount * recvtype_extent,
                                             (size - root) * recvcount, recvtype, sched,
                                             num_children, recv_id, &dtcopy_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) tmp_buf + (size - root) * recvcount * recvtype_extent,
                                     root * recvcount, recvtype, recvbuf, root * recvcount,
                                     recvtype, sched, 1, &dtcopy_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    MPIR_Treealgo_tree_free(&my_tree);

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    MPIR_FUNC_EXIT;
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
