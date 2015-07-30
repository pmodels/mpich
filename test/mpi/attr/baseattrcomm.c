/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int main(int argc, char **argv)
{
    int errs = 0;
    void *v;
    int flag;
    int vval;
    int rank, size;

    MTest_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get TAG_UB\n");
    }
    else {
        vval = *(int *) v;
        if (vval < 32767) {
            errs++;
            fprintf(stderr, "Got too-small value (%d) for TAG_UB\n", vval);
        }
    }

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_HOST, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get HOST\n");
    }
    else {
        vval = *(int *) v;
        if ((vval < 0 || vval >= size) && vval != MPI_PROC_NULL) {
            errs++;
            fprintf(stderr, "Got invalid value %d for HOST\n", vval);
        }
    }
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_IO, &v, &flag);
    if (!flag) {
        errs++;
        fprintf(stderr, "Could not get IO\n");
    }
    else {
        vval = *(int *) v;
        if ((vval < 0 || vval >= size) && vval != MPI_ANY_SOURCE && vval != MPI_PROC_NULL) {
            errs++;
            fprintf(stderr, "Got invalid value %d for IO\n", vval);
        }
    }

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL, &v, &flag);
    if (flag) {
        /* Wtime need not be set */
        vval = *(int *) v;
        if (vval < 0 || vval > 1) {
            errs++;
            fprintf(stderr, "Invalid value for WTIME_IS_GLOBAL (got %d)\n", vval);
        }
    }

    /* MPI 2.0, section 5.5.3 - MPI_APPNUM should be set if the program is
     * started with more than one executable name (e.g., in MPMD instead
     * of SPMD mode).  This is independent of the dynamic process routines,
     * and should be supported even if MPI_COMM_SPAWN and friends are not. */
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_APPNUM, &v, &flag);
    /* appnum need not be set */
    if (flag) {
        vval = *(int *) v;
        if (vval < 0) {
            errs++;
            fprintf(stderr, "MPI_APPNUM is defined as %d but must be nonnegative\n", vval);
        }
    }

    /* MPI 2.0 section 5.5.1.  MPI_UNIVERSE_SIZE need not be set, but
     * should be present.  */
    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, &v, &flag);
    /* MPI_UNIVERSE_SIZE need not be set */
    if (flag) {
        /* But if it is set, it must be at least the size of comm_world */
        vval = *(int *) v;
        if (vval < size) {
            errs++;
            fprintf(stderr, "MPI_UNIVERSE_SIZE = %d, less than comm world (%d)\n", vval, size);
        }
    }

    MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_LASTUSEDCODE, &v, &flag);
    /* Last used code must be defined and >= MPI_ERR_LASTCODE */
    if (flag) {
        vval = *(int *) v;
        if (vval < MPI_ERR_LASTCODE) {
            errs++;
            fprintf(stderr,
                    "MPI_LASTUSEDCODE points to an integer (%d) smaller than MPI_ERR_LASTCODE (%d)\n",
                    vval, MPI_ERR_LASTCODE);
        }
    }
    else {
        errs++;
        fprintf(stderr, "MPI_LASTUSECODE is not defined\n");
    }

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
