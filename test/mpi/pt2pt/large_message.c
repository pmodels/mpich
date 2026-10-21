/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_large_message
int run(const char *arg);
#endif

/* tests send/recv of a message > 2GB. count=270M, type=long long
   run with 3 processes to exercise both shared memory and TCP in Nemesis tests*/

int run(const char *arg)
{
    int i, size, rank;
    int cnt = 270000000;
    int stat_cnt = 0;
    MPI_Status status;
    long long *cols;
    int errs = 0;

/* need large memory */
    if (sizeof(void *) < 8) {
        return errs;
    }

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (size != 3) {
        if (rank == 0) {
            fprintf(stderr, "This test require 3 processes.\n");
        }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    cols = malloc(cnt * sizeof(long long));
    if (cols == NULL) {
        printf("malloc of >2GB array failed\n");
        errs++;
        return errs;
    }

    if (rank == 0) {
        for (i = 0; i < cnt; i++)
            cols[i] = i;
        /* printf("[%d] sending...\n",rank); */
        MPI_Send(cols, cnt, MPI_LONG_LONG_INT, 1, 0, MPI_COMM_WORLD);
        MPI_Send(cols, cnt, MPI_LONG_LONG_INT, 2, 0, MPI_COMM_WORLD);
    } else {
        /* printf("[%d] receiving...\n",rank); */
        for (i = 0; i < cnt; i++)
            cols[i] = -1;
        MPI_Recv(cols, cnt, MPI_LONG_LONG_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_LONG_LONG_INT, &stat_cnt);
        if (cnt != stat_cnt) {
            fprintf(stderr, "Output of MPI_Get_count (%d) does not match expected count (%d).\n",
                    stat_cnt, cnt);
            errs++;
        }
        for (i = 0; i < cnt; i++) {
            if (cols[i] != i) {
                /*printf("Rank %d, cols[i]=%lld, should be %d\n", rank, cols[i], i); */
                errs++;
            }
        }
    }
    free(cols);

    return errs;
}
