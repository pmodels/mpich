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

#include "tree_util.h"

#ifndef COLL_NAMESPACE
#error "The tree template must be namespaced with COLL_NAMESPACE"
#endif

#ifndef COLL_USE_KNOMIAL
#error "COLL_USE_KNOMIAL must be defined"
#endif

static inline void COLL_tree_kary_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    ct->rank = rank;
    ct->nranks = nranks;
    ct->numRanges = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    int lrank = (rank + (nranks - root)) % nranks;
    int child;
    COLL_Assert(k >= 1);

    ct->parent = (lrank <= 0) ? -1 : (((lrank - 1) / k) + root) % nranks;

    for (child = 1; child <= k; child++) {
        int val = lrank * k + child;
        if (val >= nranks)
            break;
        val = (val + root) % nranks;
        COLL_tree_add_child(ct, val);
    }
}

static inline void COLL_tree_knomial_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
    ct->rank = rank;
    ct->nranks = nranks;
    ct->numRanges = 0;
    ct->parent = -1;

    if (nranks <= 0)
        return;

    int lrank = (rank + (nranks - root)) % nranks;
    int basek, i, j;
    COLL_Assert(k >= 2);

    /* Parent Calculation */
    if (lrank <= 0)
        ct->parent = -1;
    else {
        basek = COLL_ilog(k, nranks - 1);

        for (i = 0; i < basek; i++) {
            if (COLL_getdigit(k, lrank, i)) {
                ct->parent = (COLL_setdigit(k, lrank, i, 0) + root) % nranks;
                break;
            }
        }
    }

    if (lrank >= nranks)
        return;

    /* Children Calculation */
    basek = COLL_ilog(k, nranks - 1);

    for (j = 0; j < basek; j++) {
        if (COLL_getdigit(k, lrank, j))
            break;

        for (i = 1; i < k; i++) {
            int child = COLL_setdigit(k, lrank, j, i);
            if (child < nranks)
                COLL_tree_add_child(ct, (child + root) % nranks);
        }
    }
}

static inline void COLL_tree_init(int rank, int nranks, int k, int root, COLL_tree_t * ct)
{
#if COLL_USE_KNOMIAL == 1
    COLL_tree_knomial_init(rank, nranks, k, root, ct);
#else
    COLL_tree_kary_init(rank, nranks, k, root, ct);
#endif
}

static inline int COLL_tree_dump(int tree_size, int root, int k)
{
    int i;
    const char *name = "collective tree";

    fprintf(stderr, "digraph \"%s%d\" {\n", name, k);

    for (i = 0; i < tree_size; i++) {
        COLL_tree_t ct;
        COLL_tree_init(i, tree_size, k, root, &ct);
        fprintf(stderr, "%d -> %d\n", ct.rank, ct.parent);
        int j;

        for (j = 0; j < ct.numRanges; j++) {
            int k;

            for (k = ct.children[j].startRank; k <= ct.children[j].endRank; k++) {
                fprintf(stderr, "%d -> %d\n", ct.rank, k);
            }
        }
    }

    fprintf(stderr, "}\n");

    return 0;
}

static inline int
COLL_sched_bcast(void *buffer,
                 int count,
                 COLL_dt_t * datatype,
                 int root, int tag, COLL_comm_t * comm, int k, COLL_sched_t * s, int finalize)
{
    COLL_tree_t myTree;
    int i, j;
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int size = mycomm->tree.nranks;
    int rank = mycomm->tree.rank;
    COLL_tree_t *tree = &myTree;

    COLL_tree_init(rank, size, k, root, tree);

    if (tree->parent == -1) {
        SCHED_FOREACHCHILDDO(TSP_send(buffer, count, &datatype->tsp_dt,
                                      j, tag, &comm->tsp_comm, &s->tsp_sched, 0, NULL));
    }
    else {
        /* Receive from Parent */
        int recv_id = TSP_recv(buffer, count, &datatype->tsp_dt,
                               tree->parent, tag,  &comm->tsp_comm,
                               &s->tsp_sched, 0, NULL);

        /* Send to all children */
        SCHED_FOREACHCHILDDO(TSP_send(buffer, count, &datatype->tsp_dt,
                                      j, tag,  &comm->tsp_comm,
                                      &s->tsp_sched, 1, &recv_id));
    }
    if (finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }
    return 0;
}

