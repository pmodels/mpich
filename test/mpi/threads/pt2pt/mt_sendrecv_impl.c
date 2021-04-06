/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mt_pt2pt_common.h"

int test_sendrecv(int id, int iter, int *buff, MPI_Comm comm, int tag, int verify);
int test_sendrecv_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify);
int test_isendirecv(int id, int iter, int *buff, MPI_Comm comm, int tag, int verify);
int test_isendirecv_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag,
                         int verify);
MTEST_THREAD_RETURN_TYPE run_test(void *arg);

int test_sendrecv(int id, int iter, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < iter; i++) {
            buff[i] = SETVAL(i, id);
            SEND_FUN(&buff[i], 1, type, 1, tag, comm);
        }
    } else if (rank == 1) {
        for (i = 0; i < iter; i++) {
            buff[i] = -1;
            MPI_Recv(&buff[i], 1, type, 0, tag, comm, MPI_STATUS_IGNORE);
            if (!verify)
                continue;
            if (buff[i] != SETVAL(i, id)) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected %d but got %d\n", id, SETVAL(i, id),
                            buff[i]);
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
    }

  fn_exit:
    return errs;
}

int test_sendrecv_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < count; i++)
            buff[i] = SETVAL(i, id);
        for (i = 0; i < iter; i++)
            SEND_FUN(buff, count, type, 1, tag, comm);
    } else if (rank == 1) {
        for (i = 0; i < iter; i++) {
            int j;
            for (j = 0; j < count; j++)
                buff[j] = -1;
            MPI_Recv(buff, count, type, 0, tag, comm, MPI_STATUS_IGNORE);
            if (!verify)
                continue;
            for (j = 0; j < count; j++) {
                if (buff[j] != SETVAL(j, id)) {
                    errs++;
                    if (errs <= DATA_WARN_THRESHOLD)
                        fprintf(stderr, "thread %d: Expected %d but got %d\n", id,
                                SETVAL(j, id), buff[j]);
                }
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
    }

  fn_exit:
    return errs;
}

int test_isendirecv(int id, int iter, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Request *reqs = malloc(sizeof(MPI_Request) * iter);
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < iter; i++) {
            buff[i] = SETVAL(i, id);
            ISEND_FUN(&buff[i], 1, type, 1, tag, comm, &reqs[i]);
        }
        /* MPI_Waitall() could be called from one leader thread instead,
         * but by doing this we can stress MPI_Waitall from multiple threads. */
        MPI_Waitall(iter, reqs, MPI_STATUSES_IGNORE);
    } else if (rank == 1) {
        for (i = 0; i < iter; i++) {
            buff[i] = -1;
            MPI_Irecv(&buff[i], 1, type, 0, tag, comm, &reqs[i]);
        }
        MPI_Waitall(iter, reqs, MPI_STATUSES_IGNORE);
        if (verify) {
            for (i = 0; i < iter; i++) {
                if (buff[i] != SETVAL(i, id)) {
                    errs++;
                    if (errs <= DATA_WARN_THRESHOLD)
                        fprintf(stderr, "thread %d: Expected %d but got %d\n", id, SETVAL(i, id),
                                buff[i]);
                }
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);

    }

  fn_exit:
    free(reqs);
    return errs;
}

int test_isendirecv_huge(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, j, rank;
    MPI_Request req;
    MPI_Datatype type = MPI_INT;

    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
        for (i = 0; i < count; i++)
            buff[i] = SETVAL(i, id);
    }

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    if (rank == 0) {
        for (i = 0; i < iter; i++) {
            for (j = 0; j < count; j++)
                buff[i] = SETVAL(i, id);
            ISEND_FUN(buff, count, type, 1, tag, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
        }
    } else if (rank == 1) {
        for (i = 0; i < iter; i++) {
            /* reset recv buffer before every recv. */
            for (j = 0; j < count; j++)
                buff[j] = -1;
            MPI_Irecv(buff, count, type, 0, tag, comm, &req);
            MPI_Wait(&req, MPI_STATUS_IGNORE);
            if (!verify)
                continue;
            for (j = 0; j < count; j++) {
                if (buff[j] != SETVAL(j, id)) {
                    errs++;
                    if (errs <= DATA_WARN_THRESHOLD)
                        fprintf(stderr, "thread %d: Expected %d but got %d\n", id,
                                SETVAL(j, id), buff[j]);
                }
            }
        }
        if (errs > 0)
            fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);
    }

  fn_exit:
    return errs;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;

    /* Send recv tests  */
#ifdef NONBLOCKING
#if defined(HUGE_COUNT)
    p->result = test_isendirecv_huge(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);
#else
    p->result = test_isendirecv(p->id, p->iter, p->buff, p->comm, p->tag, p->verify);
#endif
#else /* #ifdef NONBLOCKING */
#if defined(HUGE_COUNT)
    p->result = test_sendrecv_huge(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);
#else
    p->result = test_sendrecv(p->id, p->iter, p->buff, p->comm, p->tag, p->verify);
#endif
#endif /* #ifdef NONBLOCKING */
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}
