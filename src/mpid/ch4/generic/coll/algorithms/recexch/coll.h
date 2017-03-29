/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

static inline int COLL_init()
{
    return 0;
}

static inline int COLL_op_init(COLL_op_t * op, int id)
{
    op->id  = id;
    TSP_op_init(&op->tsp_op, COLL_OP_BASE(op));
    return 0;
}

static inline int COLL_dt_init(COLL_dt_t * dt, int id)
{
    dt->id = id;
    TSP_dt_init(&dt->tsp_dt, COLL_DT_BASE(dt));
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t * comm, int id, int* tag, int rank, int size)
{
    comm->id = id;
    comm->curTag = tag;
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    return 0;
}

static inline int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}


static inline int COLL_allreduce(const void  *sendbuf,
                                 void        *recvbuf,
                                 int          count,
                                 COLL_dt_t   *datatype,
                                 COLL_op_t   *op,
                                 COLL_comm_t *comm,
                                 int         *errflag)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->curTag)++;

    COLL_sched_init(&s);
    rc = COLL_sched_allreduce(sendbuf,recvbuf,count,
                              datatype,op,tag,comm,&s,1);

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
#if 0
    COLL_sched_t  *s;
    int                  done = 0;
    int                  tag = (*comm->curTag)++;
    COLL_sched_init_nb(&s,request);
    TSP_addref_op(&op->tsp_op,1);
    TSP_addref_dt(&datatype->tsp_dt,1);
    COLL_sched_allreduce(sendbuf,recvbuf,count,datatype,
                         op,tag,comm,s,0);
    TSP_fence(&s->tsp_sched);
    TSP_addref_op_nb(&op->tsp_op,0,&s->tsp_sched);
    TSP_addref_dt_nb(&datatype->tsp_dt,0,&s->tsp_sched);
    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);
    done = COLL_sched_kick_nb(s);
    if(1 || !done) {
        TAILQ_INSERT_TAIL(&COLL_progress_global.head,&request->elem,list_data);
    } else
        TSP_free_mem(s);
#endif
    return 0;
}
/*Unnecessary unless we do not have NB-colls here */
static inline int COLL_kick(COLL_queue_elem_t * elem)
{
    return 0;
}