static inline int
COLL_sched_reduce(const void *sendbuf,
                  void *recvbuf,
                  int count,
                  COLL_dt_t * datatype,
                  COLL_op_t * op,
                  int root,
                  int tag,
                  COLL_comm_t * comm, int k, int is_commutative, COLL_sched_t * s, int finalize)
{
    COLL_tree_t myTree;
    int i, j, is_contig;
    size_t lb, extent, type_size;
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int size = mycomm->tree.nranks;
    int rank = mycomm->tree.rank;
    COLL_tree_t *tree = &myTree;
    void *free_ptr0 = NULL;
    int is_inplace = TSP_isinplace((void *) sendbuf);

    TSP_dtinfo(&datatype->tsp_dt, &is_contig, &type_size, &extent, &lb);
    if (is_commutative)
        COLL_tree_init(rank, size, k, root, tree);
    else
        /* We have to use knomial trees to get rank order */
        COLL_tree_knomial_init(rank, size, k, root, tree);

    if (tree->parent == -1) {
        k = 0;

        if (!is_inplace)
            TSP_dtcopy(recvbuf, count, &datatype->tsp_dt, sendbuf, count, &datatype->tsp_dt);
        int rr_id;
        if (is_commutative) {
            SCHED_FOREACHCHILD() {
                rr_id = TSP_recv_reduce(recvbuf, count, &datatype->tsp_dt,
                                        &op->tsp_op, j, tag,  &comm->tsp_comm,
                                        TSP_FLAG_REDUCE_L, &s->tsp_sched,
                                        j == tree->children[0].startRank ? 0 : 1, &rr_id);
            }
        }
        else {
            SCHED_FOREACHCHILD() {
                rr_id = TSP_recv_reduce(recvbuf, count, &datatype->tsp_dt,
                                        &op->tsp_op, j, tag,  &comm->tsp_comm,
                                        TSP_FLAG_REDUCE_R, &s->tsp_sched,
                                        j == tree->children[0].startRank ? 0 : 1, &rr_id);
            }
        }
    }
    else {
        void *result_buf;
        free_ptr0 = result_buf = TSP_allocate_mem(extent * count);
        result_buf = (void *) ((char *) result_buf - lb);
        k = 0;
        TSP_dtcopy(result_buf, count, &datatype->tsp_dt, sendbuf, count, &datatype->tsp_dt);

        int rr_id, send_id;
        if (is_commutative) {
            SCHED_FOREACHCHILD()        /*NOTE: We are assuming that each rank range has
                                         * only one rank for the i==0 condition to be used correctly */
                rr_id = TSP_recv_reduce(result_buf, count, &datatype->tsp_dt,
                                        &op->tsp_op, j, tag, &comm->tsp_comm,
                                        TSP_FLAG_REDUCE_L, &s->tsp_sched,
                                        j == tree->children[0].startRank ? 0 : 1, &rr_id);
        }
        else {
            SCHED_FOREACHCHILD() {
                rr_id = TSP_recv_reduce(result_buf, count, &datatype->tsp_dt,
                                        &op->tsp_op, j, tag, &comm->tsp_comm,
                                        TSP_FLAG_REDUCE_R, &s->tsp_sched,
                                        j == tree->children[0].startRank ? 0 : 1, &rr_id);
            }
        }
        send_id = TSP_send_accumulate(result_buf, count, &datatype->tsp_dt,
                                      &op->tsp_op, tree->parent,
                                      tag, &comm->tsp_comm, &s->tsp_sched,
                                      (tree->numRanges == 0) ? 0 : 1, &rr_id);
        TSP_free_mem_nb(free_ptr0, &s->tsp_sched, 1, &send_id);
    }

    if (finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }
    return 0;
}

