/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test file_get_extent";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Aint extent, nextent;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_File_open(comm, (char *) "test.ord",
                  MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);

    MPI_File_get_type_extent(fh, MPI_INT, &extent);
    MPI_Type_extent(MPI_INT, &nextent);

    if (nextent != extent) {
        errs++;
        fprintf(stderr, "Native extent not the same as the file extent\n");
    }
    MPI_File_close(&fh);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
