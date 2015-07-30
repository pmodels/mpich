/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

#define ROOT 7
#if 0
/* Following should always work for -n 8  256, -N 32, using longs */
#define COUNT 1048576*32
#endif
#if 1
/* Following will fail for -n 8 unless gather path is 64 bit clean */
#define COUNT (1024*1024*128+1)
#endif
#define VERIFY_CONST 100000000L

int main(int argc, char *argv[])
{
    int rank, size;
    int i, j;
    long *sendbuf = NULL;
    long *recvbuf = NULL;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    sendbuf = malloc(COUNT * sizeof(long));
    if (sendbuf == NULL) {
        fprintf(stderr, "PE %d:ERROR: malloc of sendbuf failed\n", rank);
    }
    for (i = 0; i < COUNT; i++) {
        sendbuf[i] = (long) i + (long) rank *VERIFY_CONST;
    }

    if (rank == ROOT) {
        recvbuf = malloc(COUNT * sizeof(long) * size);
        if (recvbuf == NULL) {
            fprintf(stderr, "PE %d:ERROR: malloc of recvbuf failed\n", rank);
        }
        for (i = 0; i < COUNT * size; i++) {
            recvbuf[i] = -456789L;
        }
    }

    MPI_Gather(sendbuf, COUNT, MPI_LONG, recvbuf, COUNT, MPI_LONG, ROOT, MPI_COMM_WORLD);

    int lerr = 0;
    if (rank == ROOT) {
        for (i = 0; i < size; i++) {
            for (j = 0; j < COUNT; j++) {
                if (recvbuf[i * COUNT + j] != i * VERIFY_CONST + j) {
                    printf("PE 0: mis-match error");
                    printf("  recbuf[%d * %d + %d] = ", i, COUNT, j);
                    printf("  %ld,", recvbuf[i * COUNT + j]);
                    printf("  should be %ld\n", i * VERIFY_CONST + j);
                    lerr++;
                    if (lerr > 10) {
                        j = COUNT;
                    }
                }
            }
        }
        MTest_Finalize(lerr);
        free(recvbuf);
    }
    else {
        MTest_Finalize(lerr);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    free(sendbuf);
    return 0;
}
