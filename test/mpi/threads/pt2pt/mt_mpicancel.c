/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include "mpithreadtest.h"
#include "mt_pt2pt_common.h"

#define DEFAULT_ITER 100

/*
  This programs does functional testing of multithreaded support for
  MPI_Cancel of MPI_Recv requests. This multi-threaded program posts
  irecv requests from rank 0 and cancels them, without any corresponding
  send requests.
*/

MTEST_THREAD_RETURN_TYPE run_test(void *arg)
{
    struct thread_param *p = (struct thread_param *) arg;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    /* Rank 0 posts MPI_Irecv to receive from rank 1, but cancels them */
    if (rank == 0) {
        int i, flag;
        int *buffer = malloc(p->iter * sizeof(int));
        MPI_Request *request = malloc(p->iter * sizeof(MPI_Request));
        MPI_Status *status = malloc(p->iter * sizeof(MPI_Status));

        /* Post irecv requests */
        for (i = 0; i < p->iter; i++)
            MPI_Irecv(buffer, 1, MPI_INT, 1, 1, p->comm, &request[i]);

        /* Post MPI_Cancel */
        for (i = 0; i < p->iter; i++)
            MPI_Cancel(&request[i]);
        MPI_Waitall(p->iter, request, status);

        /* Check for cancellation error */
        for (i = 0; i < p->iter; i++) {
            MPI_Test_cancelled(&status[i], &flag);
            if (!flag)
                p->result++;
        }
        free(buffer);
        free(request);
        free(status);
    }

    return (MTEST_THREAD_RETURN_TYPE) NULL;
}

/* Launch multiple threads */
static int cancel_recv_test(int iter)
{
    int i, errs = 0;
    struct thread_param params[NTHREADS];
    MPI_Comm dup_worlds[NTHREADS];

    for (i = 0; i < NTHREADS; i++) {
        MPI_Comm_dup(MPI_COMM_WORLD, &dup_worlds[i]);
        params[i].comm = dup_worlds[i];
        params[i].tag = 0;
        params[i].iter = iter;
        params[i].result = 0;
        if (i == NTHREADS - 1)
            run_test(&params[i]);
        else
            MTest_Start_thread(run_test, &params[i]);
    }
    MTest_Join_threads();

    /* Cleanup comm objects */
    for (i = 0; i < NTHREADS; i++) {
        MPI_Comm_free(&dup_worlds[i]);
        errs += params[i].result;
    }
    return errs;
}

int main(int argc, char **argv)
{
    int pmode, nprocs, errs = 0, err;

    MTest_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &pmode);
    if (pmode != MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI_THREAD_MULTIPLE not supported by the MPI implementation\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs < 2) {
        fprintf(stderr, "Need at least two processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    err = MTest_thread_barrier_init();
    if (err) {
        fprintf(stderr, "Could not create thread barrier\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    /* Run the test in a single rank */
    errs += cancel_recv_test(DEFAULT_ITER);
    MPI_Barrier(MPI_COMM_WORLD);

    MTest_Finalize(errs);
    return errs;
}
