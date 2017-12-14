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

#include "coll_pipeline_util.h"
#include "coll_tree_util.h"

#ifndef MPIR_ALGO_NAMESPACE
#error "The tree template must be namespaced with MPIR_ALGO_NAMESPACE"
#endif

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_tree_init
/* Generate the tree */
MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_tree_init(int rank, int nranks, int tree_type, int k, int root,
                               MPIR_ALGO_tree_t * ct)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_TREE_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_TREE_INIT);

    int mpi_errno = MPI_SUCCESS;

    if (tree_type == 0) /* knomial tree */
        MPIR_ALGO_tree_knomial_init(rank, nranks, k, root, ct);
    else if (tree_type == 1) /* kary tree */
        MPIR_ALGO_tree_kary_init(rank, nranks, k, root, ct);
    else {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"tree_type %d not defined, initializing knomial tree\n", tree_type));
        MPIR_ALGO_tree_knomial_init(rank, nranks, k, root, ct);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_TREE_INIT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_tree_free
/* Free any memory associate with MPIR_ALGO_tree_t */
MPL_STATIC_INLINE_PREFIX void MPIR_ALGO_tree_free (MPIR_ALGO_tree_t *tree) {
    if(tree->max_children)
        MPL_free (tree->children);
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_tree_dump
/* Utility routine to print a tree */
MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_tree_dump(int tree_size, int root, int tree_type, int k)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_TREE_DUMP);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_TREE_DUMP);

    int mpi_errno = MPI_SUCCESS;
    int i;

    char str_out[1000]; int str_len=0;
    for (i = 0; i < tree_size; i++) {
        MPIR_ALGO_tree_t ct;
        int j;

        mpi_errno = MPIR_ALGO_tree_init(i, tree_size, tree_type, k, root, &ct);
        MPIR_Assert(str_len <= 950); /* make sure there is enough space to write buffer */
        str_len += sprintf(str_out+str_len, "%d -> %d, ", ct.rank, ct.parent);

        MPIR_Assert(str_len <= 950);
        str_len += sprintf(str_out+str_len, "{");
        for (j = 0; j < ct.num_children; j++) {
            int tmp = ct.children[j];
            MPIR_Assert(str_len <= 950);
            str_len += sprintf(str_out+str_len, "%d, ", tmp);
        }
        MPIR_Assert(str_len <= 950);
        str_len += sprintf(str_out+str_len, "}\n");

        MPIR_ALGO_tree_free (&ct);
    }
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,str_out));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_TREE_INIT);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_bcast_tree
/* Routine to schedule a tree based broadcast */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_bcast_tree(void *buffer, int count, MPIR_ALGO_dt_t datatype, int root, int tag,
                      MPIR_ALGO_comm_t * comm, int tree_type, int k, MPIR_TSP_sched_t * sched, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE);

    int mpi_errno = MPI_SUCCESS;
    MPIR_ALGO_tree_t my_tree;
    my_tree.children = NULL;
    MPIR_ALGO_tree_comm_t *mycomm = &comm->tree_comm;
    int size = mycomm->tree.nranks;
    int rank = mycomm->tree.rank;
    int recv_id;

    mpi_errno = MPIR_ALGO_tree_init(rank, size, tree_type, k, root, &my_tree);

    /* Receive message from Parent */
    if (my_tree.parent != -1) {
        recv_id = MPIR_TSP_recv(buffer, count, datatype, my_tree.parent, tag,
                           &comm->tsp_comm, sched, 0, NULL);
    }

    int num_children = my_tree.num_children;

    if(num_children){
        /*Multicast data to the children*/
        MPIR_TSP_multicast(buffer, count, datatype, my_tree.children, num_children, tag, &comm->tsp_comm, sched,
                            (my_tree.parent != -1) ? 1 : 0, &recv_id);
    }

    if (finalize) {
        MPIR_TSP_fence(sched);
        MPIR_TSP_sched_commit(sched);
    }

    MPIR_ALGO_tree_free (&my_tree);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_bcast_tree_pipelined
