/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

int main(int argc, char **argv)
{
    int errs = 0;
    MPI_Comm dup;
    MPI_Info info_in, info_out;
    int flag;
    char val[MPI_MAX_INFO_VAL] = { 0 };

    MTest_Init(&argc, &argv);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "mpi_assert_no_any_tag", "true");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info_in, &dup);

    MPI_Comm_get_info(dup, &info_out);
    MPI_Info_get(info_out, "mpi_assert_no_any_tag", MPI_MAX_INFO_VAL, val, &flag);
    if (!flag) {
        fprintf(stderr, "Hint was not set\n");
        errs++;
    } else if (strcmp("true", val)) {
        fprintf(stderr, "Expected value: true, received: %s\n", val);
        errs++;
    }
    MPI_Info_free(&info_out);

#if MPI_VERSION >= 4
    MPI_Comm dup2;
    MPI_Comm_dup(dup, &dup2);
    MPI_Comm_get_info(dup2, &info_out);
    MPI_Info_get(info_out, "mpi_assert_no_any_tag", MPI_MAX_INFO_VAL, val, &flag);
    if (flag && strcmp(val, "true") == 0) {
        fprintf(stderr,
                "Hint (mpi_assert_no_any_tag) was not expected to be copied from original communicator\n");
        errs++;
    }
    MPI_Comm_free(&dup2);
    MPI_Info_free(&info_out);
#endif

    MPI_Comm_free(&dup);
    MPI_Info_free(&info_in);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
