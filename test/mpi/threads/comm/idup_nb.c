/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* This test tests the overlapping of MPI_Comm_idup with other nonblocking calls
  in a multithreaded setting. Each process produces a new communicator for each
  of its threads.  A thread will duplicate it again to produce child communicators.
  When the first child communicator is ready, the thread duplicates it to produce
  grandchild communicators. Meanwhile, the thread also issues nonblocking calls,
  such as idup, iscan, ibcast, iallreduce, ibarrier on the communicators available.

  The test tests both intracommunicators and intercommunicators
*/

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"

#define NUM_THREADS  3  /* threads to spawn per process, must >= 1 */
#define NUM_IDUPS1   3  /* child communicators to iduplicate per thread, must >= 1 */
#define NUM_IDUPS2   3  /* grandchild communicators to iduplicate per thread, must >= 1 */
#define NUM_ITER     1  /* run the kernel this times */

#define check(X_)       \
    do {                \
        if (!(X_)) {    \
            printf("[%s:%d] -- Assertion failed: %s\n", __FILE__, __LINE__, #X_);\
            MPI_Abort(MPI_COMM_WORLD, 1); \
        }               \
    } while (0)

int isLeft;                     /* Is left group of an intercomm? */
MPI_Comm parentcomms[NUM_THREADS];
MPI_Comm nbrcomms[NUM_THREADS];
int errs[NUM_THREADS] = { 0 };

int verbose = 0;

/* Threads idup the communicator assigned to them NUM_IDUPS1 times. The operation
   is overlapped with other non-blocking operations on the same communicator or
   on a different communicator.
*/
MTEST_THREAD_RETURN_TYPE test_intracomm(void *arg)
{
    int i, j;
    int root, bcastbuf;
    int rank, size;
    int ans[4], expected[4];
    MPI_Request reqs[NUM_IDUPS1 + NUM_IDUPS2 + 10] = { MPI_REQUEST_NULL };      /* Preallocate enough reqs */
    MPI_Comm comms[NUM_IDUPS1 + NUM_IDUPS2];    /* Hold all descendant comms */
    int cnt;
    int tid = *(int *) arg;

    MPI_Comm parentcomm = parentcomms[tid];
    MPI_Comm nbrcomm = nbrcomms[tid];

    for (i = 0; i < NUM_ITER; i++) {
        cnt = 0;
        if (*(int *) arg == rank)
            MTestSleep(1);

        if (verbose)
            printf("%d: Thread %d - comm_idup %d start\n", rank, tid, i);

        /* Idup the parent intracomm NUM_IDUPS1 times */
        for (j = 0; j < NUM_IDUPS1; j++)
            MPI_Comm_idup(parentcomm, &comms[j], &reqs[cnt++]);

        /* Issue an iscan on parent comm to overlap with the pending idups */
        MPI_Comm_rank(parentcomm, &rank);
        MPI_Iscan(&rank, &ans[0], 1, MPI_INT, MPI_SUM, parentcomm, &reqs[cnt++]);
        expected[0] = rank * (rank + 1) / 2;
        /* Wait for the first child comm to be ready */
        MPI_Wait(&reqs[0], MPI_STATUS_IGNORE);

        /* Do Idups & iallreduce on the first child comm simultaneously */
        for (j = 0; j < NUM_IDUPS2; j++)
            MPI_Comm_idup(comms[0], &comms[NUM_IDUPS1 + j], &reqs[cnt++]);

        MPI_Comm_size(comms[0], &size);
        MPI_Iallreduce(&rank, &ans[1], 1, MPI_INT, MPI_SUM, comms[0], &reqs[cnt++]);
        expected[1] = (size - 1) * size / 2;

        /* Issue an ibcast on the parent comm */
        MPI_Comm_rank(parentcomm, &rank);
        ans[2] = (rank == 0) ? 199 : 111;
        MPI_Ibcast(&ans[2], 1, MPI_INT, 0, parentcomm, &reqs[cnt++]);
        expected[2] = 199;

        /* Do ibarrier on the dup'ed comm */
        MPI_Ibarrier(comms[0], &reqs[cnt++]);

        /* Issue an iscan on a neighbor comm */
        MPI_Comm_rank(nbrcomm, &rank);
        MPI_Iscan(&rank, &ans[3], 1, MPI_INT, MPI_SUM, nbrcomm, &reqs[cnt++]);
        expected[3] = rank * (rank + 1) / 2;

        /* Pending operations include idup/iscan/ibcast on parentcomm
         * idup/Iallreduce/Ibarrier on comms[0], and Iscan on nbrcomm */
        /* Waitall even if the first request is completed */
        MPI_Waitall(cnt, reqs, MPI_STATUSES_IGNORE);

        /* Check the answers */
        for (j = 0; j < 4; j++) {
            if (ans[j] != expected[j])
                errs[tid]++;
        }
        for (j = 0; j < NUM_IDUPS1 + NUM_IDUPS2; j++) {
            errs[tid] += MTestTestComm(comms[j]);
            MPI_Comm_free(&comms[j]);
        }
        if (verbose)
            printf("\t%d: Thread %d - comm_idup %d finish\n", rank, tid, i);
    }
    if (verbose)
        printf("%d: Thread %d - Done.\n", rank, tid);
    return (MTEST_THREAD_RETURN_TYPE) 0;
}

/* Threads idup the communicator assigned to them NUM_IDUPS1 times. The operation
   is overlapped with other non-blocking operations on the same communicator or
   on a different communicator.
*/
MTEST_THREAD_RETURN_TYPE test_intercomm(void *arg)
{
    int rank, rsize, root;
    int i, j;
    int tid = *(int *) arg;
    int ans[4], expected[4];
    MPI_Comm parentcomm = parentcomms[tid];
    MPI_Comm nbrcomm = nbrcomms[tid];

    MPI_Request reqs[NUM_IDUPS1 + NUM_IDUPS2 + 10] = { MPI_REQUEST_NULL };      /* Preallocate enough reqs */
    MPI_Comm comms[NUM_IDUPS1 + NUM_IDUPS2];    /* Hold all descendant comms */
    int cnt;

    for (i = 0; i < NUM_ITER; i++) {
        cnt = 0;
        if (*(int *) arg == rank)
            MTestSleep(1);

        if (verbose)
            printf("%d: Thread %d - comm_idup %d start\n", rank, tid, i);

        /* Idup the parent intracomm multiple times */
        for (j = 0; j < NUM_IDUPS1; j++)
            MPI_Comm_idup(parentcomm, &comms[j], &reqs[cnt++]);

        /* Issue an Iallreduce on parentcomm */
        MPI_Comm_rank(parentcomm, &rank);
        MPI_Comm_remote_size(parentcomm, &rsize);
        MPI_Iallreduce(&rank, &ans[0], 1, MPI_INT, MPI_SUM, parentcomm, &reqs[cnt++]);
        expected[0] = (rsize - 1) * rsize / 2;
        /* Wait for the first child comm to be ready */
        MPI_Wait(&reqs[0], MPI_STATUS_IGNORE);

        /* Do idup & iallreduce on the first child comm simultaneously */
        for (j = 0; j < NUM_IDUPS2; j++)
            MPI_Comm_idup(comms[0], &comms[NUM_IDUPS1 + j], &reqs[cnt++]);

        MPI_Comm_rank(comms[0], &rank);
        MPI_Comm_remote_size(comms[0], &rsize);
        MPI_Iallreduce(&rank, &ans[1], 1, MPI_INT, MPI_SUM, comms[0], &reqs[cnt++]);
        expected[1] = (rsize - 1) * rsize / 2;

        /* Issue an ibcast on parentcomm */
        MPI_Comm_rank(parentcomm, &rank);
        if (isLeft) {
            if (rank == 0) {
                root = MPI_ROOT;
                ans[2] = 199;
            }
            else {
                root = MPI_PROC_NULL;
                ans[2] = 199;   /* not needed, just to make correctness checking easier */
            }
        }
        else {
            root = 0;
            ans[2] = 111;       /* garbage value */
        }
        MPI_Ibcast(&ans[2], 1, MPI_INT, root, parentcomm, &reqs[cnt++]);
        expected[2] = 199;
        MPI_Ibarrier(comms[0], &reqs[cnt++]);

        /* Do an Iscan on a neighbor comm */
        MPI_Comm_rank(nbrcomm, &rank);
        MPI_Comm_remote_size(nbrcomm, &rsize);
        MPI_Iallreduce(&rank, &ans[3], 1, MPI_INT, MPI_SUM, nbrcomm, &reqs[cnt++]);
        expected[3] = (rsize - 1) * rsize / 2;

        /* Pending operations include idup/iallreduce/ibcast on parentcomm
         * Iallreduce/Ibarrier on comms[0], and Iallreduce on nbrcomm */
        /* Waitall even if the first request is completed */
        MPI_Waitall(cnt, reqs, MPI_STATUSES_IGNORE);

        /* Check the answers */
        for (j = 0; j < 4; j++) {
            if (ans[j] != expected[j])
                errs[tid]++;
        }
        for (j = 0; j < NUM_IDUPS1 + NUM_IDUPS2; j++) {
            errs[tid] += MTestTestComm(comms[j]);
            MPI_Comm_free(&comms[j]);
        }
        if (verbose)
            printf("\t%d: Thread %d - comm_idup %d finish\n", rank, tid, i);
    }
    if (verbose)
        printf("%d: Thread %d - Done.\n", rank, tid);
    return (MTEST_THREAD_RETURN_TYPE) 0;
}


int main(int argc, char **argv)
{
    int thread_args[NUM_THREADS];
    MPI_Request requests[NUM_THREADS * 2];
    int i, provided;
    MPI_Comm newcomm;
    int toterrs = 0;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    check(provided == MPI_THREAD_MULTIPLE);
    check(NUM_IDUPS1 >= 1 && NUM_IDUPS2 >= 1);

    /* In each iteration, the process generates a new kind of intracommunicator, then
     * uses idup to duplicate the communicator for NUM_THREADS threads.
     */
    while (MTestGetIntracommGeneral(&newcomm, 1, 1)) {
        if (newcomm == MPI_COMM_NULL)
            continue;

        for (i = 0; i < NUM_THREADS; i++) {
            MPI_Comm_idup(newcomm, &parentcomms[i], &requests[2 * i]);
            MPI_Comm_idup(newcomm, &nbrcomms[i], &requests[2 * i + 1]);
        }
        MPI_Waitall(NUM_THREADS * 2, requests, MPI_STATUSES_IGNORE);

        for (i = 0; i < NUM_THREADS; i++) {
            thread_args[i] = i;
            MTest_Start_thread(test_intracomm, (void *) &thread_args[i]);
        }
        MTest_Join_threads();

        for (i = 0; i < NUM_THREADS; i++) {
            toterrs += errs[i];
            MPI_Comm_free(&parentcomms[i]);
            MPI_Comm_free(&nbrcomms[i]);
        }
        MTestFreeComm(&newcomm);
    }

    /* In each iteration, the process generates a new kind of intercommunicator, then
     * uses idup to duplicate the communicator for NUM_THREADS threads.
     */
    while (MTestGetIntercomm(&newcomm, &isLeft, 1)) {
        if (newcomm == MPI_COMM_NULL)
            continue;

        for (i = 0; i < NUM_THREADS; i++) {
            MPI_Comm_idup(newcomm, &parentcomms[i], &requests[2 * i]);
            MPI_Comm_idup(newcomm, &nbrcomms[i], &requests[2 * i + 1]);
        }
        MPI_Waitall(NUM_THREADS * 2, requests, MPI_STATUSES_IGNORE);

        for (i = 0; i < NUM_THREADS; i++) {
            thread_args[i] = i;
            MTest_Start_thread(test_intercomm, (void *) &thread_args[i]);
        }
        MTest_Join_threads();

        for (i = 0; i < NUM_THREADS; i++) {
            toterrs += errs[i];
            MPI_Comm_free(&parentcomms[i]);
            MPI_Comm_free(&nbrcomms[i]);
        }
        MTestFreeComm(&newcomm);
    }
    MTest_Finalize(toterrs);
    MPI_Finalize();
    return 0;
}
