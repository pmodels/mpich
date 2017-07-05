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

#include "coll_progress_util.h"
#include "coll_schedule_recexch.h"

MPL_STATIC_INLINE_PREFIX int COLL_init()
{
    return 0;
}


MPL_STATIC_INLINE_PREFIX int COLL_comm_init(COLL_comm_t * comm, int *tag, int rank, int size)
{
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    comm->curTag = tag;
    return 0;
}

MPL_STATIC_INLINE_PREFIX int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME COLL_allreduce
MPL_STATIC_INLINE_PREFIX int COLL_allreduce(const void *sendbuf,
                               void *recvbuf,
                               int count,
                               COLL_dt_t datatype,
                               COLL_op_t op, COLL_comm_t * comm, int per_nbr_buffer, int k, int *errflag)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_COLL_TREE_ALLREDUCE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_COLL_TREE_ALLREDUCE);
    int rc = 0;
    /*generate the key to search this schedule */
    COLL_args_t coll_args = {.coll_op=ALLREDUCE,
                             .args = {.allreduce = {.allreduce =
                                                      {.sbuf = (void *) sendbuf,
                                                       .rbuf = recvbuf,
                                                       .count = count,
                                                       .dt_id = (int) datatype,
                                                       .op_id = (int) op
                                                       },
                                                       .k=k}
                                       }
    };
    int is_new = 0;
    int tag = (*comm->curTag)++;

    /*Check if schedule already exists */
    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args,
                                      sizeof(COLL_args_t), tag, &is_new);
    if (is_new) {
        rc = COLL_sched_allreduce_recexch(sendbuf, recvbuf, count,
                                          datatype, op, tag, comm, k, s, per_nbr_buffer, 1);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }
    COLL_sched_wait(s);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_COLL_RECEXCH_ALLREDUCE);
    return rc;
}
MPL_STATIC_INLINE_PREFIX int COLL_iallreduce(const void *sendbuf,
                                void *recvbuf,
                                int count,
                                COLL_dt_t datatype,
                                COLL_op_t op,
                                COLL_comm_t * comm, COLL_req_t * request, int per_nbr_buffer, int k)
{
    int rc = 0;
    /*generate the key to search this schedule */
    COLL_args_t coll_args = {.coll_op=ALLREDUCE,
                             .args = {.allreduce = {.allreduce =
                                                      {.sbuf = (void *) sendbuf,
                                                       .rbuf = recvbuf,
                                                       .count = count,
                                                       .dt_id = (int) datatype,
                                                       .op_id = (int) op
                                                       },
                                                       .k=k}
                                       }
    };
    int is_new = 0;
    int tag = (*comm->curTag)++;

    /*Check if schedule already exists */
    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm, (void *) &coll_args,
                                      sizeof(COLL_args_t), tag, &is_new);
    if (is_new) {
        /*FIXME: Refounts should be inside */
        rc = COLL_sched_allreduce_recexch(sendbuf, recvbuf, count,
                                          datatype, op, tag, comm, k, s, per_nbr_buffer, 1);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }
    COLL_sched_test(s, request);
    return rc;

#if 0   /*The code needs to be tested */
    COLL_sched_t *s;
    int done = 0;
    int tag = (*comm->curTag)++;
    COLL_sched_init_nb(&s, request);
    TSP_addref_op(&op->tsp_op, 1);
    TSP_addref_dt(&datatype->tsp_dt, 1);
    COLL_sched_allreduce_recexch(sendbuf, recvbuf, count, datatype, op, tag, comm, s, 0);
    TSP_fence(&s->tsp_sched);
    TSP_addref_op_nb(&op->tsp_op, 0, &s->tsp_sched);
    TSP_addref_dt_nb(&datatype->tsp_dt, 0, &s->tsp_sched);
    TSP_fence(&s->tsp_sched);
    TSP_sched_commit(&s->tsp_sched);
    done = COLL_sched_test(s);
    if (1 || !done) {
        TAILQ_INSERT_TAIL(&COLL_progress_global.head, &request->elem, list_data);
    } else
        TSP_free_mem(s);
#endif
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_barrier(COLL_comm_t * comm, int *errflag, int k)
{
    int rc = 0;
    COLL_args_t coll_args = {.coll_op = BARRIER,
                             .args = {.barrier = {
                                       .barrier = {},
                                        .k = k}
                             }
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;
    void *recvbuf = NULL;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_allreduce_recexch(MPI_IN_PLACE, recvbuf, 0,
                                          MPI_BYTE, MPI_SUM, tag, comm, k, s, 0, 1);

        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_wait(s);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_allgather(const void *sendbuf,
                               int sendcount,
                               COLL_dt_t sendtype,
                               void *recvbuf,
                               int recvcount,
                               COLL_dt_t recvtype,
                               COLL_comm_t * comm, int k, int halving, int *errflag)
{
    int rc = 0;
    /*Check if schedule already exists */
    /*generate the key to search this schedule */
    COLL_args_t coll_args = { .coll_op = ALLGATHER,
                              .args = { .allgather = {
                                         .allgather = {
                                                .sbuf = (void *) sendbuf,
                                                .scount = sendcount,
                                                .st_id = (int) sendtype,
                                                .rbuf = recvbuf,
                                                .rcount = recvcount,
                                                .rt_id = (int) recvtype},
                                         .k=k,
                                         .halving = halving
                                      }
                 }
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_allgather_recexch(sendbuf, sendcount, sendtype, recvbuf,
                                          recvcount, recvtype, tag, comm, s, k, halving);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }
    COLL_sched_wait(s);
    return rc;
}
