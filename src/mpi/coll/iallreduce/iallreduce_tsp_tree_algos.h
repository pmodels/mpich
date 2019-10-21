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

/* Header protection (i.e., IALLREDUCE_TSP_ALGOS_H_INCLUDED) is
 * intentionally omitted since this header might get included multiple
 * times within the same .c file. */

#include "algo_common.h"
#include "treealgo.h"
#include "tsp_namespace_def.h"

/* Routine to schedule a pipelined tree based allreduce */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallreduce_sched_intra_tree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallreduce_sched_intra_tree(const void *sendbuf, void *recvbuf, int count,
                                         MPI_Datatype datatype, MPI_Op op,
                                         MPIR_Comm * comm, int tree_type, int k, int maxbytes,
                                         MPIR_TSP_sched_t * sched)
{
    int mpi_errno = MPI_SUCCESS;
    int i, j, t;
    int num_chunks, chunk_size_floor, chunk_size_ceil;
    int offset = 0;
    size_t extent, type_size;
    MPI_Aint type_lb, true_extent;
    int is_commutative;
    int size;
    int rank;
    int num_children = 0;
    MPII_Treealgo_tree_t my_tree;
    void **child_buffer;        /* Buffer array in which data from children is received */
    void *reduce_buffer;        /* Buffer in which allreduced data is present */
    int *vtcs = NULL, *recv_id = NULL, *reduce_id = NULL;       /* Arrays to store graph vertex ids */
    int sink_id;
    int nvtcs;
    int buffer_per_child = MPIR_CVAR_IALLREDUCE_TREE_BUFFER_PER_CHILD;
    int tag;
    int root = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_TREE);

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                    (MPL_DBG_FDEST, "Scheduling pipelined allreduce on %d ranks, root=%d ",
                     MPIR_Comm_size(comm), root));

    size = MPIR_Comm_size(comm);
    rank = MPIR_Comm_rank(comm);

    MPIR_Datatype_get_size_macro(datatype, type_size);
    MPIR_Datatype_get_extent_macro(datatype, extent);
    MPIR_Type_get_true_extent_impl(datatype, &type_lb, &true_extent);
    extent = MPL_MAX(extent, true_extent);
    is_commutative = MPIR_Op_is_commutative(op);


    /* calculate chunking information for pipelining */
    MPII_Algo_calculate_pipeline_chunk_info(maxbytes, type_size, count, &num_chunks,
                                            &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE, (MPL_DBG_FDEST,
                                             "Reduce pipeline info: maxbytes=%d count=%d num_chunks=%d chunk_size_floor=%d chunk_size_ceil=%d ",
                                             maxbytes, count, num_chunks,
                                             chunk_size_floor, chunk_size_ceil));

    if (!is_commutative) {
        tree_type = MPIR_TREE_TYPE_KNOMIAL_1;   /* Force tree_type to be knomial because kary trees cannot
                                                 * handle non-commutative operations correctly */
    }

    /* initialize the tree */
    my_tree.children = NULL;
    mpi_errno = MPII_Treealgo_tree_create(rank, size, tree_type, k, root, &my_tree);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    num_children = my_tree.num_children;

    /* Allocate buffers to receive data from children. Any memory required for execution
     * of the schedule, for example child_buffer, reduce_buffer below, is allocated using
     * MPIR_TSP_sched_malloc function. This function stores the allocated memory address
     * in the schedule and then frees it once the schedule completes execution. The
     * programmer need not free this memory. This is unlike the MPL_malloc function that
     * will be used for any memory required to generate the schedule and then freed by
     * the programmer once that memory is no longer required */
    if (num_children > 0) {
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
    } else {
        /* silence warnings */
        child_buffer = NULL;
    }

    /* Set reduce_buffer based on location in the tree */
    /* FIXME: This can also be pipelined along with rest of the reduction */
    reduce_buffer = recvbuf;
    if (sendbuf != MPI_IN_PLACE)
        MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);

    /* initialize arrays to store graph vertex indices */
    vtcs = MPL_malloc(sizeof(int) * (num_children + 1), MPL_MEM_COLL);
    reduce_id = MPL_malloc(sizeof(int) * num_children, MPL_MEM_COLL);
    recv_id = MPL_malloc(sizeof(int) * num_children, MPL_MEM_COLL);

    /* do pipelined allreduce */
    /* NOTE: Make sure you are handling non-contiguous datatypes
     * correctly with pipelined allreduce, for example, buffer+offset
     * if being calculated correctly */
    for (j = 0; j < num_chunks; j++) {
        int msgsize = (j == 0) ? chunk_size_floor : chunk_size_ceil;
        void *reduce_address = (char *) reduce_buffer + offset * extent;

        /* For correctness, transport based collectives need to get the
         * tag from the same pool as schedule based collectives */
        mpi_errno = MPIR_Sched_next_tag(comm, &tag);
        if (mpi_errno)
            MPIR_ERR_POP(mpi_errno);

        for (i = 0; i < num_children; i++) {
            void *recv_address = (char *) child_buffer[i] + offset * extent;
            int child = *(int *) utarray_eltptr(my_tree.children, i);

            /* Setup the dependencies for posting receive for child's data */
            if (buffer_per_child) {     /* no dependency, just post the receive */
                nvtcs = 0;
            } else {
                if (i == 0) {   /* this is the first receive and therefore no dependency */
                    nvtcs = 0;
                } else {        /* wait for the previous allreduce to complete, before posting the next receive */
                    vtcs[0] = reduce_id[i - 1];
                    nvtcs = 1;
                }
            }

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Schedule receive from child %d ", child));
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL, VERBOSE,
                            (MPL_DBG_FDEST, "Posting receive at address %p ", recv_address));

            recv_id[i] = MPIR_TSP_sched_irecv(recv_address, msgsize, datatype, child, tag, comm,
                                              sched, nvtcs, vtcs);

            /* Setup dependencies for reduction. Reduction depends on the corresponding recv to complete */
            vtcs[0] = recv_id[i];
            nvtcs = 1;

            if (is_commutative) {       /* reduction order does not matter */
                reduce_id[i] = MPIR_TSP_sched_reduce_local(recv_address, reduce_address, msgsize,
                                                           datatype, op, sched, nvtcs, vtcs);
            } else {    /* wait for the previous allreduce to complete */

                /* NOTE: Make sure that knomial tree is being constructed differently for allreduce for optimal performace.
                 * In bcast, leftmost subtree is the largest while it should be the opposite in case of allreduce */

                if (i > 0) {
                    vtcs[nvtcs] = reduce_id[i - 1];
                    nvtcs++;
                }
                reduce_id[i] =
                    MPIR_TSP_sched_reduce_local(reduce_address, recv_address, msgsize, datatype, op,
                                                sched, nvtcs, vtcs);
                reduce_id[i] =
                    MPIR_TSP_sched_localcopy(recv_address, msgsize, datatype, reduce_address,
                                             msgsize, datatype, sched, 1, &reduce_id[i]);
            }
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
        if (rank != root)
            MPIR_TSP_sched_isend(reduce_address, msgsize, datatype, my_tree.parent, tag, comm,
                                 sched, nvtcs, vtcs);

        /* Broadcast start here */
        sink_id = MPIR_TSP_sched_sink(sched);

        /* Receive message from parent */
        int bcast_recv_id = sink_id;
        if (my_tree.parent != -1) {
            bcast_recv_id =
                MPIR_TSP_sched_irecv(reduce_address, msgsize, datatype,
                                     my_tree.parent, tag, comm, sched, 1, &sink_id);
        }

        if (num_children) {
            /* Multicast data to the children */
            nvtcs = 1;
            vtcs[0] = bcast_recv_id;
            MPIR_TSP_sched_imcast(reduce_address, msgsize, datatype,
                                  my_tree.children, num_children, tag, comm, sched, nvtcs, vtcs);
        }

        offset += msgsize;
    }

    MPII_Treealgo_tree_free(&my_tree);

  fn_exit:
    MPL_free(vtcs);
    MPL_free(reduce_id);
    MPL_free(recv_id);
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_SCHED_INTRA_TREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


/* Non-blocking tree based allreduce */
#undef FUNCNAME
#define FUNCNAME MPIR_TSP_Iallreduce_intra_tree
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_TSP_Iallreduce_intra_tree(const void *sendbuf, void *recvbuf, int count,
                                   MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm,
                                   MPIR_Request ** req, int tree_type, int k, int maxbytes)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_TSP_sched_t *sched;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_TREE);

    *req = NULL;

    /* generate the schedule */
    sched = MPL_malloc(sizeof(MPIR_TSP_sched_t), MPL_MEM_COLL);
    MPIR_Assert(sched != NULL);
    MPIR_TSP_sched_create(sched);

    /* schedule pipelined tree algo */
    mpi_errno =
        MPIR_TSP_Iallreduce_sched_intra_tree(sendbuf, recvbuf, count, datatype, op, comm,
                                             tree_type, k, maxbytes, sched);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    /* start and register the schedule */
    mpi_errno = MPIR_TSP_sched_start(sched, comm, req);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_TSP_IALLREDUCE_INTRA_TREE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
