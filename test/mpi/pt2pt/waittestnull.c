/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2005 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/*
 * This program checks that the various MPI_Test and MPI_Wait routines
 * allow both null requests and in the multiple completion cases, empty
 * lists of requests.
 */

int main(int argc, char **argv)
{
    int errs = 0;
    MPI_Status status, *status_array = 0;
    int count = 0, flag, idx, rc, errlen, *indices = 0, outcnt;
    MPI_Request *reqs = 0;
    char errmsg[MPI_MAX_ERROR_STRING];

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    rc = MPI_Testall(count, reqs, &flag, status_array);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Testall returned failure: %s\n", errmsg);
        errs++;
    }
    else if (!flag) {
        printf("MPI_Testall(0, ...) did not return a true flag\n");
        errs++;
    }

    rc = MPI_Waitall(count, reqs, status_array);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Waitall returned failure: %s\n", errmsg);
        errs++;
    }

    rc = MPI_Testany(count, reqs, &idx, &flag, &status);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Testany returned failure: %s\n", errmsg);
        errs++;
    }
    else if (!flag) {
        printf("MPI_Testany(0, ...) did not return a true flag\n");
        errs++;
    }

    rc = MPI_Waitany(count, reqs, &idx, &status);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Waitany returned failure: %s\n", errmsg);
        errs++;
    }

    rc = MPI_Testsome(count, reqs, &outcnt, indices, status_array);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Testsome returned failure: %s\n", errmsg);
        errs++;
    }

    rc = MPI_Waitsome(count, reqs, &outcnt, indices, status_array);
    if (rc != MPI_SUCCESS) {
        MPI_Error_string(rc, errmsg, &errlen);
        printf("MPI_Waitsome returned failure: %s\n", errmsg);
        errs++;
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
