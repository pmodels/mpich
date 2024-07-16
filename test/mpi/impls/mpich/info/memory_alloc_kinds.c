/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int check_value(const char *value, const char *expected);

int main(int argc, char *argv[])
{
    char value[MPI_MAX_INFO_VAL];
    const char *key = "mpi_memory_alloc_kinds";
    int flag;
    int errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Info_get(MPI_INFO_ENV, key, MPI_MAX_INFO_VAL, value, &flag);
    if (flag) {
        /* It was discussed in the October 2023 MPI Forum meeting that
         * if a value for mpi_memory_alloc_kinds is returned from
         * MPI_INFO_ENV, it should be the requested value. This is
         * consistent with the other keys defined in MPI_INFO_ENV. Like
         * those keys, returning a value for mpi_memory_alloc_kind is
         * optional, and MPICH does not support it at this time. */
        errs++;
        printf("Key mpi_memory_alloc_kinds was not found in MPI_INFO_ENV.\n");
    }

    /* test if MPI_COMM_WORLD gets the right value */
    MPI_Info cinfo;
    MPI_Comm_get_info(MPI_COMM_WORLD, &cinfo);
    MPI_Info_get(cinfo, key, MPI_MAX_INFO_VAL, value, &flag);
    MPI_Info_free(&cinfo);
    if (flag) {
        errs += check_value(value, "mpi,system,mpi:alloc_mem");
    } else {
        errs++;
        printf("Key mpi_memory_alloc_kinds was not found in info from MPI_COMM_WORLD.\n");
    }

    /* test if session gets the right value */
    MPI_Info sinfo;
    MPI_Session session;
    MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_ARE_FATAL, &session);
    MPI_Session_get_info(session, &sinfo);
    MPI_Info_get(sinfo, key, MPI_MAX_INFO_VAL, value, &flag);
    MPI_Info_free(&sinfo);
    if (flag) {
        errs += check_value(value, "mpi,system,mpi:alloc_mem");
    } else {
        errs++;
        printf("Key mpi_memory_alloc_kinds was not found in info from MPI session.\n");
    }
    MPI_Session_finalize(&session);

    /* test if session info overrides the default */
    MPI_Info info_in;
    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, key, "mpi:win_allocate:alloc_mem");
    MPI_Session_init(info_in, MPI_ERRORS_ARE_FATAL, &session);
    MPI_Session_get_info(session, &sinfo);
    MPI_Info_get(sinfo, key, MPI_MAX_INFO_VAL, value, &flag);
    if (flag) {
        errs += check_value(value, "mpi,system,mpi:win_allocate:alloc_mem");
    } else {
        errs++;
        printf("Key mpi_memory_alloc_kinds was not found in info from MPI_Session_init with info.\n");
    }
    MPI_Info_free(&info_in);
    MPI_Info_free(&sinfo);
    MPI_Session_finalize(&session);

    MTest_Finalize(errs);
    return 0;
}

static int check_value(const char *value, const char *expected)
{
    if (strcmp(value, expected)) {
        printf("mpi_memory_alloc_kinds value is \"%s\", expected \"%s\"\n", value, expected);
        return 1;
    }

    return 0;
}
