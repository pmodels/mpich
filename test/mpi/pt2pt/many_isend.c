/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run pt2pt_many_isend
int run(const char *arg);
#endif

#define ITER 5
#define BUF_COUNT (16*1024/sizeof(int))

static int rank, nprocs;
static int recvbuf[BUF_COUNT], sendbuf[BUF_COUNT];

int run(const char *arg)
{
    int x, i, j, errs = 0;
    MPI_Status *sendstats = NULL;
    MPI_Request *sendreqs = NULL;
    MPI_Status recvstatus;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    sendreqs = (MPI_Request *) malloc(nprocs * sizeof(MPI_Request));
    sendstats = (MPI_Status *) malloc(nprocs * sizeof(MPI_Status));

    for (x = 0; x < ITER; x++) {
        MPI_Barrier(MPI_COMM_WORLD);

        for (i = 0; i < BUF_COUNT; i++) {
            sendbuf[i] = rank;
            recvbuf[i] = -1;
        }

        /* all to all send */
        for (i = 0; i < nprocs; i++) {
            MPI_Isend(sendbuf, BUF_COUNT, MPI_INT, i, 0, MPI_COMM_WORLD, &sendreqs[i]);
        }

        /* receive one by one */
        for (i = 0; i < nprocs; i++) {
            MPI_Recv(recvbuf, BUF_COUNT, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD,
                     &recvstatus);

            for (j = 0; j < BUF_COUNT; j++) {
                if (recvbuf[j] != recvstatus.MPI_SOURCE) {
                    if (++errs < 10) {
                        fprintf(stderr, "recvbuf[%d] (%d) != %d\n", j, recvbuf[j],
                                recvstatus.MPI_SOURCE);
                    }
                }
            }
        }

        /* ensure all send request completed */
        MPI_Waitall(nprocs, sendreqs, sendstats);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    free(sendreqs);
    free(sendstats);

    return errs;
}
