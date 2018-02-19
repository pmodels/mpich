/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* test case contributed by Lisandro Dalcin <dalcinl@gmail.com> */
#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    int errs;
    MPI_File fh;
    MPI_Init(&argc, &argv);
    MTest_Init(&argc, &argv);

    MPI_File_open(MPI_COMM_WORLD, "/tmp/datafile",
                  MPI_MODE_CREATE | MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);

    MPI_File_set_errhandler(fh, MPI_ERRORS_RETURN);
    errs = MPI_File_call_errhandler(fh, MPI_ERR_FILE);
    if (errs != MPI_SUCCESS) {
        fprintf(stderr, "errs: %d, expected: %d\n", errs, (int) MPI_SUCCESS);
    } else {
        printf(" No Errors\n");
    }
    MPI_File_close(&fh);

    MPI_Finalize();
    return 0;
}
