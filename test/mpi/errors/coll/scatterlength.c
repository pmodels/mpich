/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpicolltest.h"

/* Very simple test that MPI_Scatter handled mismatched lengths.
   Extended from bcastlength.c
*/

int verbose = 0;

int main(int argc, char *argv[])
{
    int buf[10];
    int *sendbuf;
    int ierr, errs = 0;
    int rank, num_ranks;
    char str[MPI_MAX_ERROR_STRING + 1];
    int slen;

    MTEST_VG_MEM_INIT(buf, 10 * sizeof(int));

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
    sendbuf = (int *) (malloc(sizeof(int) * 10 * num_ranks));

    if (rank == 0) {
        ierr = MTest_Scatter(sendbuf, 1, MPI_INT, buf, 10, MPI_INT, 0, MPI_COMM_WORLD);
    } else {
        ierr = MTest_Scatter(sendbuf, 10, MPI_INT, buf, 10, MPI_INT, 0, MPI_COMM_WORLD);
    }
    if (ierr == MPI_SUCCESS) {
        if (rank != 0) {
            /* The root process may not detect that a too-short buffer
             * was provided by the non-root processes, but those processes
             * should detect this. */
            errs++;
            printf("Did not detect mismatched length (short) on process %d\n", rank);
        }
    } else {
        if (verbose) {
            MPI_Error_string(ierr, str, &slen);
            printf("Found expected error; message is: %s\n", str);
        }
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    free(sendbuf);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