/* Routine to schedule a pipelined tree based broadcast */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_bcast_tree_pipelined(void *buffer, int count, MPIR_ALGO_dt_t datatype, int root, int tag,
                                MPIR_ALGO_comm_t * comm, int tree_type, int k, int segsize,
                                MPIR_TSP_sched_t * sched, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE_PIPELINED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE_PIPELINED);

    int mpi_errno = MPI_SUCCESS;
    int i;

    /* variables to store pipelining information */
    int num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil;
    int offset = 0;

    /* variables for storing datatype information */
    int is_contig;
    size_t lb, extent, type_size;

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Scheduling pipelined broadcast on %d ranks, root=%d\n",
                MPIR_TSP_size(&comm->tsp_comm), root));

    MPIR_TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    /* calculate chunking information for pipelining */
    MPIR_COLL_calculate_chunk_info(segsize, type_size, count, &num_chunks, &num_chunks_floor,
                              &chunk_size_floor, &chunk_size_ceil);
    /* print chunking information */
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,
        "Broadcast pipeline info: segsize=%d count=%d num_chunks=%d num_chunks_floor=%d chunk_size_floor=%d chunk_size_ceil=%d \n",
         segsize, count, num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil));

    /* do pipelined broadcast */
    /* NOTE: Make sure you are handling non-contiguous datatypes correctly with pipelined
     * broadcast, for example, buffer+offset if being calculated correctly */
    for (i = 0; i < num_chunks; i++) {
        int msgsize = (i < num_chunks_floor) ? chunk_size_floor : chunk_size_ceil;
        mpi_errno =
            MPIR_ALGO_sched_bcast_tree((char *) buffer + offset * extent, msgsize, datatype, root, tag,
                                  comm, tree_type, k, sched, 0);
        offset += msgsize;
    }

    /* if this is final part of the schedule, commit it */
    if (finalize)
        MPIR_TSP_sched_commit(sched);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_BCAST_TREE_PIPELINED);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_reduce_tree
