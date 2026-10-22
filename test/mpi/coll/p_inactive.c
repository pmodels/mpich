/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* test/wait on an inactive persistent collective operation. */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_p_inactive
int run(const char *arg);
#endif

static void init_status(MPI_Status * status);
static int check_empty_status(MPI_Status status);

int run(const char *arg)
{
    MPI_Status status;
    MPI_Request req;
    int flag;
    int errs = 0;
    int a;

    MPI_Bcast_init(&a, 1, MPI_INT, 0, MPI_COMM_WORLD, MPI_INFO_NULL, &req);

    /* test should return flag=true and empty status */
    init_status(&status);
    MPI_Test(&req, &flag, &status);
    if (!flag) {
        ++errs;
    }
    errs += check_empty_status(status);

    /* wait should return empty status */
    init_status(&status);
    MPI_Wait(&req, &status);
    errs += check_empty_status(status);

    MPI_Request_free(&req);

    return errs;
}

/* static routines to init and verify status object */

static void init_status(MPI_Status * status)
{
    status->MPI_TAG = 42;
    status->MPI_SOURCE = 42;
    MPI_Status_set_elements(status, MPI_INT, 42);
    MPI_Status_set_cancelled(status, 1);
}

static int check_empty_status(MPI_Status status)
{
    int errs = 0;

    if (status.MPI_TAG != MPI_ANY_TAG) {
        ++errs;
    }

    if (status.MPI_SOURCE != MPI_ANY_SOURCE) {
        ++errs;
    }

    int count;
    MPI_Get_elements(&status, MPI_INT, &count);
    if (count != 0) {
        ++errs;
    }

    int flag;
    MPI_Test_cancelled(&status, &flag);
    if (flag != 0) {
        ++errs;
    }

    return errs;
}
