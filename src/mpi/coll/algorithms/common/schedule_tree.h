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
                  COLL_comm_t * comm, int k, int is_commutative, COLL_sched_t * s, int finalize, bool per_child_buffer)
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
    int  num_children   = 0;
    TSP_sched_t *sched = &s->tsp_sched;
    TSP_comm_t *tsp_comm = &comm->tsp_comm;

    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb);
    if (is_commutative)
        COLL_tree_init(rank, size, k, root, tree);
    else
        /* We have to use knomial trees to get rank order */
        COLL_tree_knomial_init(rank, size, k, root, tree);
    /*calculate the number of children to allocate #children buffers */
    SCHED_FOREACHCHILDDO(num_children++);

    void *result_buf;
    bool is_root = tree->parent==-1;
    bool is_leaf = (num_children==0);
    bool is_intermediate = !is_root && !is_leaf; /*is an intermediate node in the tree*/

    int dtcopy_id;
    int nvtcs;
    int *vtcs = TSP_allocate_mem(sizeof(int)*num_children);
    int *reduce_id = TSP_allocate_mem(sizeof(int)*num_children);
    int *recv_id = TSP_allocate_mem(sizeof(int)*num_children);
    int child_count=0;

    void *target_buf; /*data is accumulated in this buffer*/
    if (is_root){
        target_buf = recvbuf;
    }
    else if(is_intermediate){/*For intermediate vertex in tree, recv buffer can be invalid, 
                               therefore, we need a buffer to accumulate reduced data */
        result_buf = TSP_allocate_buffer(extent * count, sched);
        result_buf = (void *) ((char *) result_buf - lb);
        target_buf = result_buf;
    }
    if(0) fprintf(stderr, "is_root: %d, is_inplace: %d, is_intermediate: %d, lb: %d\n", is_root, is_inplace, is_intermediate, lb);
    if((is_root && !is_inplace) || (is_intermediate)){
        if(0)fprintf(stderr, "scheduling data copy\n");
        dtcopy_id = TSP_dtcopy_nb(target_buf, count, datatype, sendbuf, count, datatype,sched,0,NULL);
    }
    
    if(0) fprintf(stderr, "Allocating buffer for receiving data\n");
    /*allocate buffer space to receive data from children*/
    void **childbuf;
    childbuf = (void**)TSP_allocate_mem(sizeof(void*)*num_children);
    if(num_children){
        childbuf[0] = TSP_allocate_buffer(extent*count, sched);
        childbuf[0] = (void*)((char*)childbuf[0]-lb);
    }
    for(i=0; i<tree->numRanges; i++){
        for(j=tree->children[i].startRank; j<=tree->children[i].endRank; j++, child_count++){
            if(child_count==0)
                continue;/*memory is already allocated for the first child above*/
            else{
                if(per_child_buffer){
                    childbuf[child_count] = TSP_allocate_buffer(extent*count, sched);
                    childbuf[child_count] = (void*)((char*)childbuf[child_count]-lb);
                }
                else
                    childbuf[child_count] = childbuf[0];
            }
        }
    }
    
    if(0) fprintf(stderr, "Start receiving data\n");
    child_count=0;
    for(i=0; i<tree->numRanges; i++){
       for(j=tree->children[i].startRank; j<=tree->children[i].endRank; j++,child_count++){

            /*Setup the dependencies for posting the recv for child's data*/
            if(per_child_buffer){/*no dependency, just post the receive*/
                nvtcs = 0;
            }else{
                if(child_count==0)/*this is the first recv and therefore no dependency*/
                    nvtcs=0;
                else{
                   /*wait for the previous reduce to complete, before posting the next receive*/
                    vtcs[0] = reduce_id[child_count-1];
                    nvtcs = 1;
                }
            }
            if(0) fprintf(stderr, "Schedule receive from child %d\n", j);
            if(0) fprintf(stderr, "Posting receive at address %p\n", childbuf[child_count]);
            /*post the recv*/
            recv_id[child_count] = TSP_recv(childbuf[child_count],count,datatype,
                                            j,tag,tsp_comm, sched, nvtcs, vtcs);
            
            if(0) fprintf(stderr, "receive scheduled\n");
            /*Reduction depends on the data copy to complete*/
            nvtcs = 0;
            if((is_root && !is_inplace) || is_intermediate){ //this is when data copy was done
                nvtcs = 1; 
                vtcs[0] = dtcopy_id;
            }
            if(0) fprintf(stderr, "added datacopy dependency (if applicable)\n");
            /*reduction depends on the corresponding recv to complete*/
            vtcs[nvtcs] = recv_id[child_count]; 
            nvtcs+=1;
            
            if(0) fprintf(stderr, "schedule reduce\n");
            if (is_commutative) {/*reduction order does not matter*/
            }
            else{
                 /* non commutative case. Order of reduction matters. Knomia tree has to constructed differently for reduce for optimal performace. 
     *              If it is non commutative the tree is Knomial and assumes that children are in increasing order. Note: Works only when root is zero */
                /*wait for the previous reduce to complete*/
                if(child_count!=0){
                    vtcs[nvtcs] = reduce_id[child_count-1]; 
                    nvtcs++;
                }
            }
            reduce_id[child_count] = TSP_reduce_local(childbuf[child_count],target_buf, count,
                                                       datatype,op, sched, nvtcs, vtcs);
        }
   }
    
    if(!is_root){
        if(0) fprintf(stderr, "schedule send to parent %d\n", tree->parent);
        if(is_leaf){
            nvtcs=0; /*just send the data to parent*/
            target_buf = sendbuf;
        }
        else if(is_commutative){//wait for all the reductions to complete
            nvtcs = num_children;
            for(i=0; i<num_children; i++)
                vtcs[i] = reduce_id[i];
        }else {/*Just wait for the last reduce to complete*/
                nvtcs = 1;
                vtcs[0] = reduce_id[num_children-1];
        }
    
        /* send date to parent */
        TSP_send(target_buf, count, datatype, tree->parent,
                 tag, tsp_comm, sched, nvtcs, vtcs);
    }
    
    if(0) fprintf(stderr, "completed schedule generation\n");
    TSP_free_mem(childbuf);
    TSP_free_mem(vtcs);
    TSP_free_mem(recv_id);
    TSP_free_mem(reduce_id);
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
