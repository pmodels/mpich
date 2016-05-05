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

#include <sys/queue.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

static inline int COLL_init()
{
    TSP_init_control_dt(&COLL_global.control_dt);
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t *comm, int *tag_ptr)
{
    comm->tree_comm.tree.rank   = TSP_rank(&comm->tsp_comm);
    comm->tree_comm.tree.nranks = TSP_size(&comm->tsp_comm);
    comm->tree_comm.curTag      = tag_ptr;
    return 0;
}


static inline int COLL_barrier(COLL_comm_t *comm,
                               int         *errflag)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_barrier_dissem(tag, comm,&s);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_kick(COLL_queue_elem_t *elem)
{
    int                 done;
    COLL_sched_t *s    = ((COLL_req_t *)elem)->phases;
    done = COLL_sched_kick_nb(s);

    if(done) {
        TAILQ_REMOVE(&COLL_progress_global.head, elem, list_data);
        TSP_sched_finalize(&s->tsp_sched);
        TSP_free_mem(s);
    }

    return done;
}

static inline int COLL_ibarrier(COLL_comm_t *comm,
                                COLL_req_t  *request)
{
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag  = (*comm->tree_comm.curTag)++;
    COLL_sched_init_nb(&s,request);
    COLL_sched_barrier_dissem(tag,comm,s);
    done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);

    return 0;
}
static inline int COLL_alltoall(const void  *sendbuf,
                                int sendcount,
                                COLL_dt_t *sendtype,
                                void *recvbuf,
                                int recvcount,
                                COLL_dt_t *recvtype,
                                COLL_comm_t *comm,
                                int *errflag)
{
    int                 rc;
    COLL_sched_t  s;
    int                 tag     = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);

    rc = COLL_sched_alltoall(sendbuf,sendcount,sendtype,recvbuf,recvcount,recvtype,comm,tag,&s);

    TSP_fence(&s.tsp_sched);
    TSP_sched_commit(&s.tsp_sched);

    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_ialltoall(const void  *sendbuf,
                                 int sendcount,
                                 COLL_dt_t *sendtype,
                                 void *recvbuf,
                                 int recvcount,
                                 COLL_dt_t *recvtype,
                                 COLL_comm_t *comm,
                                 COLL_req_t *request)
{
    int           rc;
    COLL_sched_t  *s;
    int            tag     = (*comm->tree_comm.curTag)++;

    COLL_sched_init_nb(&s,request);

    rc = COLL_sched_alltoall(sendbuf,sendcount,sendtype,recvbuf,
                             recvcount,recvtype,comm,tag,s);

    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);

    int done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(&s);

    return rc;
}

static inline int COLL_allreduce(const void  *sendbuf,
                                 void        *recvbuf,
                                 int          count,
                                 COLL_dt_t   *datatype,
                                 COLL_op_t   *op,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    int                 rc, is_inplace, is_commutative, is_contig;
    size_t              type_size,extent,lb;
    COLL_sched_t  s;
    void               *tmp_buf;
    void               *sbuf    = (void *)sendbuf;
    void               *rbuf    = recvbuf;
    int                 tag     = (*comm->tree_comm.curTag)++;

    is_inplace = TSP_isinplace((void *)sendbuf);
    TSP_opinfo(&op->tsp_op,&is_commutative);
    TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);

    if(!is_commutative) return -1;

    COLL_sched_init(&s);

    if(is_inplace) {
        tmp_buf = TSP_allocate_mem(extent*count);
        sbuf    = recvbuf;
        rbuf    = tmp_buf;
    }

    rc = COLL_sched_allreduce_dissem(sbuf,rbuf,count,
                                     datatype,op,tag,comm,&s);

    if(is_inplace) {
        TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt,
                      tmp_buf,count,&datatype->tsp_dt,
                      &s.tsp_sched);

        TSP_fence(&s.tsp_sched);
        TSP_free_mem_nb(tmp_buf,&s.tsp_sched);
    }

    TSP_fence(&s.tsp_sched);
    TSP_sched_commit(&s.tsp_sched);

    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_iallreduce(const void  *sendbuf,
                                  void        *recvbuf,
                                  int          count,
                                  COLL_dt_t   *datatype,
                                  COLL_op_t   *op,
                                  COLL_comm_t *comm,
                                  COLL_req_t  *request)
{
    COLL_sched_t *s;
    int                 is_inplace,is_commutative,is_contig,rc,done = 0;
    size_t              type_size,extent,lb;
    int                 tag     = (*comm->tree_comm.curTag)++;
    void               *tmp_buf;
    void               *sbuf    = (void *)sendbuf;
    void               *rbuf    = recvbuf;

    is_inplace = TSP_isinplace((void *)sendbuf);
    TSP_opinfo(&op->tsp_op,&is_commutative);
    TSP_dtinfo(&datatype->tsp_dt,&is_contig,&type_size,&extent,&lb);

    if(!is_commutative) return -1;

    COLL_sched_init_nb(&s,request);

    if(is_inplace) {
        tmp_buf = TSP_allocate_mem(extent*count);
        sbuf    = recvbuf;
        rbuf    = tmp_buf;
    }

    rc = COLL_sched_allreduce_dissem(sbuf,rbuf,count,
                                     datatype,op,tag,comm,
                                     s);

    if(is_inplace) {
        TSP_dtcopy_nb(recvbuf,count,&datatype->tsp_dt,
                      tmp_buf,count,&datatype->tsp_dt,
                      &s->tsp_sched);
        TSP_fence(&s->tsp_sched);
        TSP_free_mem_nb(tmp_buf,&s->tsp_sched);
    }

    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);

    done = COLL_sched_kick_nb(s);

    if(1 || !done) { /* always enqueue until we can fix the request interface */
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);

    return rc;
}
