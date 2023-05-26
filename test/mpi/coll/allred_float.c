/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

/* MPI_Allreduce need produce identical results on all ranks. This is
 * particular challenging for floating point datatypes since computer
 * floating point arithmetic do not follow associative law. This means
 * certain algorithms that works for integers need to be excluded for
 * floating point.
 *
 * This test checks when an inapproprate algorithms is used for floating
 * point reduction.
 */

/* single-precision float has roughly a precision of 7 decimal digits */
#define BIG 1e6
#define TINY 1e-2

#define N 8

float buf[N];

static void init_buf(int rank, int pos1, int pos2)
{
    /* Mix a pair of (BIG, -BIG) and TINY, the sum of array will be the sum of
     * all TINYs if we add (BIG, -BIG) first, but different results following
     * different associativity. A valid algorithm need to produce consistent
     * results on all ranks.
     */
    for (int i = 0; i < N; i++) {
        if (rank == pos1) {
            buf[i] = BIG;
        } else if (rank == pos2) {
            buf[i] = -BIG;
        } else {
            buf[i] = TINY;
        }
    }
}

int main(int argc, char **argv)
{
    int errs = 0;

    MTest_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 3) {
        printf("At least 3 processes required. More (e.g. 10) is recommended.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    for (int pos1 = 0; pos1 < size; pos1++) {
        for (int pos2 = pos1 + 1; pos2 < size; pos2++) {
            init_buf(rank, pos1, pos2);

            MPI_Allreduce(MPI_IN_PLACE, buf, N, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

            float *check_buf;
            if (rank == 0) {
                check_buf = malloc(N * size * sizeof(float));
            }
            MPI_Gather(buf, N, MPI_FLOAT, check_buf, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

            if (rank == 0) {
                MTestPrintfMsg(1, "BIG positions = (%d, %d), result = [", pos1, pos2);
                for (int j = 0; j < N; j++) {
                    MTestPrintfMsg(1, "%f ", buf[j]);
                }
                MTestPrintfMsg(1, "]\n");

                for (int i = 0; i < size; i++) {
                    for (int j = 0; j < N; j++) {
                        if (memcmp(&check_buf[i * N + j], &buf[j], sizeof(float)) != 0) {
                            if (errs < 10) {
                                printf("(%d - %d) Result [%d] from rank %d mismatch: %f != %f\n",
                                       pos1, pos2, j, i, check_buf[i * N + j], buf[j]);
                            }
                            errs++;
                        }
                    }
                }
                free(check_buf);
            }
        }
    }

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
