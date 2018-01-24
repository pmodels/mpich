/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "mt_pt2pt_common.h"

int test_sendrecv(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < ITER; i++) {
            buff[i] = i + id;
            SEND_FUN(&buff[i], 1, MPI_INT, 1, tag, comm);
        }
    } else if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            buff[i] = -1;
            MPI_Recv(&buff[i], 1, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
            if (verify && buff[i] != i + id) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected %d but got %d\n", id, i + id, buff[i]);
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
    }

    return errs;
}

int test_sendrecv_huge(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;

    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
        for (i = 0; i < COUNT; i++)
            buff[i] = i + id;
    } else {
        for (i = 0; i < COUNT; i++)
            buff[i] = -1;
    }

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < ITER; i++)
            SEND_FUN(buff, COUNT, MPI_INT, 1, tag, comm);
    } else if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            MPI_Recv(buff, COUNT, MPI_INT, 0, tag, comm, MPI_STATUS_IGNORE);
            if (verify) {
                int j;
                for (j = 0; j < COUNT; j++) {
                    if (buff[j] != j + id) {
                        errs++;
                        if (errs <= DATA_WARN_THRESHOLD)
                            fprintf(stderr, "thread %d: Expected %d but got %d\n", id, i + id,
                                    buff[i]);
                    }
                }
                if (errs > 0)
                    fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
            }
        }
    }

    return errs;
}

int test_isendirecv(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Request reqs[ITER];

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < ITER; i++) {
            buff[i] = i + id;
            ISEND_FUN(&buff[i], 1, MPI_INT, 1, tag, comm, &reqs[i]);
        }
        /* MPI_Waitall() could be called from one leader thread instead,
         * but by doing this we can stress MPI_Waitall from multiple threads. */
        MPI_Waitall(ITER, reqs, MPI_STATUSES_IGNORE);
    } else if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            buff[i] = -1;
            MPI_Irecv(&buff[i], 1, MPI_INT, 0, tag, comm, &reqs[i]);
        }
        MPI_Waitall(ITER, reqs, MPI_STATUSES_IGNORE);

        for (i = 0; i < ITER; i++) {
            if (verify && buff[i] != i + id) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected %d but got %d\n", id, i + id, buff[i]);
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
    }

    return errs;
}

int test_isendirecv_huge(int id, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Request req;

    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
        for (i = 0; i < COUNT; i++)
            buff[i] = i + id;
    } else {
        for (i = 0; i < COUNT; i++)
            buff[i] = -1;
    }

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < ITER; i++) {
            ISEND_FUN(buff, COUNT, MPI_INT, 1, tag, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }
    } else if (rank == 1) {
        for (i = 0; i < ITER; i++) {
            MPI_Irecv(buff, COUNT, MPI_INT, 0, tag, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
            if (verify) {
                int j;
                for (j = 0; j < COUNT; j++) {
                    if (buff[j] != j + id) {
                        errs++;
                        if (errs <= DATA_WARN_THRESHOLD)
                            fprintf(stderr, "thread %d: Expected %d but got %d\n", id, i + id,
                                    buff[i]);
                    }
                }
                if (errs > 0)
                    fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
            }
        }
    }

    return errs;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;

    /* Send recv tests  */
#ifdef NONBLOCKING
#if defined(HUGE_COUNT) || defined(MEDIUM_COUNT)
    p->result = test_isendirecv_huge(p->id, p->buff, p->comm, p->tag, p->verify);
#else
    p->result = test_isendirecv(p->id, p->buff, p->comm, p->tag, p->verify);
#endif
#else /* #ifdef NONBLOCKING */
#if defined(HUGE_COUNT) || defined(MEDIUM_COUNT)
    p->result = test_sendrecv_huge(p->id, p->buff, p->comm, p->tag, p->verify);
#else
    p->result = test_sendrecv(p->id, p->buff, p->comm, p->tag, p->verify);
#endif
#endif /* #ifdef NONBLOCKING */
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}
