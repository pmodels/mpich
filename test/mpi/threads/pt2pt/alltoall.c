/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include "mpitest.h"
#include "mpithreadtest.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>

#ifdef DO_DEBUG
#define DEBUG(_a){ _a ;fflush(stdout);}
#else
#define DEBUG(_a)
#endif

#define NUM_ITER 10000

const int REQ_TAG = 111;
const int ANS_TAG = 222;

/* MPI environment description */
int rank, size, provided;

MTEST_THREAD_RETURN_TYPE listener(void *);

/*
   LISTENER THREAD
   it waits for communication from any source (including calling thread)
   messages which it receives have tag REQ_TAG
   thread runs infinite loop which will stop only if every node in the
   MPI_COMM_WORLD send request containing -1
*/
MTEST_THREAD_RETURN_TYPE listener(void *extra)
{
    int req;
    int source;
    MPI_Status stat;

    int no_fins = 0;            /* this must be equal to size to break loop below */

    while (1) {
        /* wait for request */
        MPI_Recv(&req, 1, MPI_INT, MPI_ANY_SOURCE, REQ_TAG, MPI_COMM_WORLD, &stat);

        /* get request source */
        source = stat.MPI_SOURCE;

        DEBUG(printf("node %d got request %d from %d\n", rank, req, source));

        if (req == -1)
            ++no_fins;  /* one more node finished requesting */

        /* no more requests can arrive */
        if (no_fins == size)
            break;

    }

    DEBUG(printf("node %d has stopped listener\n", rank));
    return MTEST_THREAD_RETVAL_IGN;
}


int main(int argc, char *argv[])
{
    int buf = 0;
    long int i, j;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    if (provided != MPI_THREAD_MULTIPLE) {
        printf("This test requires MPI_THREAD_MULTIPLE\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for (i = 0; i < NUM_ITER; i++) {
        /* create listener thread */
        MTest_Start_thread(listener, NULL);

        /* no more requests to send
         * inform other in the group that we have finished */
        buf = -1;
        for (j = 0; j < size; ++j) {
            MPI_Send(&buf, 1, MPI_INT, j, REQ_TAG, MPI_COMM_WORLD);
        }

        /* and wait for others to do the same */
        MTest_Join_threads();

        /* barrier to avoid deadlock */
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();

    /* This program works if it gets here */
    if (rank == 0) {
        printf(" No Errors\n");
    }

    return 0;
}
