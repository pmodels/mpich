/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"

/* Routine to schedule a tree based scatter */
int MPIR_TSP_Iscatter_sched_intra_tree(const void *sendbuf, MPI_Aint sendcount,
                                       MPI_Datatype sendtype, void *recvbuf, MPI_Aint recvcount,
                                       MPI_Datatype recvtype, int root, MPIR_Comm * comm,
                                       int k, MPIR_TSP_sched_t sched)
{
    MPIR_FUNC_ENTER;

    int mpi_errno = MPI_SUCCESS;
    int mpi_errno_ret ATTRIBUTE((unused)) = MPI_SUCCESS;
    int size, rank;
    int i, j, is_inplace = false;
    int lrank;
    MPI_Aint sendtype_lb, sendtype_extent, sendtype_true_extent;
    MPI_Aint recvtype_lb, recvtype_extent, recvtype_true_extent;
    int dtcopy_id[2];
    void *tmp_buf = NULL;
    int recv_id, vtx_id;
    int tree_type;
    MPIR_Treealgo_tree_t my_tree, parents_tree;
    int next_child;
    int num_children, *child_subtree_size = NULL;
    MPI_Aint *child_data_offset = NULL;
    MPI_Aint offset, recv_size;
    int tag;
    int num_send_dependencies;
    MPIR_Errflag_t errflag ATTRIBUTE((unused)) = MPIR_ERR_NONE;
    MPIR_CHKLMEM_DECL(2);

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);
    lrank = (rank - root + size) % size;        /* logical rank when root is non-zero */

    if (rank == root)
        is_inplace = (recvbuf == MPI_IN_PLACE); /* For scatter, MPI_IN_PLACE is significant only at root */

    tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* currently only tree_type=MPIR_TREE_TYPE_KNOMIAL_1 is supported for scatter */
    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    if (rank == root && is_inplace) {
        recvtype = sendtype;
        recvcount = sendcount;
    } else if (rank != root) {  /* all non root ranks will forward data in recvtype format */
        sendtype = recvtype;
        sendcount = recvcount;
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
                         "i:%d rank:%d current_child:%d, next_child:%d, child_subtree_size[i]:%d, recv_size:%d",
                         i, rank, current_child, next_child, child_subtree_size[i], recv_size));
        recv_size += child_subtree_size[i];
    }

    MPIR_Treealgo_tree_free(&parents_tree);

    recv_size *= (lrank == 0) ? sendcount : recvcount;
    offset = (lrank == 0) ? sendcount : recvcount;
    num_send_dependencies = 0;  /* number of tasks to complete before sends to children can be done */

    for (i = 0; i < num_children; i++) {
        child_data_offset[i] = offset;
        offset += (child_subtree_size[i] * ((lrank == 0) ? sendcount : recvcount));
    }

    if (root != 0 && lrank == 0) {      /* reorganize sendbuf in tmp_buf */
        tmp_buf = MPIR_TSP_sched_malloc(recv_size * sendtype_extent, sched);
        mpi_errno =
            MPIR_TSP_sched_localcopy((char *) sendbuf + root * sendcount * sendtype_extent,
                                     (size - root) * sendcount, sendtype, tmp_buf,
                                     (size - root) * sendcount, sendtype, sched, 0, NULL,
                                     &dtcopy_id[0]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        mpi_errno =
            MPIR_TSP_sched_localcopy(sendbuf, root * sendcount, sendtype,
                                     (char *) tmp_buf + (size - root) * sendcount * sendtype_extent,
                                     root * sendcount, sendtype, sched, 0, NULL, &dtcopy_id[1]);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);

        num_send_dependencies = 2;
    } else if (root == 0 && lrank == 0) {
        tmp_buf = (void *) sendbuf;
        num_send_dependencies = 0;
    } else if (num_children > 0 && lrank != 0) {        /* intermediate ranks in the tree */
        tmp_buf = MPIR_TSP_sched_malloc(recv_size * recvtype_extent, sched);
        num_send_dependencies = 1;      /* wait for data from the parent */
    } else {    /* leaf ranks in the tree */
        tmp_buf = recvbuf;
        num_send_dependencies = 0;      /* these ranks do not have to do any send */
    }

    /* receive data from the parent */
    if (my_tree.parent != -1) {
        mpi_errno = MPIR_TSP_sched_irecv(tmp_buf, recv_size, recvtype, my_tree.parent,
                                         tag, comm, sched, 0, NULL, &recv_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv", rank));
    }

    /* send data to children */
    for (i = 0; i < num_children; i++) {
        int child = *(int *) utarray_eltptr(my_tree.children, i);
        mpi_errno = MPIR_TSP_sched_isend((char *) tmp_buf + child_data_offset[i] * sendtype_extent,
                                         child_subtree_size[i] * sendcount, sendtype,
                                         child, tag, comm, sched, num_send_dependencies,
                                         (lrank == 0) ? dtcopy_id : &recv_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    if (num_children > 0 && lrank != 0) {       /* copy data from tmp_buf to recvbuf */
        mpi_errno = MPIR_TSP_sched_localcopy(tmp_buf, recvcount, recvtype, recvbuf,
                                             recvcount, recvtype, sched, 1, &recv_id, &vtx_id);
        MPIR_ERR_COLL_CHECKANDCONT(mpi_errno, errflag);
    }

    if (lrank == 0 && !is_inplace) {    /* root puts data in recvbuf */
        mpi_errno = MPIR_TSP_sched_localcopy(tmp_buf, sendcount, sendtype, recvbuf,
                                             recvcount, recvtype, sched, 0, NULL, &vtx_id);
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
