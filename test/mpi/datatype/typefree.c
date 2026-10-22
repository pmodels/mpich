/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run datatype_typefree
int run(const char *arg);
#endif

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

int run(const char *arg)
{
    int errs = 0;
    MPI_Datatype type;

    MPI_Type_dup(MPI_INT, &type);
    MPI_Type_free(&type);
    return errs;
}
