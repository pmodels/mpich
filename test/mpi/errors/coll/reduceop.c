/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"
#include "mpicolltest.h"

/* Very simple test that Reduce handled mismatched operator
   Extended from bcastlength.c
*/

int verbose = 0;

int main(int argc, char *argv[])
{
    const int size = 10;
    int buf[size], recvbuf[size], ierr, errs = 0;
    int rank;
    char str[MPI_MAX_ERROR_STRING + 1];
    int slen;
    int i = 0;

    MTEST_VG_MEM_INIT(buf, size * sizeof(int));

    MTest_Init(&argc, &argv);

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for (i = 0; i < size; ++i) {
        buf[i] = recvbuf[i] = rank + 1;
    }

    if (rank == 0) {
        ierr = MTest_Reduce(buf, recvbuf, size, MPI_INT, MPI_PROD, 0, MPI_COMM_WORLD);
    } else {
        ierr = MTest_Reduce(buf, recvbuf, size, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    }
    if (ierr == MPI_SUCCESS) {
        if (rank != 0) {
            /* The root process may not detect that a mismatched operator
             * was provided by the non-root processes, but the root
             * should detect this. */
            errs++;
            printf("Did not detect mismatched operator on process %d\n", rank);
        }
    } else {
        if (verbose) {
            MPI_Error_string(ierr, str, &slen);
            printf("Found expected error; message is: %s\n", str);
        }
    }
    if (verbose && rank == 0) {
        printf("Result : \n");
        for (i = 0; i < size; i++) {
            printf("%d, ", recvbuf[i]);
        }
        printf("\n");
    }

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
