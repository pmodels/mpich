/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
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

static inline
int COLL_tree_kary(int                 rank,
                   int                 k,
                   int                 numranks,
                   int                *parent,
                   COLL_child_range_t  children[])
{
    int child;
    int numRanges=0;
    assert(k >=1);

    *parent = (rank<=0)?-1:(rank-1)/k;

    for(child=1; child<=k; child++) {
        int val = rank*k+child;

        if(val >= numranks)
            break;

        if(child==1) {
            children[0].startRank=val;
            children[0].endRank=val;
        } else
            children[0].endRank=val;

        numRanges=1;
    }

    return numRanges;
}

static inline
int COLL_tree_knomial(int                 rank,
                      int                 k,
                      int                 numranks,
                      int                *parent,
                      COLL_child_range_t  children[])
{
    int num_children = 0;
    int basek, i, j;
    assert(k>=2);

    /* Parent Calculation */
    if(rank<=0) *parent=-1;
    else {
        basek = COLL_ilog(k, numranks-1);

        for(i=0; i<basek; i++) {
            if(COLL_getdigit(k,rank,i)) {
                *parent = COLL_setdigit(k, rank, i, 0);
                break;
            }
        }
    }

    if(rank>=numranks)
        return -1;

    /* Children Calculation */
    num_children = 0;
    basek        = COLL_ilog(k, numranks-1);

    for(j=0; j<basek; j++) {
        if(COLL_getdigit(k, rank, j))
            break;

        for(i=1; i<k; i++) {
            int child = COLL_setdigit(k, rank, j, i);

            if(child < numranks) {
                assert(num_children < COLL_MAX_TREE_BREADTH);
                children[num_children].startRank = child;
                children[num_children].endRank   = child;
                num_children++;
            }
        }
    }

    return num_children;
}

static inline
void COLL_tree_kary_init(int          rank,
                         int          nranks,
                         int          k,
                         COLL_tree_t *ct)
{
    ct->rank      = rank;
    ct->nranks    = nranks;
    ct->numRanges = COLL_tree_kary(ct->rank,
                                   k,
                                   ct->nranks,
                                   &ct->parent,
                                   ct->children);
}

static inline
void COLL_tree_knomial_init(int          rank,
                            int          nranks,
                            int          k,
                            COLL_tree_t *ct)
{
    ct->rank        = rank;
    ct->nranks      = nranks;
    ct->numRanges   = COLL_tree_knomial(ct->rank,
                                        k,
                                        ct->nranks,
                                        &ct->parent,
                                        ct->children);
}

static inline
void COLL_tree_init(int          rank,
                    int          nranks,
                    int          k,
                    COLL_tree_t *ct)
{
#if COLL_USE_KNOMIAL == 1
    COLL_tree_knomial_init(rank,nranks,k,ct);
#else
    COLL_tree_kary_init(rank,nranks,k,ct);
#endif
}

static inline
int COLL_tree_dump(int tree_size)
{
    int   i;
    const char *name       = "collective tree";
    int   tree_radix = COLL_TREE_RADIX;

    fprintf(stderr, "digraph \"%s%d\" {\n",
            name, tree_radix);

    for(i=0; i<tree_size; i++) {
        COLL_tree_t ct;
        COLL_tree_init(i,tree_size,tree_radix,&ct);
        fprintf(stderr, "%d -> %d\n",ct.rank, ct.parent);
        int j;

        for(j=0; j< ct.numRanges;  j++) {
            int k;

            for(k=ct.children[j].startRank;
                k<=ct.children[j].endRank;
                k++) {
                fprintf(stderr, "%d -> %d\n",ct.rank,k);
            }
        }
    }

    fprintf(stderr, "}\n");

    return 0;
}

static inline int
COLL_sched_barrier(int                 tag,
                   COLL_comm_t        *comm,
                   COLL_sched_t *s)
{
    int i, j;
    COLL_tree_comm_t *mycomm    = &comm->tree_comm;
    COLL_tree_t      *tree      = &mycomm->tree;
    int               numRanges = tree->numRanges;
    TSP_dt_t         *dt        = &COLL_global.control_dt;
    int               children  = 0;

    SCHED_FOREACHCHILDDO(children++);

    if(tree->parent == -1) {
        SCHED_FOREACHCHILDDO(TSP_recv(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched));

        TSP_fence(&s->tsp_sched);
        SCHED_FOREACHCHILDDO(TSP_send(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched));
        TSP_fence(&s->tsp_sched);
    } else {
        /* Receive from all children */
        SCHED_FOREACHCHILDDO(TSP_recv(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched));
        TSP_fence(&s->tsp_sched);
        /* Send to Parent      */
        TSP_send(NULL,0,dt,tree->parent,tag,&comm->tsp_comm,&s->tsp_sched);
        TSP_fence(&s->tsp_sched);
        /* Receive from Parent */
        TSP_recv(NULL,0,dt,tree->parent,tag,&comm->tsp_comm,&s->tsp_sched);
        TSP_fence(&s->tsp_sched);
        /* Send to all children */
        SCHED_FOREACHCHILDDO(TSP_send(NULL,0,dt,j,tag,&comm->tsp_comm,
                                      &s->tsp_sched));
        TSP_fence(&s->tsp_sched);
    }

    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);
    return 0;
}


