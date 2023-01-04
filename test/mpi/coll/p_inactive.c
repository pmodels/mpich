/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* test/wait on an inactive persistent collective operation. */

#include "mpi.h"
#include "mpitest.h"

static void init_status(MPI_Status * status);
static int check_empty_status(MPI_Status status);

int main(int argc, char **argv)
{
    MPI_Status status;
    MPI_Request req;
    int flag;
    int errs = 0;
    int a;

    MTest_Init(&argc, &argv);

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

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
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
