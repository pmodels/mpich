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

static inline int COLL_comm_init(COLL_comm_t * comm, int id, int *tag_ptr, int rank, int comm_size)
{
    comm->id = id;
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    COLL_tree_comm_t *mycomm = &comm->tree_comm;
    int k = COLL_TREE_RADIX_DEFAULT;
    mycomm->curTag = 0;

    COLL_tree_init(rank, comm_size, k, 0, &mycomm->tree);
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


static inline int COLL_allreduce(const void *sendbuf,
                                 void *recvbuf,
                                 int count,
                                 COLL_dt_t datatype,
                                 COLL_op_t op, COLL_comm_t * comm, int *errflag, int k)
{

    int rc = 0;
    COLL_args_t coll_args = {.algo = COLL_NAME,.nargs = 5,
        .args = {.allreduce =
                 {.sbuf = (void*)sendbuf,
                  .rbuf = recvbuf,
                  .count = count,
                  .dt_id = (int) datatype,
                  .op_id = (int)op}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /*Allocate or load from cache schedule (depends of transport implementation).
     * Reset scheduler if necessary. */
    TSP_sched_t *s = TSP_get_schedule( &comm->tsp_comm, (void*) &coll_args,
            sizeof(COLL_args_t), tag, &is_new);
    if (is_new) {
        rc = COLL_sched_allreduce_tree(sendbuf, recvbuf, count,
                    datatype, op, tag, comm, k, s, 1);
        TSP_save_schedule(&comm->tsp_comm, (void*)&coll_args,
                        sizeof(COLL_args_t), (void *) s);
    }
    COLL_sched_kick(s);
    return rc;
}


static inline int COLL_bcast(void *buffer,
                             int count,
                             COLL_dt_t datatype,
                             int root, COLL_comm_t * comm, int *errflag, int k, int segsize)
{
    int rc = 0;
    COLL_args_t coll_args = {.algo = COLL_NAME,.nargs = 6,
        .args = {.bcast =
                 {.buf = buffer,.count = count,.dt_id = (int) datatype,.root = root,
                     .k = k,.segsize = segsize}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /*Allocate or load from cache schedule (depends of transport implementation).
     * Reset scheduler if necessary. */
    TSP_sched_t *s = TSP_get_schedule( &comm->tsp_comm, (void*) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        MPIC_DBG("schedule does not exist\n");
        /* Generate schedule in case it wasn't loaded from cache'*/
        rc = COLL_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm, k, segsize,
                                             s, 1);
        /* Push schedule to cache (do nothing incase it wasn't implemented in transport)*/
        TSP_save_schedule(&comm->tsp_comm,  (void*)&coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_kick(s);
    return rc;
}

static inline int COLL_ibcast(void *buffer,
                             int count,
                             COLL_dt_t datatype,
                             int root, COLL_comm_t * comm, COLL_req_t *request, int k, int segsize)
{
    int rc = 0;
    COLL_args_t coll_args = {.algo = COLL_NAME,.nargs = 6,
        .args = {.bcast =
                 {.buf = buffer,.count = count,.dt_id = (int) datatype,.root = root,
                     .k = k,.segsize = segsize}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    TSP_sched_t *s = TSP_get_schedule( &comm->tsp_comm, (void*) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        MPIC_DBG("schedule does not exist\n");
        rc = COLL_sched_bcast_tree_pipelined(buffer, count, datatype, root, tag, comm, k, segsize,
                                             s, 1);
        TSP_save_schedule(&comm->tsp_comm, (void*)&coll_args, sizeof(COLL_args_t), (void *) s);
    }

    /* Enqueue schedule to NBC queue, and start it */
    COLL_sched_kick_nb(s, request);
    return rc;
}



static inline int COLL_reduce(const void *sendbuf,
                              void *recvbuf,
                              int count,
                              COLL_dt_t datatype,
                              COLL_op_t op, int root, COLL_comm_t * comm, int *errflag, int k,
                              int nbuffers)
{
    int rc = 0;
    COLL_args_t coll_args = {.algo = COLL_NAME,.nargs = 6,
        .args = {.reduce =
                 {.sbuf = (void*)sendbuf,
                  .rbuf = recvbuf,
                  .count = count,
                  .dt_id = (int) datatype,
                  .op_id = (int)op,
                  .root = root}}
    };


    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    /*Allocate or load from cache schedule (depends of transport implementation).
     * Reset scheduler if necessary. */
    TSP_sched_t *s = TSP_get_schedule( &comm->tsp_comm, (void*) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new)
    {
        rc = COLL_sched_reduce_tree_full(sendbuf, recvbuf, count,
                                datatype, op, root, tag, comm, k, s,
                                     1, nbuffers);
        TSP_save_schedule(&comm->tsp_comm, (void*)&coll_args, sizeof(COLL_args_t), (void *) s);
    }
    COLL_sched_kick(s);
    return rc;
}

static inline int COLL_barrier(COLL_comm_t * comm, int *errflag, int k)
{
    int rc = 0;
    COLL_args_t coll_args = {.algo = COLL_NAME,.nargs = 1,
        .args = {.barrier = {.k = k}}
    };

    int is_new = 0;
    int tag = (*comm->tree_comm.curTag)++;

    TSP_sched_t *s = TSP_get_schedule( &comm->tsp_comm,
                    (void*) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_barrier_tree(tag, comm, k, s);
        TSP_save_schedule(&comm->tsp_comm, (void*)&coll_args,
                        sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_kick(s);
    return rc;
}

