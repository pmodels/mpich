/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include "mpitest.h"

int main(int argc, char *argv[])
{
    int errs = 0;
    int elems = 20;
    int rank, nproc, dest, i;
    float *in_buf, *out_buf;
    MPI_Comm comm;
    MPI_Request *reqs;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &nproc);

    reqs = (MPI_Request *) malloc(2 * nproc * sizeof(MPI_Request));
    in_buf = (float *) malloc(elems * nproc * sizeof(float));
    out_buf = (float *) malloc(elems * nproc * sizeof(float));
    MTEST_VG_MEM_INIT(out_buf, elems * nproc * sizeof(float));

    for (i = 0; i < nproc; i++) {
        MPI_Irecv(&in_buf[elems * i], elems, MPI_FLOAT, i, 0, comm, &reqs[i]);
    }

    for (i = 0; i < nproc; i++) {
        MPI_Isend(&out_buf[elems * i], elems, MPI_FLOAT, i, 0, comm, &reqs[i + nproc]);
    }

    MPI_Waitall(nproc * 2, reqs, MPI_STATUSES_IGNORE);

    free(reqs);
    free(in_buf);
    free(out_buf);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