static inline int
COLL_sched_bcast(void               *buffer,
                 int                 count,
                 COLL_dt_t          *datatype,
                 int                 root,
                 int                 tag,
                 COLL_comm_t        *comm,
                 COLL_sched_t *s,
                 int                 finalize)
{
    COLL_tree_t       logicalTree;
    int               i,j,numRanges;
    int               k        = COLL_TREE_RADIX;
    COLL_tree_comm_t *mycomm   = &comm->tree_comm;
    int               size     = mycomm->tree.nranks;
    int               rank     = mycomm->tree.rank;
    int               lrank    = (rank+(size-root))%size;
    COLL_tree_t      *tree     = &logicalTree;
    int               children = 0;

    COLL_tree_init(lrank,size,k,tree);
    numRanges = tree->numRanges;
    SCHED_FOREACHCHILDDO(children++);

    if(tree->parent == -1) {
        SCHED_FOREACHCHILDDO(TSP_send(buffer,count,&datatype->tsp_dt,
                                      (j+root)%size,tag,&comm->tsp_comm,
                                      &s->tsp_sched));
        TSP_fence(&s->tsp_sched);
    } else {
        /* Receive from Parent */
        TSP_recv(buffer,count,&datatype->tsp_dt,
                 (tree->parent+root)%size,tag,&comm->tsp_comm,
                 &s->tsp_sched);
        TSP_fence(&s->tsp_sched);

        /* Send to all children */
        SCHED_FOREACHCHILDDO(TSP_send(buffer,count,&datatype->tsp_dt,
                                      (j+root)%size,tag,&comm->tsp_comm,
                                      &s->tsp_sched));
        TSP_fence(&s->tsp_sched);
    }

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

static inline int
COLL_sched_reduce(const void         *sendbuf,
                  void               *recvbuf,
                  int                 count,
                  COLL_dt_t          *datatype,
                  COLL_op_t          *op,
                  int                 root,
                  int                 tag,
                  COLL_comm_t        *comm,
                  int                 is_commutative,
                  COLL_sched_t *s,
                  int                 finalize)
{
    COLL_tree_t       logicalTree;
    int               i,j,numRanges,is_contig;
    size_t            lb, extent, type_size;
    int               k          = COLL_TREE_RADIX;
    COLL_tree_comm_t *mycomm     = &comm->tree_comm;
    int               size       = mycomm->tree.nranks;
    int               rank       = mycomm->tree.rank;
    int               lrank      = (rank+(size-root))%size;
    COLL_tree_t      *tree       = &logicalTree;
    void             *free_ptr0  = NULL;
    int               children   = 0;
    int               is_inplace = TSP_isinplace((void *)sendbuf);

    TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);

    if(is_commutative)
        COLL_tree_init(lrank,size,k,tree);
    else
        /* We have to use knomial trees to get rank order */
        COLL_tree_knomial_init(lrank,size,k,tree);

    numRanges = tree->numRanges;
    SCHED_FOREACHCHILDDO(children++);

    if(tree->parent == -1) {
        k=0;

        if(!is_inplace)TSP_dtcopy(recvbuf,count,&datatype->tsp_dt,
                                      sendbuf,count,&datatype->tsp_dt);

        if(is_commutative) {
            SCHED_FOREACHCHILD()
            TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,
                            &op->tsp_op,(j+root)%size,tag,&comm->tsp_comm,
                            &s->tsp_sched,TSP_FLAG_REDUCE_L);
            TSP_fence(&s->tsp_sched);
        } else {
            SCHED_FOREACHCHILD() {
                TSP_recv_reduce(recvbuf,count,&datatype->tsp_dt,
                                &op->tsp_op,(j+root)%size,tag,&comm->tsp_comm,
                                &s->tsp_sched,TSP_FLAG_REDUCE_R);
                TSP_fence(&s->tsp_sched);
            }
        }
    } else {
        void *result_buf;
        free_ptr0  = result_buf = TSP_allocate_mem(extent*count);
        result_buf = (void *)((char *)result_buf-lb);
        k          = 0;
        TSP_dtcopy(result_buf,count,&datatype->tsp_dt,sendbuf,count,&datatype->tsp_dt);
        {
            if(is_commutative) {
                SCHED_FOREACHCHILD()
                TSP_recv_reduce(result_buf,count,&datatype->tsp_dt,
                                &op->tsp_op,(j+root)%size,tag,&comm->tsp_comm,
                                &s->tsp_sched,TSP_FLAG_REDUCE_L);
                TSP_fence(&s->tsp_sched);
            } else {
                SCHED_FOREACHCHILD() {
                    TSP_recv_reduce(result_buf,count,&datatype->tsp_dt,
                                    &op->tsp_op,(j+root)%size,tag,&comm->tsp_comm,
                                    &s->tsp_sched,TSP_FLAG_REDUCE_R);
                    TSP_fence(&s->tsp_sched);
                }
            }
        }
        TSP_fence(&s->tsp_sched);
        TSP_send_accumulate(result_buf,count,&datatype->tsp_dt,
                            &op->tsp_op,(tree->parent+root)%size,
                            tag,&comm->tsp_comm,&s->tsp_sched);
    }

    TSP_fence(&s->tsp_sched);

    if(free_ptr0) TSP_free_mem_nb(free_ptr0,&s->tsp_sched);

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}

