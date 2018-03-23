/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

int add(double *, double *, int *, MPI_Datatype *);
/*
 * User-defined operation on a long value (tests proper handling of
 * possible pipelining in the implementation of reductions with user-defined
 * operations).
 */
int add(double *invec, double *inoutvec, int *len, MPI_Datatype * dtype)
{
    int i, n = *len;
    for (i = 0; i < n; i++) {
        inoutvec[i] = invec[i] + inoutvec[i];
    }
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Op op;
    int i, rank, size, bufsize, errcnt = 0;
    double *inbuf, *outbuf, value;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Op_create((MPI_User_function *) add, 1, &op);

    bufsize = 1;
    while (bufsize < 100000) {
        inbuf = (double *) malloc(bufsize * sizeof(double));
        outbuf = (double *) malloc(bufsize * sizeof(double));
        if (!inbuf || !outbuf) {
            fprintf(stderr, "Could not allocate buffers for size %d\n", bufsize);
            errcnt++;
            break;
        }

        value = (rank & 0x1) ? 1.0 : -1.0;
        for (i = 0; i < bufsize; i++) {
            inbuf[i] = value;
            outbuf[i] = 100.0;
        }
        MPI_Allreduce(inbuf, outbuf, bufsize, MPI_DOUBLE, op, MPI_COMM_WORLD);
        /* Check values */
        value = (size & 0x1) ? -1.0 : 0.0;
        for (i = 0; i < bufsize; i++) {
            if (outbuf[i] != value) {
                if (errcnt < 10)
                    printf("outbuf[%d] = %f, should = %f\n", i, outbuf[i], value);
                errcnt++;
            }
        }
        free(inbuf);
        free(outbuf);
        bufsize *= 2;
    }

    MPI_Op_free(&op);
    MTest_Finalize(errcnt);
    return MTestReturnValue(errcnt);
}