/* routing to schedule a tree based broadcast. This routing only handles the
 * cases when (root==0 || operation is commutative) */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_reduce_tree(const void *sendbuf,
                       void *recvbuf,
                       int count,
                       MPIR_ALGO_dt_t datatype,
                       MPIR_ALGO_op_t op,
                       int root,
                       int tag,
                       MPIR_ALGO_comm_t * comm, int tree_type, int k, int is_commutative,
                       MPIR_TSP_sched_t * sched, int finalize, bool per_child_buffer)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE);

    int mpi_errno = MPI_SUCCESS;

    int i, j;
    /* variables for storing datatype information */
    int is_contig;
    size_t lb, extent, type_size;

    MPIR_ALGO_tree_comm_t *mycomm = &comm->tree_comm;
    int size = mycomm->tree.nranks;
    int rank = mycomm->tree.rank;
    int is_inplace, dtcopy_id=-1, *vtcs, *reduce_id, *recv_id, nvtcs;
    void *result_buf;
    void *target_buf = NULL; /* reduced data is accumulated in this buffer */
    void **childbuf;
    bool is_root, is_leaf, is_intermediate;

    MPIR_TSP_comm_t *tsp_comm = &comm->tsp_comm;

    /* structure in which tree is stored */
    MPIR_ALGO_tree_t my_tree;
    my_tree.children = NULL;
    int num_children = 0; /* number of children in the tree */

    if (is_commutative)
        MPIR_ALGO_tree_init(rank, size, tree_type, k, root, &my_tree);
    else
        /* We have to use knomial trees to get rank order */
        MPIR_ALGO_tree_knomial_init(rank, size, k, root, &my_tree);

    /* get dataype information */
    MPIR_TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    is_inplace = MPIR_TSP_isinplace((void *) sendbuf);

    /* calculate the number of children to allocate #children buffers */
    num_children = my_tree.num_children;

    is_root = (my_tree.parent == -1);         /* whether this rank is the root of the tree */
    is_leaf = (num_children == 0);          /* whether this ranks is a leaf vertex in the tree */
    is_intermediate = !is_root && !is_leaf; /* is an intermediate vertex in the tree */

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"num_children: %d, num_ranks: %d, is_root: %d, is_inplace: %d, is_intermediate: %d, lb: %zd\n",
                num_children, size, is_root, is_inplace, is_intermediate, lb));

    /* variables to store task ids */
    vtcs = MPIR_TSP_allocate_mem(sizeof(int) * (num_children + 1));
    reduce_id = MPIR_TSP_allocate_mem(sizeof(int) * num_children);
    recv_id = MPIR_TSP_allocate_mem(sizeof(int) * num_children);

    if (is_root) {
        target_buf = recvbuf;
    } else if (is_intermediate) { /* For intermediate vertex in tree, recv buffer can be invalid,
                                   * therefore, we need a buffer to accumulate reduced data */
        result_buf = MPIR_TSP_allocate_buffer(extent * count, sched);
        result_buf = (void *) ((char *) result_buf - lb);
        target_buf = result_buf;
    }

    if ((is_root && !is_inplace) || (is_intermediate)) {
        /* no need to copy for leaves because no reduction happens there */
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"scheduling data copy\n"));
        dtcopy_id =
            MPIR_TSP_dtcopy_nb(target_buf, count, datatype, sendbuf, count, datatype, sched, 0, NULL);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Allocating buffer for receiving data from children\n"));

    /* allocate buffer space to receive data from children */
    childbuf = (void **) MPIR_TSP_allocate_mem(sizeof(void *) * num_children);

    if (num_children) { /* allocate at least one buffer if you have at least one child */
        childbuf[0] = MPIR_TSP_allocate_buffer(extent * count, sched);
        childbuf[0] = (void *) ((char *) childbuf[0] - lb);
    }
    /* allocate/assign buffer for all the children */
    for (i = 0; i < my_tree.num_children; i++) {
        j = my_tree.children[i];
        if (i == 0)
            continue; /* memory is already allocated for the first child above */
        else {
            if (per_child_buffer) { /* different buffer per child */
                childbuf[i] = MPIR_TSP_allocate_buffer(extent * count, sched);
                childbuf[i] = (void *) ((char *) childbuf[i] - lb);
            } else  /* same buffer for every child */
                childbuf[i] = childbuf[0];
        }
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Start receiving data\n"));
    for (i = 0; i < my_tree.num_children; i++) {
            j = my_tree.children[i];
            /* Setup the dependencies for posting the recv for child's data */
            if (per_child_buffer) { /* no dependency, just post the receive */
                nvtcs = 0;
            } else {
                if (i == 0) /* this is the first recv and therefore no dependency */
                    nvtcs = 0;
                else {
                    /* wait for the previous reduce to complete, before posting the next receive */
                    vtcs[0] = reduce_id[i - 1];
                    nvtcs = 1;
                }
            }
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Schedule receive from child %d\n", j));
            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"Posting receive at address %p\n", childbuf[i]));
            /* post the recv */
            recv_id[i] = MPIR_TSP_recv(childbuf[i], count, datatype,
                                            j, tag, tsp_comm, sched, nvtcs, vtcs);

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"receive scheduled\n"));
            /* Reduction depends on the data copy to complete */
            nvtcs = 0;
            if ((is_root && !is_inplace) || is_intermediate) { /* this is when data copy was done */
                nvtcs = 1;
                vtcs[0] = dtcopy_id;
            }

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"added datacopy dependency (if applicable)\n"));
            /* reduction depends on the corresponding recv to complete */
            vtcs[nvtcs] = recv_id[i];
            nvtcs += 1;

            MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"schedule reduce\n"));
            if (is_commutative) {       /* reduction order does not matter */
                reduce_id[i] = MPIR_TSP_reduce_local(childbuf[i], target_buf, count,
                                                          datatype, op, MPIR_TSP_FLAG_REDUCE_L, sched,
                                                          nvtcs, vtcs);
            } else {
                /* NOTE: Make sure that knomial tree is being constructed differently for reduce for optimal performace.
                 * In bcast, leftmost subtree is the largest while it should be the opposite in case of reduce */

                /* wait for the previous reduce to complete */
                if (i != 0) {
                    vtcs[nvtcs] = reduce_id[i - 1];
                    nvtcs++;
                }
                reduce_id[i] = MPIR_TSP_reduce_local(childbuf[i], target_buf, count,
                                                          datatype, op, MPIR_TSP_FLAG_REDUCE_R, sched,
                                                          nvtcs, vtcs);
            }
    }

    if (!is_root) {
        MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"schedule send to parent %d\n", my_tree.parent));
        if (is_leaf) {
            nvtcs = 0;  /* just send the data to parent */
            target_buf = (void *) sendbuf;
        } else if (is_commutative) { /* wait for all the reductions to complete */
            nvtcs = num_children;
            for (i = 0; i < num_children; i++)
                vtcs[i] = reduce_id[i];
        } else { /* Just wait for the last reduce to complete */
            nvtcs = 1;
            vtcs[0] = reduce_id[num_children - 1];
        }

        /* send date to parent */
        MPIR_TSP_send(target_buf, count, datatype, my_tree.parent, tag, tsp_comm, sched, nvtcs, vtcs);
    }

    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,"completed schedule generation\n"));
    MPIR_TSP_free_mem(childbuf);
    MPIR_TSP_free_mem(vtcs);
    MPIR_TSP_free_mem(recv_id);
    MPIR_TSP_free_mem(reduce_id);
    MPIR_ALGO_tree_free (&my_tree);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_reduce_tree_full
