/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test count for zero length reads";
*/

int main(int argc, char **argv)
{
    int errs = 0;
    int len, rank, get_size;
    char buf[10];
    const char *filename = argv[0];
    MPI_File fh;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);

    if (rank == 0)
        len = 10;
    else
        len = 0;

    /* mimic the case status object may not be initialized to 0 */
    memset(&status, 1, sizeof(status));

    MPI_File_read_all(fh, buf, len, MPI_BYTE, &status);
    MPI_Get_count(&status, MPI_BYTE, &get_size);

    if (len != get_size) {
        printf("Error: expecting get_size to be %d but got %d\n", len, get_size);
        errs++;
    }

    MPI_File_close(&fh);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
