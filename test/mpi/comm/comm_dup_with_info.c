/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include "mpitest.h"

#define VERBOSE 0

int main(int argc, char **argv)
{
    MPI_Comm world_copy, world_copy_with_info;
    MPI_Info info_in, info_out, info_out2;
    int errors = 0, errs = 0;

    char test_key[] = "dup_test_key";
    char buf[MPI_MAX_INFO_VAL];
    char buf2[MPI_MAX_INFO_VAL];
    int flag;

    MTest_Init(&argc, &argv);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, test_key, "original");

    MPI_Comm_set_info(MPI_COMM_WORLD, info_in);
    MPI_Comm_dup(MPI_COMM_WORLD, &world_copy);

    MPI_Info_set(info_in, test_key, "copy");
    MPI_Comm_dup_with_info(MPI_COMM_WORLD, info_in, &world_copy_with_info);

    MPI_Comm_get_info(world_copy, &info_out);
    MPI_Comm_get_info(world_copy_with_info, &info_out2);

    MPI_Info_get(info_out, test_key, MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        printf("%s was lost during MPI_Comm_dup\n", test_key);
        errors++;
    }

    MPI_Info_get(info_out2, test_key, MPI_MAX_INFO_VAL, buf2, &flag);
    if (!flag) {
        printf("%s was lost during MPI_Comm_dup_with_info\n", test_key);
        errors++;
    }

    if (strcmp(buf, "original") != 0) {
        printf("MPI_Comm_dup: [%s] got %d expected 'original'\n", test_key, buf);
        errors++;
    }

    if (strcmp(buf2, "copy") != 0) {
        printf("MPI_Comm_dup_with_info: [%s] got %d expected 'copy'\n", test_key, buf2);
        errors++;
    }

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);
    MPI_Info_free(&info_out2);
    MPI_Comm_free(&world_copy);
    MPI_Comm_free(&world_copy_with_info);

    MPI_Reduce(&errors, &errs, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    MTest_Finalize(errs);

    return MTestReturnValue(errs);
}
