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
    int errors = 0, errs = 0;
    MPI_Comm comm;
    void *base;
    char key[] = "test_key";
    char buf[MPI_MAX_INFO_VAL];
    int flag;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, key, "true");

    MPI_Comm_dup(MPI_COMM_WORLD, &comm);

    MPI_Comm_set_info(comm, info_in);
    MPI_Comm_get_info(comm, &info_out);

    MPI_Info_get(info_out, key, MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        printf("%d: %s was ignored\n", rank, key);
        errors++;
    }

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);
    MPI_Comm_free(&comm);

    MPI_Reduce(&errors, &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
