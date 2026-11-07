/*
 * Copyright (C) by Argonne National Laboratory
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run io_getextent
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Test file_get_extent";
*/

int run(const char *arg)
{
    int errs = 0;
    MPI_File fh;
    MPI_Comm comm;
    MPI_Aint extent, nextent, tmp_lb;


    comm = MPI_COMM_WORLD;
    MPI_File_open(comm, (char *) "test.ord",
                  MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, &fh);

    MPI_File_get_type_extent(fh, MPI_INT, &extent);
    MPI_Type_get_extent(MPI_INT, &tmp_lb, &nextent);

    if (nextent != extent) {
        errs++;
        fprintf(stderr, "Native extent not the same as the file extent\n");
    }
    MPI_File_close(&fh);

    return errs;
}