/* routine to schedule a reduce. This routine handles all the scenarios */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_reduce_tree_full(const void *sendbuf,
                            void *recvbuf,
                            int count,
                            MPIR_ALGO_dt_t datatype,
                            MPIR_ALGO_op_t op,
                            int root, int tag, MPIR_ALGO_comm_t * comm, int tree_type, int k,
                            MPIR_TSP_sched_t * s, int finalize, int nbuffers)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL);

    int is_commutative, rc;
    MPIR_TSP_opinfo(op, &is_commutative);

    if (root == 0 || is_commutative) {
        rc = MPIR_ALGO_sched_reduce_tree(sendbuf, recvbuf, count, datatype,
                                    op, root, tag, comm, tree_type, k, is_commutative, s, finalize,
                                    nbuffers);
    } else {    /* when root!=0 && !is_commutative */
        int rank = comm->tree_comm.tree.rank;
        /* datatype information */
        size_t lb, extent, type_size;
        int is_contig, is_inplace, fence_id;
        void *tmp_buf = NULL, *sb = NULL;

        MPIR_TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

        is_inplace = MPIR_TSP_isinplace((void *) sendbuf);

        if (rank == 0)
            tmp_buf = MPIR_TSP_allocate_buffer(extent * count, s);

        if (rank == root && is_inplace)
            sb = recvbuf;
        else
            sb = (void *) sendbuf;

        /* do a reduce to rank 0 */
        rc = MPIR_ALGO_sched_reduce_tree(sb, tmp_buf, count, datatype,
                                    op, 0, tag, comm, tree_type, k, is_commutative, s, 0, nbuffers);

        /* send the reduced data to the root */
        fence_id = MPIR_TSP_fence(s);
        if (rank == 0) {
            MPIR_TSP_send(tmp_buf, count, datatype, root, tag, &comm->tsp_comm, s, 1, &fence_id);
        } else if (rank == root) {
            MPIR_TSP_recv(recvbuf, count, datatype, 0, tag, &comm->tsp_comm, s, 1, &fence_id);
        }

        if (finalize) {
            MPIR_TSP_fence(s);
            MPIR_TSP_sched_commit(s);
        }
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL);

    return rc;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_bcast_tree_full_pipelined
/* Routing to schedule a pipelined tree based reduce */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_reduce_tree_full_pipelined(const void *sendbuf, void *recvbuf, int count,
                                      MPIR_ALGO_dt_t datatype, MPIR_ALGO_op_t op, int root, int tag,
                                      MPIR_ALGO_comm_t * comm, int tree_type, int k, MPIR_TSP_sched_t * sched,
                                      int segsize, int finalize, int nbuffers)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL_PIPELINED);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL_PIPELINED);

    int mpi_errno = MPI_SUCCESS;
    /* variables to store pipelining information */
    int num_chunks, num_chunks_floor, chunk_size_floor, chunk_size_ceil;
    /* variables to store datatype information */
    int is_contig;
    size_t lb, extent, type_size;

    int i, offset = 0;
    int is_inplace = MPIR_TSP_isinplace((void *) sendbuf);

    /* get datatype information */
    MPIR_TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);

    /* get chunking information for pipelining */
    MPIR_COLL_calculate_chunk_info(segsize, type_size, count, &num_chunks, &num_chunks_floor,
                              &chunk_size_floor, &chunk_size_ceil);
    MPL_DBG_MSG_FMT(MPIR_DBG_COLL,VERBOSE,(MPL_DBG_FDEST,
         "Reduce segsize %d type_size %zd count %d num_chunks %d num_chunks_floor %d chunk_size_floor %d chunk_size_ceil %d \n",
         segsize, type_size, count, num_chunks, num_chunks_floor, chunk_size_floor,
         chunk_size_ceil));

    /* do pipelined reduce */
    for (i = 0; i < num_chunks; i++) {
        int msgsize = (i < num_chunks_floor) ? chunk_size_floor : chunk_size_ceil;

        const char *send_buf = (is_inplace) ? sendbuf : (char *) sendbuf + offset * extent;     /* if it is in_place send_buf should remain MPI_INPLACE */
        MPIR_ALGO_sched_reduce_tree_full(send_buf, (char *) recvbuf + offset * extent, msgsize, datatype,
                                    op, root, tag, comm, tree_type, k, sched, 0, nbuffers);

        offset += msgsize;
    }

    if (finalize) {
        MPIR_TSP_fence(sched);
        MPIR_TSP_sched_commit(sched);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_REDUCE_TREE_FULL_PIPELINED);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_allreduce_tree
