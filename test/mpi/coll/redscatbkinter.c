/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * Test of reduce scatter block with large data on an intercommunicator
 * (needed in MPICH to trigger the long-data algorithm)
 *
 * Each processor contributes its rank + the index to the reduction,
 * then receives the ith sum
 *
 * Can be called with any number of processors.
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    int err = 0;
    int size, rsize, rank, i;
    int recvcount,              /* Each process receives this much data */
     sendcount,                 /* Each process contributes this much data */
     basecount;                 /* Unit of elements - basecount *rsize is recvcount,
                                 * etc. */
    int isLeftGroup;
    long long *sendbuf, *recvbuf;
    long long sumval;
    MPI_Comm comm;


    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    basecount = 1024;

    while (MTestGetIntercomm(&comm, &isLeftGroup, 2)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_remote_size(comm, &rsize);
        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);

        if (0) {
            printf("[%d] %s (%d,%d) remote %d\n", rank, isLeftGroup ? "L" : "R", rank, size, rsize);
        }

        recvcount = basecount * rsize;
        sendcount = basecount * rsize * size;

        sendbuf = (long long *) malloc(sendcount * sizeof(long long));
        if (!sendbuf) {
            fprintf(stderr, "Could not allocate %d ints for sendbuf\n", sendcount);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        for (i = 0; i < sendcount; i++) {
            sendbuf[i] = (long long) (rank * sendcount + i);
        }
        recvbuf = (long long *) malloc(recvcount * sizeof(long long));
        if (!recvbuf) {
            fprintf(stderr, "Could not allocate %d ints for recvbuf\n", recvcount);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        for (i = 0; i < recvcount; i++) {
            recvbuf[i] = (long long) (-i);
        }

        MPI_Reduce_scatter_block(sendbuf, recvbuf, recvcount, MPI_LONG_LONG, MPI_SUM, comm);

        /* Check received data */
        for (i = 0; i < recvcount; i++) {
            sumval = (long long) (sendcount) * (long long) ((rsize * (rsize - 1)) / 2) +
                (long long) (i + rank * rsize * basecount) * (long long) rsize;
            if (recvbuf[i] != sumval) {
                err++;
                if (err < 4) {
                    fprintf(stdout, "Did not get expected value for reduce scatter\n");
                    fprintf(stdout, "[%d] %s recvbuf[%d] = %lld, expected %lld\n",
                            rank, isLeftGroup ? "L" : "R", i, recvbuf[i], sumval);
                }
            }
        }

        free(sendbuf);
        free(recvbuf);

        MTestFreeComm(&comm);
    }

    MTest_Finalize(err);

    MPI_Finalize();

    return 0;
}
