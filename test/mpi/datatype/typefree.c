/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2007 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This test may be used to confirm that memory is properly recovered from
 * freed datatypes.  To test this, build the MPI implementation with memory
 * leak checking.  As this program may be run with a single process, it should
 * also be easy to run it under valgrind or a similar program.  With MPICH,
 * you can configure with the option
 *
 *   --enable-g=mem
 *
 * to turn on MPICH's internal memory checking.
 */

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Datatype type;

    MTest_Init(&argc, &argv);
    MPI_Type_dup(MPI_INT, &type);
    MPI_Type_free(&type);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
