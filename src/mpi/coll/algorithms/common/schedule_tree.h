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
COLL_sched_bcast_tree(void *buffer,
                 int count,
                 COLL_dt_t datatype,
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
        SCHED_FOREACHCHILDDO(TSP_send(buffer, count, datatype,
                                      j, tag, &comm->tsp_comm, &s->tsp_sched, 0, NULL));
    }
    else {
        /* Receive from Parent */
        int recv_id = TSP_recv(buffer, count, datatype,
                               tree->parent, tag,  &comm->tsp_comm,
                               &s->tsp_sched, 0, NULL);

        /* Send to all children */
        SCHED_FOREACHCHILDDO(TSP_send(buffer, count, datatype,
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
COLL_sched_bcast_tree_pipelined(void *buffer, int count, COLL_dt_t datatype, int root, int tag,
                                 COLL_comm_t *comm, int k, int segsize, COLL_sched_t *s, int finalize)
{
    int segment_size = (segsize==-1)?count:segsize;
    /*NOTE: Change this so that the segsize is in bytes and then calculate its closest number of elements of type datatype*/
    int num_chunks = (count+segment_size-1)/segment_size; /*ceil of count/segment_size*/
    /*The message is divided into num_chunks*/
    int chunk_size_floor = count/num_chunks; /*smaller of the chunk sizes obtained by integer division*/
    int chunk_size_ceil; /*larger of the chunk size*/
    if(count%num_chunks == 0)
        chunk_size_ceil = chunk_size_floor;/*all chunk sizes are equal*/
    else
        chunk_size_ceil = chunk_size_floor+1;
    int num_chunks_floor; /*number of chunks of size chunk_size_floor*/
    num_chunks_floor = num_chunks*chunk_size_ceil - count;

    int offset=0;

    int is_contig;
    size_t lb, extent, type_size;
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
    /*NOTE: Make sure you are handling non-contiguous datatypes correctly 
    * with pipelined broadcast, for example, buffer+offset if being calculated
    * correctly */
    int i;
    for(i=0; i<num_chunks; i++){
        //(*tag)++;
        int msgsize = (i < num_chunks_floor) ? chunk_size_floor : chunk_size_ceil;
        COLL_sched_bcast_tree((char*)buffer + offset*extent, msgsize, datatype, root, tag, comm, k, s, 0);
        offset += msgsize;
    }
    if (finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }
    return 0;
}

static inline int
COLL_sched_reduce_tree(const void *sendbuf,
                  void *recvbuf,
                  int count,
                  COLL_dt_t datatype,
                  COLL_op_t op,
                  int root,
                  int tag,
                  COLL_comm_t * comm, int k, int is_commutative, COLL_sched_t * s, int finalize, int nbuffers)
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
    int  children   = 0;

    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
    if (is_commutative)
        COLL_tree_init(rank, size, k, root, tree);
    else
        /* We have to use knomial trees to get rank order */
        COLL_tree_knomial_init(rank, size, k, root, tree);

    /*calculate the number of children to allocate #children buffers */
    SCHED_FOREACHCHILDDO(children++);

    void *result_buf;
    int rr_id[3];
    int buf=0;
    if (!is_inplace && tree->parent == -1){
        rr_id[1] = TSP_dtcopy_nb(recvbuf, count, datatype, sendbuf, count, datatype,&s->tsp_sched,0,NULL);
    }
    else if(tree->parent != -1){
        /* How about we just use recvbuf instead of result_buf for non root? Code could be simplied more */
        result_buf = TSP_allocate_buffer(extent * count,&s->tsp_sched);
        result_buf = (void *) ((char *) result_buf - lb);
        rr_id[1] = TSP_dtcopy_nb(result_buf, count, datatype, sendbuf, count, datatype,&s->tsp_sched,0,NULL);
    }
    /*create #children buffers */
    void *result_nbuf[children];
    if(nbuffers!=1){ /* If using only one recv buffer, allocate memory here */
        result_nbuf[0] = TSP_allocate_buffer(extent*count,&s->tsp_sched);
        result_nbuf[0] = (void *)((char *)result_nbuf[0]-lb);
    }
    for(i=0; i<tree->numRanges; i++){
       for(j=tree->children[i].startRank; j<=tree->children[i].endRank; j++){
        if(nbuffers==1){
            /* Allocate nth buffer to receive data */
            result_nbuf[buf] = TSP_allocate_buffer(extent*count,&s->tsp_sched);
            result_nbuf[buf] = (void *)((char *)result_nbuf[buf]-lb);
            rr_id[0] = TSP_recv(result_nbuf[buf],count,datatype,
                                j,tag,&comm->tsp_comm,
                                &s->tsp_sched,0,NULL);
        }
        else{
            rr_id[0] = TSP_recv(result_nbuf[buf],count,datatype,
                                j,tag,&comm->tsp_comm,
                                &s->tsp_sched,j==tree->children[0].startRank?0:1,&rr_id[2]);
        }
        if (is_commutative) {
             /* Either branch or one additional data copy */
             if(tree->parent == -1)
                 rr_id[2] = TSP_reduce_local(result_nbuf[buf],recvbuf,count,
                                             datatype, op, &s->tsp_sched,is_inplace?1:2,rr_id);
             else
                 rr_id[2] = TSP_reduce_local(result_nbuf[buf],result_buf,count,
                                              datatype,op, &s->tsp_sched,2,rr_id);
        }
        else{
             /* non commutative case. Order of reduction matters. Knomia tree has to constructed differently for reduce for optimal performace. 
 *              If it is non commutative the tree is Knomial. Note: Works only when root is zero */
             if(tree->parent == -1){
                 rr_id[2] = TSP_reduce_local(recvbuf,result_nbuf[buf],count,
                                             datatype,op, &s->tsp_sched,is_inplace?1:2,rr_id);
                TSP_dtcopy_nb(recvbuf, count, datatype, result_nbuf[buf], count, datatype,&s->tsp_sched,1,&rr_id[2]);
             }
             else{
                 rr_id[2] = TSP_reduce_local(result_buf,result_nbuf[buf],count,
                                             datatype, op, &s->tsp_sched,2,rr_id);
                TSP_dtcopy_nb(result_buf, count, datatype, result_nbuf[buf], count, datatype,&s->tsp_sched,1,&rr_id[2]);
             }
        }
        if(nbuffers==1)
             buf++;
    }
   }
    if(tree->parent!=-1){
        /* send date to parent if you are a non root node */
        TSP_send(result_buf, count, datatype,
                                    tree->parent,
                                    tag, &comm->tsp_comm, &s->tsp_sched,
                                    1, (tree->numRanges == 0) ? &rr_id[1]:&rr_id[2]);
    }

    return 0;
}

static inline int
COLL_sched_reduce_tree_full(const void *sendbuf,
                       void *recvbuf,
                       int count,
                       COLL_dt_t datatype,
                       COLL_op_t op,
                       int root, int tag, COLL_comm_t * comm, int k, COLL_sched_t * s, int finalize, int nbuffers)
{
    int is_commutative, rc;
    TSP_opinfo(op, &is_commutative);

    if (root == 0 || is_commutative) {
        rc = COLL_sched_reduce_tree(sendbuf, recvbuf, count, datatype,
                               op, root, tag, comm, k, is_commutative, s, finalize, nbuffers);
    }
    else {
        COLL_tree_comm_t *mycomm = &comm->tree_comm;
        int rank = mycomm->tree.rank;
        size_t lb, extent, type_size;
        int is_contig;
        int is_inplace = TSP_isinplace((void *) sendbuf);

        TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
        void *tmp_buf = NULL;
        void *sb = NULL;

        if (rank == 0)
            tmp_buf = TSP_allocate_mem(extent * count);

        if (rank == root && is_inplace)
            sb = recvbuf;
        else
            sb = (void *) sendbuf;
        rc = COLL_sched_reduce_tree(sb, tmp_buf, count, datatype,
                               op, 0, tag, comm, k, is_commutative, s, 0, nbuffers);
        int fence_id = TSP_fence(&s->tsp_sched);
        int send_id;
        if (rank == 0) {
            send_id = TSP_send(tmp_buf, count, datatype,
                               root, tag, &comm->tsp_comm, &s->tsp_sched, 1, &fence_id);
            TSP_free_mem_nb(tmp_buf, &s->tsp_sched, 1, &send_id);
        }
        else if (rank == root) {
            TSP_recv(recvbuf, count, datatype,
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
COLL_sched_allreduce_tree(const void *sendbuf,
                     void *recvbuf,
                     int count,
                     COLL_dt_t datatype,
                     COLL_op_t op,
                     int tag, COLL_comm_t * comm, int k, COLL_sched_t * s, int finalize)
{
    int is_commutative, is_inplace, is_contig;
    size_t lb, extent, type_size;
    void *tmp_buf = NULL;
    void *sbuf = (void *) sendbuf;
    void *rbuf = recvbuf;

    is_inplace = TSP_isinplace((void *) sendbuf);
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
    TSP_opinfo(op, &is_commutative);

    if (is_inplace) {
        tmp_buf = TSP_allocate_mem(extent * count);
        sbuf = recvbuf;
        rbuf = tmp_buf;
    }
    COLL_sched_reduce_tree_full(sbuf, rbuf, count, datatype, op, 0, tag, comm, k, s, 0, 0);
    TSP_wait(&s->tsp_sched);
    COLL_sched_bcast_tree(rbuf, count, datatype, 0, tag, comm, k, s, 0);

    if (is_inplace) {
        int fence_id = TSP_fence(&s->tsp_sched);
        int dtcopy_id = TSP_dtcopy_nb(recvbuf, count, datatype,
                                      tmp_buf, count, datatype,
                                      &s->tsp_sched, 1, &fence_id);
        TSP_free_mem_nb(tmp_buf, &s->tsp_sched, 1, &dtcopy_id);
    }

    if (finalize) {
        TSP_sched_commit(&s->tsp_sched);
    }
    return 0;
}


static inline int
COLL_sched_barrier_tree(int                 tag,
                   COLL_comm_t        *comm,
                   int                 k,
                   COLL_sched_t *s)
{
    int i, j;
    COLL_tree_comm_t *mycomm    = &comm->tree_comm;
    COLL_tree_t      *tree      = &mycomm->tree;
    COLL_tree_t       myTree;
    TSP_dt_t          dt        = TSP_global.control_dt;

    if(k > 1 && 
       k != COLL_TREE_RADIX_DEFAULT) {
        tree = &myTree;
        COLL_tree_init(TSP_rank(&comm->tsp_comm),
                       TSP_size(&comm->tsp_comm),
                       k,
                       0,
                       tree);
    }
    /* Receive from all children */
    SCHED_FOREACHCHILDDO(TSP_recv(NULL,0,dt,j,tag,&comm->tsp_comm,
			      &s->tsp_sched,0,NULL));

    int fid = TSP_fence(&s->tsp_sched);
    
    if(tree->parent == -1) {
        SCHED_FOREACHCHILDDO(TSP_send(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched,1,&fid));
    } else {
        /* Send to Parent      */
        TSP_send(NULL,0,dt,tree->parent,tag,&comm->tsp_comm,&s->tsp_sched,1,&fid);
        /* Receive from Parent */
        int recv_id = TSP_recv(NULL,0,dt,tree->parent,tag,&comm->tsp_comm,&s->tsp_sched,0,NULL);
        /* Send to all children */
        SCHED_FOREACHCHILDDO(TSP_send(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched,1,&recv_id));
    }
    return 0;
}