static inline int
COLL_sched_reduce_full(const void         *sendbuf,
                       void               *recvbuf,
                       int                 count,
                       COLL_dt_t          *datatype,
                       COLL_op_t          *op,
                       int                 root,
                       int                 tag,
                       COLL_comm_t        *comm,
                       COLL_sched_t *s,
                       int                 finalize)
{
    int       is_commutative,rc;
    TSP_opinfo(&op->tsp_op,&is_commutative);

    if(root == 0 || is_commutative) {
        rc = COLL_sched_reduce(sendbuf,recvbuf,count,datatype,
                               op,root,tag,comm,
                               is_commutative,s,finalize);
    } else {
        COLL_tree_comm_t *mycomm    = &comm->tree_comm;
        int               rank      = mycomm->tree.rank;
        size_t            lb,extent,type_size;
        int               is_contig;
        int               is_inplace = TSP_isinplace((void *)sendbuf);

        TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);
        void *tmp_buf = NULL;
        void *sb      = NULL;

        if(rank == 0)
            tmp_buf = TSP_allocate_mem(extent*count);

        if(rank == root && is_inplace)
            sb = recvbuf;
        else
            sb = (void *)sendbuf;

        rc = COLL_sched_reduce(sb,tmp_buf,count,datatype,
                               op,0,tag,comm,is_commutative,s,0);

        if(rank == 0) {
            TSP_send(tmp_buf,count,&datatype->tsp_dt,
                     root,tag,&comm->tsp_comm,&s->tsp_sched);
            TSP_fence(&s->tsp_sched);
        } else if(rank == root) {
            TSP_recv(recvbuf,count,&datatype->tsp_dt,
                     0,tag,&comm->tsp_comm,&s->tsp_sched);
            TSP_fence(&s->tsp_sched);
        }

        if(tmp_buf) {
            TSP_free_mem_nb(tmp_buf,&s->tsp_sched);
            TSP_fence(&s->tsp_sched);
        }

        if(finalize) {
            TSP_fence(&s->tsp_sched);
            TSP_sched_commit(&s->tsp_sched);
        }
    }

    return rc;
}

static inline int
COLL_sched_allreduce(const void         *sendbuf,
                     void               *recvbuf,
                     int                 count,
                     COLL_dt_t          *datatype,
                     COLL_op_t          *op,
                     int                 tag,
                     COLL_comm_t        *comm,
                     COLL_sched_t *s,
                     int                 finalize)
{
    int    is_commutative, is_inplace,is_contig;
    size_t lb,extent,type_size;
    void *tmp_buf = NULL;
    void *sbuf    = (void *)sendbuf;
    void *rbuf    = recvbuf;

    is_inplace = TSP_isinplace((void *)sendbuf);
    TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);
    TSP_opinfo(&op->tsp_op,&is_commutative);

    if(is_inplace) {
        tmp_buf = TSP_allocate_mem(extent*count);
        sbuf    = recvbuf;
        rbuf    = tmp_buf;
    }

    COLL_sched_reduce(sbuf,rbuf,count,datatype,
                      op,0,tag,comm,is_commutative,s,0);
    COLL_sched_bcast(rbuf,count,datatype,0,tag,comm,s,0);

    if(is_inplace) {
        TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt,
                      tmp_buf,count,&datatype->tsp_dt,
                      &s->tsp_sched);
        TSP_fence(&s->tsp_sched);
        TSP_free_mem_nb(tmp_buf,&s->tsp_sched);
    }

    if(finalize) {
        TSP_fence(&s->tsp_sched);
        TSP_sched_commit(&s->tsp_sched);
    }

    return 0;
}