/* Routine to schedule tree based allreduce */
MPL_STATIC_INLINE_PREFIX int
MPIR_ALGO_sched_allreduce_tree(const void *sendbuf, void *recvbuf, int count, MPIR_ALGO_dt_t datatype,
                          MPIR_ALGO_op_t op, int tag, MPIR_ALGO_comm_t * comm, int tree_type, int k,
                          MPIR_TSP_sched_t * s, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_ALLREDUCE_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_ALLREDUCE_TREE);

    int mpi_errno = MPI_SUCCESS;
    int is_commutative, is_inplace, is_contig;
    size_t lb, extent, type_size;
    void *tmp_buf = NULL;
    void *sbuf = (void *) sendbuf;
    void *rbuf = recvbuf;

    is_inplace = MPIR_TSP_isinplace((void *) sendbuf);
    MPIR_TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
    MPIR_TSP_opinfo(op, &is_commutative);

    if (is_inplace) {
        tmp_buf = MPIR_TSP_allocate_buffer(extent * count, s);
        sbuf = recvbuf;
        rbuf = tmp_buf;
    }
    /* schedule reduce */
    MPIR_ALGO_sched_reduce_tree_full(sbuf, rbuf, count, datatype, op, 0, tag, comm, tree_type, k, s, 0,
                                0);
    /* wait for reduce to complete */
    MPIR_TSP_wait(s);
    /* schedule broadcast */
    MPIR_ALGO_sched_bcast_tree(rbuf, count, datatype, 0, tag, comm, tree_type, k, s, 0);

    if (is_inplace) {
        int fence_id = MPIR_TSP_fence(s);
        MPIR_TSP_dtcopy_nb(recvbuf, count, datatype, tmp_buf, count, datatype, s, 1, &fence_id);
    }

    if (finalize) {
        MPIR_TSP_sched_commit(s);
    }

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_ALLREDUCE_TREE);

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIR_ALGO_sched_barrier_tree
/* routine to schedule a tree based barrier */
MPL_STATIC_INLINE_PREFIX int MPIR_ALGO_sched_barrier_tree(int tag, MPIR_ALGO_comm_t * comm, int tree_type, int k,
                                        MPIR_TSP_sched_t * s, int finalize)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIR_ALGO_SCHED_BARRIER_TREE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIR_ALGO_SCHED_BARRIER_TREE);

    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIR_ALGO_tree_t my_tree;
    my_tree.children = NULL;
    MPIR_TSP_dt_t dt = MPIR_TSP_global.control_dt;

    MPIR_ALGO_tree_init(MPIR_TSP_rank(&comm->tsp_comm), MPIR_TSP_size(&comm->tsp_comm), tree_type, k, 0, &my_tree);

    /* Receive from all children */
    for(i=0; i< my_tree.num_children; i++) {
        MPIR_TSP_recv(NULL, 0, dt, my_tree.children[i], tag, &comm->tsp_comm, s, 0, NULL);
    }

    int fid = MPIR_TSP_fence(s);

    if (my_tree.parent == -1) {
        for(i=0; i< my_tree.num_children; i++) {
            MPIR_TSP_send(NULL, 0, dt, my_tree.children[i], tag, &comm->tsp_comm, s, 1, &fid);
        }
    } else {
        int recv_id;

        /* Send to Parent      */
        MPIR_TSP_send(NULL, 0, dt, my_tree.parent, tag, &comm->tsp_comm, s, 1, &fid);
        /* Receive from Parent */
        recv_id = MPIR_TSP_recv(NULL, 0, dt, my_tree.parent, tag, &comm->tsp_comm, s, 0, NULL);
        /* Send to all children */
        for(i=0; i< my_tree.num_children; i++) {
            MPIR_TSP_send(NULL, 0, dt, my_tree.children[i], tag, &comm->tsp_comm, s, 1, &recv_id);
        }
    }

    if(finalize)
        MPIR_TSP_sched_commit(s);

    MPIR_ALGO_tree_free (&my_tree);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIR_ALGO_SCHED_BARRIER_TREE);

    return mpi_errno;
}
