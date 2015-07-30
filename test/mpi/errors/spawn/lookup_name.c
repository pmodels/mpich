/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/* calling lookup_name 15 times in a loop when the name has not been
   been published used to cause MPICH to crash. Adding this as a
   regression test */

int main(int argc, char *argv[])
{
    int errs = 0, i;
    char port_name[MPI_MAX_PORT_NAME], serv_name[256];

    MTest_Init(&argc, &argv);

    strcpy(serv_name, "MyTest");

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    for (i = 0; i < 20; i++)
        MPI_Lookup_name(serv_name, MPI_INFO_NULL, port_name);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
