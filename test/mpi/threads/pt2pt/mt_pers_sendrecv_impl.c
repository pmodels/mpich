/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mt_pt2pt_common.h"

/* Macros to generate Init function names for persistent send functions */
#define EXPAND_N_JOIN(x, y) x ## y
#define JOIN(x, y) EXPAND_N_JOIN(x, y)
#define SENDINIT JOIN(SEND_FUN, _init)

int pers_sendrecv_kernel(int id, int rank, int count, int *buff, MPI_Comm comm, int tag,
                         MPI_Request * r, MPI_Status * s, int verify);
int test_pers_sendrecv(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify);
MTEST_THREAD_RETURN_TYPE run_test(void *arg);

int pers_sendrecv_kernel(int id, int rank, int count, int *buff, MPI_Comm comm, int tag,
                         MPI_Request * r, MPI_Status * s, int verify)
{
    int errs = 0, i;

    if (rank == 0) {
        for (i = 0; i < count; i++)
            buff[i] = SETVAL(i, id);
        MPI_Start(r);
        MPI_Wait(r, s);
    } else if (rank == 1) {
        for (i = 0; i < count; i++)
            buff[i] = -1;
        MPI_Start(r);
        MPI_Wait(r, s);
        /* verify result */
        for (i = 0; i < count; i++) {
            if (verify && buff[i] != SETVAL(i, id)) {
                errs++;
                if (errs <= DATA_WARN_THRESHOLD)
                    fprintf(stderr, "thread %d: Expected %d but got %d\n", id, i + id, buff[i]);
            }
        }

    }
    return errs;
}

int test_pers_sendrecv(int id, int iter, int count, int *buff, MPI_Comm comm, int tag, int verify)
{
    int errs = 0, i, rank;
    MPI_Datatype type = MPI_INT;
    MPI_Comm_rank(comm, &rank);

    /* Make sure all threads have launched */
    MTest_thread_barrier(NTHREADS);

    MPI_Request r;
    MPI_Status s;
    /* Init objects */
    if (rank == 0) {
        SENDINIT(buff, count, type, 1, tag, comm, &r);
    } else {
        MPI_Recv_init(buff, count, type, 0, tag, comm, &r);
    }
    /* Init data and call send/recv */
    for (i = 0; i < iter; i++)
        errs = pers_sendrecv_kernel(id, rank, count, buff, comm, tag, &r, &s, verify);
    /* Release objects */
    MTest_thread_barrier(NTHREADS);
    MPI_Request_free(&r);
    if (errs > 0)
        fprintf(stderr, "thread %d: %d errors found in received data\n", id, errs);

    return errs;
}

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;
    p->result = test_pers_sendrecv(p->id, p->iter, p->count, p->buff, p->comm, p->tag, p->verify);
    return (MTEST_THREAD_RETURN_TYPE) NULL;
}
