/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int verbose = 0;
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
        /* MPICH will return the default kinds plus any user requests
         * that are also supported. As of MPI-4.1, that only includes the
         * mpi kind with restrictors. */
        errs += check_value(value, "mpi,system,mpi:alloc_mem");
    } else {
        /* MPICH supports this key since MPICH 4.2.0 */
        errs++;
    }

    /* test if session gets the same default */
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
        MTestPrintfMsg(verbose, "mpi_memory_alloc_kinds value is \"%s\", expected \"%s\"\n",
                       value, expected);
        return 1;
    }

    return 0;
}
