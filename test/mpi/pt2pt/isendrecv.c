/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

/* Similar test as isendirecv.c, but use MPI_Isendrecv */

int errs = 0;
int elems = 20;
int rank, nproc, dest;
MPI_Comm comm;
float *in_buf, *out_buf;
MPI_Request *reqs;

/* sendrecv may use the same or different source and destination,
 * offset defines the offset between them */
static void test_sendrecv(int offset)
{
    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            in_buf[i * elems + j] = -1.0;
            out_buf[i * elems + j] = rank * 10000.0 + i * 100.0 + j;
        }
    }

    for (int i = 0; i < nproc; i++) {
        int j = (i + offset) % nproc;
        MPI_Isendrecv(&out_buf[elems * i], elems, MPI_FLOAT, i, 0,
                      &in_buf[elems * j], elems, MPI_FLOAT, j, 0, comm, &reqs[i]);
    }

    MPI_Waitall(nproc, reqs, MPI_STATUSES_IGNORE);

    for (int i = 0; i < nproc; i++) {
        for (int j = 0; j < elems; j++) {
            if (in_buf[i * elems + j] != i * 10000.0 + rank * 100.0 + j) {
                errs++;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = malloc(nproc * sizeof(MPI_Request));
    in_buf = malloc(elems * nproc * sizeof(float));
    out_buf = malloc(elems * nproc * sizeof(float));

    for (int offset = 0; offset < nproc - 1; offset++) {
        test_sendrecv(offset);
    }

    free(reqs);
    free(in_buf);
    free(out_buf);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
