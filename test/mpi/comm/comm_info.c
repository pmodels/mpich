/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run comm_comm_info
int run(const char *arg);
#endif

#define VERBOSE 0

int run(const char *arg)
{
    int rank;
    MPI_Info info_in, info_out;
    int errors = 0, errs = 0;
    MPI_Comm comm;
    char invalid_key[] = "invalid_test_key";
    char buf[MPI_MAX_INFO_VAL];
    int flag;

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

    MPI_Reduce(&errors, &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    return errs;
}
