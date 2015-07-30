/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * This program checks that MPI_Iprobe and MPI_Probe correctly handle
 * a source of MPI_PROC_NULL
 */

int main(int argc, char **argv)
{
    int flag;
    int errs = 0;
    MPI_Status status;

    MTest_Init(&argc, &argv);

    MPI_Iprobe(MPI_PROC_NULL, 10, MPI_COMM_WORLD, &flag, &status);
    if (!flag) {
        errs++;
        printf("Iprobe of source=MPI_PROC_NULL returned flag=false\n");
    }
    else {
        if (status.MPI_SOURCE != MPI_PROC_NULL) {
            printf("Status.MPI_SOURCE was %d, should be MPI_PROC_NULL\n", status.MPI_SOURCE);
            errs++;
        }
        if (status.MPI_TAG != MPI_ANY_TAG) {
            printf("Status.MPI_TAG was %d, should be MPI_ANY_TAGL\n", status.MPI_TAG);
            errs++;
        }
    }
    /* If Iprobe failed, probe is likely to as well.  Avoid a possible hang
     * by testing Probe only if Iprobe test passed */
    if (errs == 0) {
        MPI_Probe(MPI_PROC_NULL, 10, MPI_COMM_WORLD, &status);
        if (status.MPI_SOURCE != MPI_PROC_NULL) {
            printf("Status.MPI_SOURCE was %d, should be MPI_PROC_NULL\n", status.MPI_SOURCE);
            errs++;
        }
        if (status.MPI_TAG != MPI_ANY_TAG) {
            printf("Status.MPI_TAG was %d, should be MPI_ANY_TAGL\n", status.MPI_TAG);
            errs++;
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
