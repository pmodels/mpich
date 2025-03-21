/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define N 4
MPI_Comm comm = MPI_COMM_WORLD;

int main(int argc, char *argv[])
{
    int errs = 0;
    MTest_Init(&argc, &argv);

    /* Test MPIX_BFLOAT16 */
    unsigned short buf[N];
    for (int i = 0; i < N; i++) {
        buf[i] = 0x3f80;        /* 1.0 in bfloat16 */
    }
    MPI_Allreduce(MPI_IN_PLACE, buf, N, MPIX_BFLOAT16, MPI_SUM, comm);

    MTestPrintfMsg(1, "Allreduce %d MPIX_BFLOAT16 (val=0.1), result: 0x%x, 0x%x, ...\n",
                   N, buf[0], buf[1]);

    /* Test MPIX_C_FLOAT16 */
#ifdef HAVE_FLOAT16
    _Float16 buf_f16[N];
    for (int i = 0; i < N; i++) {
        buf_f16[i] = 1.0;
    }
    MPI_Allreduce(MPI_IN_PLACE, buf_f16, N, MPIX_C_FLOAT16, MPI_SUM, comm);

    MTestPrintfMsg(1, "Allreduce %d MPIX_C_FLOAT16 (val=0.1), result: %f, %f, ...\n",
                   N, (float) buf_f16[0], (float) buf_f16[1]);
#endif

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
