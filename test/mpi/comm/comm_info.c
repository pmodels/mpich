/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

#define VERBOSE 0

int main(int argc, char **argv)
{
    int i, j, rank;
    MPI_Info info_in, info_out;
    int errors = 0, all_errors = 0;
    MPI_Comm comm;
    void *base;
    char invalid_key[] = "invalid_test_key";
    char buf[MPI_MAX_INFO_VAL];
    int flag;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, invalid_key, "true");

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    MPI_Comm_set_info(comm, info_in);
    MPI_Comm_get_info(comm, &info_out);

    MPI_Info_get(info_out, invalid_key, MPI_MAX_INFO_VAL, buf, &flag);
#ifndef USE_STRICT_MPI
    /* Check if our invalid key was ignored.  Note, this check's MPICH's
     * behavior, but this behavior may not be required for a standard
     * conforming MPI implementation. */
    if (flag) {
        printf("%d: %s was not ignored\n", rank, invalid_key);
        errors++;
    }
#endif

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);
    MPI_Comm_free(&comm);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
