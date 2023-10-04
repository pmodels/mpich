/*
 * ParaStation
 *
 * Copyright (C) 2023 ParTec AG, Munich
 *
 * This file may be distributed under the terms of the Q Public License
 * as defined in the file LICENSE.QPL included in the packaging of this
 * file.
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int errs = 0;
int calls = 0;

void eh(MPI_Comm * comm, int *err, ...)
{
    calls++;
    return;
}

/* Test for the errhandler arg of MPI_Comm_create_from_group */

int main(int argc, char *argv[])
{
    int rc;
    char *pset_name = "mpi://WORLD";

    MPI_Session shandle = MPI_SESSION_NULL;
    MPI_Group ghandle = MPI_GROUP_NULL;
    MPI_Comm comm = MPI_COMM_NULL;
    MPI_Errhandler errhandler = MPI_ERRHANDLER_NULL;

    rc = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned error code: %i\n", rc);
        goto fn_exit;
    }

    MPI_Comm_create_errhandler(eh, &errhandler);

    MPI_Session_set_errhandler(shandle, MPI_ERRORS_RETURN);

    /* create a group from the WORLD process set */
    rc = MPI_Group_from_session_pset(shandle, pset_name, &ghandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* try to get a communicator with NULL comm as arg */
    rc = MPI_Comm_create_from_group(ghandle, "org.mpi-forum.mpi-v4_0.example-ex10_8",
                                    MPI_INFO_NULL, errhandler, NULL);
    if (rc == MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* get a communicator */
    rc = MPI_Comm_create_from_group(ghandle, "org.mpi-forum.mpi-v4_0.example-ex10_8",
                                    MPI_INFO_NULL, errhandler, &comm);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    if (calls != 1) {
        /* errhandler should have been called exactly once at this point */
        errs++;
        fprintf(stderr, "Errhandler of MPI_Comm_create_from_group called %i times (expected 1)\n",
                calls);
    }

  fn_exit:

    if (ghandle != MPI_GROUP_NULL) {
        MPI_Group_free(&ghandle);
    }

    if (comm != MPI_COMM_NULL) {
        MPI_Comm_free(&comm);
    }

    if (errhandler != MPI_ERRHANDLER_NULL) {
        MPI_Errhandler_free(&errhandler);
    }

    rc = MPI_Session_finalize(&shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned error code: %i\n", rc);
    }

    if (errs == 0) {
        fprintf(stdout, " No Errors\n");
    } else {
        fprintf(stderr, "%d Errors\n", errs);
    }
    return MTestReturnValue(errs);
}