static inline int
COLL_sched_reduce_full(const void *sendbuf,
                       void *recvbuf,
                       int count,
                       COLL_dt_t * datatype,
                       COLL_op_t * op,
                       int root, int tag, COLL_comm_t * comm, int k, COLL_sched_t * s, int finalize)
{
    int is_commutative, rc;
    TSP_opinfo(&op->tsp_op, &is_commutative);

    if (root == 0 || is_commutative) {
        rc = COLL_sched_reduce(sendbuf, recvbuf, count, datatype,
                               op, root, tag, comm, k, is_commutative, s, finalize);
    }
    else {
        COLL_tree_comm_t *mycomm = &comm->tree_comm;
        int rank = mycomm->tree.rank;
        size_t lb, extent, type_size;
        int is_contig;
        int is_inplace = TSP_isinplace((void *) sendbuf);

        TSP_dtinfo(&datatype->tsp_dt, &is_contig, &type_size, &extent, &lb);
        void *tmp_buf = NULL;
        void *sb = NULL;

        if (rank == 0)
            tmp_buf = TSP_allocate_mem(extent * count);

        if (rank == root && is_inplace)
            sb = recvbuf;
        else
            sb = (void *) sendbuf;
        rc = COLL_sched_reduce(sb, tmp_buf, count, datatype,
                               op, 0, tag, comm, k, is_commutative, s, 0);
        int fence_id = TSP_fence(&s->tsp_sched);
        int send_id;
        if (rank == 0) {
            send_id = TSP_send(tmp_buf, count, &datatype->tsp_dt,
                               root, tag, &comm->tsp_comm, &s->tsp_sched, 1, &fence_id);
            TSP_free_mem_nb(tmp_buf, &s->tsp_sched, 1, &send_id);
        }
        else if (rank == root) {
            TSP_recv(recvbuf, count, &datatype->tsp_dt,
                     0, tag, &comm->tsp_comm, &s->tsp_sched, 1, &fence_id);
        }

        if (finalize) {
            TSP_fence(&s->tsp_sched);
            TSP_sched_commit(&s->tsp_sched);
        }
    }
    return rc;
}

static inline int
COLL_sched_allreduce(const void *sendbuf,
                     void *recvbuf,
                     int count,
                     COLL_dt_t * datatype,
                     COLL_op_t * op,
                     int tag, COLL_comm_t * comm, int k, COLL_sched_t * s, int finalize)
{
    int is_commutative, is_inplace, is_contig;
    size_t lb, extent, type_size;
    void *tmp_buf = NULL;
    void *sbuf = (void *) sendbuf;
    void *rbuf = recvbuf;

    is_inplace = TSP_isinplace((void *) sendbuf);
    TSP_dtinfo(&datatype->tsp_dt, &is_contig, &type_size, &extent, &lb);
    TSP_opinfo(&op->tsp_op, &is_commutative);

    if (is_inplace) {
        tmp_buf = TSP_allocate_mem(extent * count);
        sbuf = recvbuf;
        rbuf = tmp_buf;
    }

    COLL_sched_reduce_full(sbuf, rbuf, count, datatype, op, 0, tag, comm, k, s, 0);
    TSP_wait(&s->tsp_sched);
    COLL_sched_bcast(rbuf, count, datatype, 0, tag, comm, k, s, 0);

    if (is_inplace) {
        int fence_id = TSP_fence(&s->tsp_sched);
        int dtcopy_id = TSP_dtcopy_nb(recvbuf, count, &datatype->tsp_dt,
                                      tmp_buf, count, &datatype->tsp_dt,
                                      &s->tsp_sched, 1, &fence_id);
        TSP_free_mem_nb(tmp_buf, &s->tsp_sched, 1, &dtcopy_id);
    }

    if (finalize) {
        TSP_sched_commit(&s->tsp_sched);
    }
    return 0;
}
