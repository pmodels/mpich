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

#include <sys/queue.h>

#ifndef COLL_NAMESPACE
#error "The collectives template must be namespaced with COLL_NAMESPACE"
#endif

#include "coll_schedule_dissem.h"
#include "coll_progress_util.h"

/*Initializa/setup the algorithm*/
MPL_STATIC_INLINE_PREFIX int COLL_init()
{
    return 0;
}

/*Initialize communicator for this algorithm*/
MPL_STATIC_INLINE_PREFIX int COLL_comm_init(COLL_comm_t * comm, int *tag_ptr, int rank, int comm_size)
{
    comm->curTag = tag_ptr;
    comm->rank = rank;
    comm->nranks = comm_size;
    TSP_comm_init(&comm->tsp_comm, COLL_COMM_BASE(comm));
    return 0;
}

/*clean up communicators*/
MPL_STATIC_INLINE_PREFIX int COLL_comm_cleanup(COLL_comm_t * comm)
{
    TSP_comm_cleanup(&comm->tsp_comm);
    return 0;
}


MPL_STATIC_INLINE_PREFIX int COLL_barrier(COLL_comm_t * comm, int *errflag)
{
    int rc = 0;
    COLL_args_t coll_args = {.nargs = 1,
        .args = {.barrier = {.k = 0}}
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_barrier_dissem(tag, comm, s);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_wait(s);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_ibarrier(COLL_comm_t * comm, COLL_req_t * request)
{
    int rc = 0;
    COLL_args_t coll_args = {.nargs = 1,
        .args = {.barrier = {.k = 0}}
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_barrier_dissem(tag, comm, s);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_kick_sched_nb(s, request);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_alltoall(const void *sendbuf,
                              int sendcount,
                              COLL_dt_t sendtype,
                              void *recvbuf,
                              int recvcount,
                              COLL_dt_t recvtype, COLL_comm_t * comm, int *errflag, int k, bool buffer_per_phase)
{
    int rc = 0;
    COLL_args_t coll_args = {.nargs = 8,
        .args = {.alltoall = {.sbuf = (void *) sendbuf,
                              .scount = sendcount,
                              .stype = (int) sendtype,
                              .rbuf = (void *) recvbuf,
                              .rcount = recvcount,
                              .rtype = (int) recvtype,
                              .k = k,
                              .buffer_per_phase = buffer_per_phase}}
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        rc = COLL_sched_alltoall_brucks(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm, tag, s, k, buffer_per_phase);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_wait(s);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_ialltoall(const void *sendbuf,
                               int sendcount,
                               COLL_dt_t sendtype,
                               void *recvbuf,
                               int recvcount,
                               COLL_dt_t recvtype, COLL_comm_t * comm, COLL_req_t * request, int k)
{
    int rc = 0;
    COLL_args_t coll_args = {.nargs = 6,
        .args = {.alltoall = {.sbuf = (void *) sendbuf,
                              .scount = sendcount,
                              .stype = (int) sendtype,
                              .rbuf = (void *) recvbuf,
                              .rcount = recvcount,
                              .rtype = (int) recvtype}}
    };

    int is_new = 0;
    int tag = (*comm->curTag)++;

    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {

        rc = COLL_sched_alltoall_brucks(sendbuf, sendcount, sendtype,
                                        recvbuf, recvcount, recvtype, comm, tag, s, k, 0);
        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_kick_sched_nb(s, request);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_allreduce(const void *sendbuf,
                               void *recvbuf,
                               int count,
                               COLL_dt_t datatype, COLL_op_t op, COLL_comm_t * comm, int *errflag)
{
    int rc, is_inplace, is_commutative, is_contig;
    size_t type_size, extent, lb;
    void *rbuf = recvbuf;
    void *sbuf = (void *) sendbuf;
    void *tmp_buf;
    int is_new = 0;
    int tag = (*comm->curTag)++;

    COLL_args_t coll_args = {.nargs = 5,
        .args = {.allreduce = {.sbuf = (void *) sendbuf,
                               .rbuf = recvbuf,
                               .count = count,
                               .dt_id = (int) datatype,
                               .op_id = (int) op}}
    };

    is_inplace = TSP_isinplace((void *) sendbuf);       /*is it in place collective operation */
    TSP_opinfo(op, &is_commutative);    /*check whether reduction operation is commutative */
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb); /*collect specifics of the data type */

    if (!is_commutative)
        return -1;      /*this implementatation currently does not handle non-commutative operations */


    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        if (is_inplace) {       /*allocate temporary buffer for receiving data */
            tmp_buf = TSP_allocate_buffer(extent * count, s);
            sbuf = recvbuf;
            rbuf = tmp_buf;
        }

        rc = COLL_sched_allreduce_dissem(sbuf, rbuf, count, datatype, op, tag, comm, s);

        int fenceid = TSP_fence(s);
        if (is_inplace) {       /*copy the data back to receive buffer */
            TSP_dtcopy_nb(recvbuf, count, datatype, tmp_buf, count, datatype, s, 1, &fenceid);
        }

        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_sched_wait(s);
    return rc;
}

MPL_STATIC_INLINE_PREFIX int COLL_iallreduce(const void *sendbuf,
                                void *recvbuf,
                                int count,
                                COLL_dt_t datatype,
                                COLL_op_t op, COLL_comm_t * comm, COLL_req_t * request)
{
    int rc, is_inplace, is_commutative, is_contig;
    size_t type_size, extent, lb;
    void *rbuf = recvbuf;
    void *sbuf = (void *) sendbuf;
    void *tmp_buf;

    COLL_args_t coll_args = {.nargs = 5,
        .args = {.allreduce = {.sbuf = (void *) sendbuf,
                               .rbuf = recvbuf,
                               .count = count,
                               .dt_id = (int) datatype,
                               .op_id = (int) op}}
    };

    is_inplace = TSP_isinplace((void *) sendbuf);       /*is it in place collective operation */
    TSP_opinfo(op, &is_commutative);    /*check whether reduction operation is commutative */
    TSP_dtinfo(datatype, &is_contig, &type_size, &extent, &lb); /*collect specifics of the data type */

    int is_new = 0;
    int tag = (*comm->curTag)++;


    if (!is_commutative)
        return -1;      /*this implementatation currently does not handle non-commutative operations */


    TSP_sched_t *s = TSP_get_schedule(&comm->tsp_comm,
                                      (void *) &coll_args, sizeof(COLL_args_t), tag, &is_new);

    if (is_new) {
        if (is_inplace) {       /*allocate temporary buffer for receiving data */
            tmp_buf = TSP_allocate_buffer(extent * count, s);
            sbuf = recvbuf;
            rbuf = tmp_buf;
        }

        rc = COLL_sched_allreduce_dissem(sbuf, rbuf, count, datatype, op, tag, comm, s);
        int fenceid = TSP_fence(s);
        if (is_inplace) {       /*copy the data back to receive buffer */
            TSP_dtcopy_nb(recvbuf, count, datatype, tmp_buf, count, datatype, s, 1, &fenceid);
        }

        TSP_save_schedule(&comm->tsp_comm, (void *) &coll_args, sizeof(COLL_args_t), (void *) s);
    }

    COLL_kick_sched_nb(s, request);
    return rc;

}
