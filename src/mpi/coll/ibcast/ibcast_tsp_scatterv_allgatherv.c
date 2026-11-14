/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpiimpl.h"
#include "algo_common.h"
#include "ibcast.h"

/* The chunks are in rank order in the buffer, and a subtree of the scatter tree is a range of
 * logical ranks, which wraps around the end of the rank order when the root is not 0. Split the
 * logical range [lbeg, lend) into the one or two contiguous pieces it occupies in the buffer, in
 * logical order. displs has size + 1 entries, with displs[size] being the total size. */
static int get_pieces(int lbeg, int lend, int root, int size, const MPI_Aint * displs,
                      MPI_Aint offs[2], MPI_Aint lens[2])
{
    int first = (lbeg + root) % size;
    int last = first + (lend - lbeg);

    offs[0] = displs[first];
    if (last <= size) {
        lens[0] = displs[last] - displs[first];
        return 1;
    }
    lens[0] = displs[size] - displs[first];
    offs[1] = 0;
    lens[1] = displs[last - size];
    return 2;
}

/* Routine to schedule a scatter followed by recursive exchange based broadcast */
int MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(void *buffer, MPI_Aint count,
                                                    MPI_Datatype datatype, int root,
                                                    MPIR_Comm * comm, int use_ring,
                                                    int scatterv_k, int allgatherv_k,
                                                    MPIR_TSP_sched_t sched)
{
    int mpi_errno = MPI_SUCCESS;
    MPI_Aint type_size;
    MPI_Aint true_lb, true_extent;
    int size, rank, tag;
    int i, j, x, is_contig;
    void *tmp_buf = NULL;
    MPI_Aint *cnts, *displs;
    MPI_Aint nbytes;
    int tree_type, vtx_id, recv_id;
    MPIR_Treealgo_tree_t my_tree, parents_tree;
    int current_child, next_child, lrank, total_count, sink_id;
    int num_children, *child_subtree_size = NULL, *child_subtree_nranks = NULL;
    int num_send_dependencies;
    int lend, npieces, recv_ids[2], nrecv = 0;
    MPI_Aint offs[2], lens[2];
    MPIR_CHKLMEM_DECL();

    /* For correctness, transport based collectives need to get the
     * tag from the same pool as schedule based collectives */
    mpi_errno = MPIR_Sched_next_tag(comm, &tag);
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_FUNC_ENTER;

    MPIR_COMM_RANK_SIZE(comm, rank, size);
    lrank = (rank - root + size) % size;        /* logical rank when root is non-zero */

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST,
                     "Scheduling scatter followed by recursive exchange allgather based broadcast on %d ranks, root=%d\n",
                     size, root));

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
    MPIR_Datatype_is_contig(datatype, &is_contig);

    nbytes = type_size * count;
    MPIR_CHKLMEM_MALLOC(cnts, sizeof(MPI_Aint) * size);
    MPIR_CHKLMEM_MALLOC(displs, sizeof(MPI_Aint) * (size + 1));

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
    displs[size] = nbytes;


    if (is_contig) {
        /* contiguous. no need to pack. */
        tmp_buf = MPIR_get_contig_ptr(buffer, true_lb);
    } else {
        tmp_buf = (void *) MPIR_TSP_sched_malloc(nbytes, sched);

        if (rank == root) {
            mpi_errno =
                MPIR_TSP_sched_localcopy(buffer, count, datatype, tmp_buf, nbytes,
                                         MPIR_BYTE_INTERNAL, sched, 0, NULL, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);
            mpi_errno = MPIR_TSP_sched_fence(sched);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }

    /* knomial scatter for bcast */
    tree_type = MPIR_TREE_TYPE_KNOMIAL_1;       /* currently only tree_type=MPIR_TREE_TYPE_KNOMIAL_1 is supported for scatter */
    mpi_errno = MPIR_Treealgo_tree_create(rank, size, tree_type, scatterv_k, root, &my_tree);
    MPIR_ERR_CHECK(mpi_errno);
    num_children = my_tree.num_children;

    MPIR_CHKLMEM_MALLOC(child_subtree_size, sizeof(int) * num_children);
    MPIR_CHKLMEM_MALLOC(child_subtree_nranks, sizeof(int) * num_children);
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
    lend = lrank + 1;   /* the end of my subtree in logical ranks */
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
        child_subtree_nranks[i] = next_child - current_child;
        for (x = current_child; x < next_child; x++) {
            child_subtree_size[i] += cnts[(x + root) % size];
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                        (MPL_DBG_FDEST,
                         "i:%d rank:%d current_child:%d, next_child:%d, child_subtree_size[i]:%d, recv_size:%ld",
                         i, rank, current_child, next_child, child_subtree_size[i],
                         (long) recv_size));
        recv_size += child_subtree_size[i];
        lend = next_child;
    }
    if (my_tree.parent != -1)
        MPIR_Treealgo_tree_free(&parents_tree);

    /* receive data from the parent */
    if (my_tree.parent != -1) {
        npieces = get_pieces(lrank, lend, root, size, displs, offs, lens);
        for (i = 0; i < npieces; i++) {
#ifdef HAVE_ERROR_CHECKING
            struct MPII_Ibcast_state *ibcast_state =
                MPIR_TSP_sched_malloc(sizeof(struct MPII_Ibcast_state), sched);
            MPIR_ERR_CHKANDJUMP(ibcast_state == NULL, mpi_errno, MPI_ERR_OTHER, "**nomem");
            ibcast_state->n_bytes = lens[i];
            mpi_errno =
                MPIR_TSP_sched_irecv_status((char *) tmp_buf + offs[i], lens[i],
                                            MPIR_BYTE_INTERNAL, my_tree.parent, tag, comm,
                                            &ibcast_state->status, sched, 0, NULL, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);
            MPIR_TSP_sched_cb(&MPII_Ibcast_sched_test_length, ibcast_state, sched, 1, &recv_id,
                              &vtx_id);
#else
            mpi_errno =
                MPIR_TSP_sched_irecv((char *) tmp_buf + offs[i], lens[i], MPIR_BYTE_INTERNAL,
                                     my_tree.parent, tag, comm, sched, 0, NULL, &recv_id);
            MPIR_ERR_CHECK(mpi_errno);
#endif
            recv_ids[nrecv++] = recv_id;
        }
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST, "rank:%d posts recv", rank));
    }

    /* send data to children */
    for (i = 0; i < num_children; i++) {
        int child = *(int *) utarray_eltptr(my_tree.children, i);
        int lchild = (child - root + size) % size;
        int lchild_end = lchild + child_subtree_nranks[i];
        if (num_children > 0 && lrank != 0)
            num_send_dependencies = nrecv;
        else
            num_send_dependencies = 0;

        /* a subtree that wraps around the rank order goes in two messages, which the
         * child receives in the same order */
        npieces = get_pieces(lchild, lchild_end, root, size, displs, offs, lens);
        for (j = 0; j < npieces; j++) {
            mpi_errno = MPIR_TSP_sched_isend((char *) tmp_buf + offs[j], lens[j],
                                             MPIR_BYTE_INTERNAL, child, tag, comm, sched,
                                             num_send_dependencies, recv_ids, &vtx_id);
            MPIR_ERR_CHECK(mpi_errno);
        }
    }


    MPIR_Treealgo_tree_free(&my_tree);
    mpi_errno = MPIR_TSP_sched_fence(sched);    /* wait for scatter to complete */
    MPIR_ERR_CHECK(mpi_errno);

    if (use_ring)
        /* Schedule Allgatherv ring */
        mpi_errno =
            MPIR_TSP_Iallgatherv_sched_intra_ring(MPI_IN_PLACE, cnts[rank], MPIR_BYTE_INTERNAL,
                                                  tmp_buf, cnts, displs, MPIR_BYTE_INTERNAL, comm,
                                                  sched);
    else
        /* Schedule Allgatherv recexch */
        mpi_errno =
            MPIR_TSP_Iallgatherv_sched_intra_recexch_doubling(MPI_IN_PLACE, cnts[rank],
                                                              MPIR_BYTE_INTERNAL, tmp_buf, cnts,
                                                              displs, MPIR_BYTE_INTERNAL, comm,
                                                              allgatherv_k, sched);
    MPIR_ERR_CHECK(mpi_errno);

    if (!is_contig) {
        if (rank != root) {
            mpi_errno = MPIR_TSP_sched_sink(sched, &sink_id);   /* wait for allgather to complete */
            MPIR_ERR_CHECK(mpi_errno);

            mpi_errno =
                MPIR_TSP_sched_localcopy(tmp_buf, nbytes, MPIR_BYTE_INTERNAL, buffer, count,
                                         datatype, sched, 1, &sink_id, &vtx_id);
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

int MPIR_TSP_Ibcast_sched_intra_scatterv_ring_allgatherv(void *buffer, MPI_Aint count,
                                                         MPI_Datatype datatype, int root,
                                                         MPIR_Comm * comm, int scatterv_k,
                                                         MPIR_TSP_sched_t sched)
{
    return MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root, comm,
                                                           1, scatterv_k, 0, sched);
}

int MPIR_TSP_Ibcast_sched_intra_scatterv_recexch_allgatherv(void *buffer, MPI_Aint count,
                                                            MPI_Datatype datatype, int root,
                                                            MPIR_Comm * comm, int scatterv_k,
                                                            int allgatherv_k,
                                                            MPIR_TSP_sched_t sched)
{
    return MPIR_TSP_Ibcast_sched_intra_scatterv_allgatherv(buffer, count, datatype, root, comm,
                                                           0, scatterv_k, allgatherv_k, sched);
}
