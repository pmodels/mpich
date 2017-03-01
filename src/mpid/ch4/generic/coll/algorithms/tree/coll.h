/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>
#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif


static inline int COLL_init()
{
    return 0;
}

static inline int COLL_comm_init(COLL_comm_t * comm, int *tag_ptr, int rank, int comm_size)
{
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int k = COLL_TREE_RADIX_DEFAULT;
    mycomm->curTag = 0;

    COLL_tree_init( rank, comm_size, k, 0, &mycomm->tree);
    char *e = getenv("COLL_DUMPTREE");

    if (e && atoi(e))
        COLL_tree_dump(mycomm->tree.nranks, 0, k);

    comm->tree_comm.curTag = tag_ptr;
    return 0;
}

static inline int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}

static inline int COLL_op_init(COLL_op_t * op)
{
    TSP_op_init(&op->tsp_op, COLL_OP_BASE(op));
    return 0;
}

static inline int COLL_dt_init(COLL_dt_t * dt)
{
    TSP_dt_init(&dt->tsp_dt, COLL_DT_BASE(dt));
    return 0;
}


static inline int COLL_allreduce(const void *sendbuf,
                                 void *recvbuf,
                                 int count,
                                 COLL_dt_t * datatype,
                                 COLL_op_t * op, COLL_comm_t * comm, int *errflag, int k)
{
    int rc;
    COLL_sched_t s;
    int tag = (*comm->tree_comm.curTag)++;

    COLL_sched_init(&s);
    rc = COLL_sched_allreduce(sendbuf, recvbuf, count, datatype, op, tag, comm, k, &s, 1);
    COLL_sched_kick(&s);
    return rc;
}


static inline int COLL_bcast(void *buffer,
                             int count,
                             COLL_dt_t * datatype,
                             int root, COLL_comm_t * comm, int *errflag, int k)
{
    int rc;
    COLL_sched_t s;
    int tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_bcast(buffer, count, datatype, root, tag, comm, k, &s, 1);
    COLL_sched_kick(&s);
    return rc;
}


static inline int COLL_reduce(const void *sendbuf,
                              void *recvbuf,
                              int count,
                              COLL_dt_t * datatype,
                              COLL_op_t * op, int root, COLL_comm_t * comm, int *errflag, int k)
{
    int rc;
    COLL_sched_t s;
    int tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_reduce_full(sendbuf, recvbuf, count, datatype, op, root, tag, comm, k, &s, 1);
    COLL_sched_kick(&s);
    return rc;
}

static inline int COLL_barrier(COLL_comm_t *comm,
                               int         *errflag,
                               int          k)
{
    int                rc;
    COLL_sched_t s;
    int                tag = (*comm->tree_comm.curTag)++;
    COLL_sched_init(&s);
    rc = COLL_sched_barrier(tag, comm, k, &s);
    COLL_sched_kick(&s);
    return rc;
}


static inline int COLL_kick(COLL_queue_elem_t * elem)
{
    int done;
    COLL_sched_t *s = ((COLL_req_t *) elem)->phases;
    done = COLL_sched_kick_nb(s);

    if (done) {
        TAILQ_REMOVE(&COLL_progress_global.head, elem, list_data);
        TSP_sched_finalize(&s->tsp_sched);
        TSP_free_mem(s);
    }

    return done;
}
