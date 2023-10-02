/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Test for strict_finalize parameter and reference counting of MPI Sessions */

int main(int argc, char *argv[])
{
    int rc;
    int errs = 0;
    int rank = 0;
    int size = 0;
    const char sf_key[] = "strict_finalize";
    const char sf_value[] = "1";

    MPI_Session shandle = MPI_SESSION_NULL;
    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Group ghandle = MPI_GROUP_NULL;
    MPI_Comm commhandle = MPI_COMM_NULL;

    MPI_Info_create(&sinfo);
    MPI_Info_set(sinfo, sf_key, sf_value);

    rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned error: %d\n", rc);
        goto fn_exit;
    }

    MPI_Session_set_errhandler(shandle, MPI_ERRORS_RETURN);

    rc = MPI_Group_from_session_pset(shandle, "mpi://WORLD", &ghandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Group_from_session_pset returned error: %d\n", rc);
    }

    rc = MPI_Comm_create_from_group(ghandle, "org.mpi-forum.mpi-v4_0.example-ex10_8",
                                    MPI_INFO_NULL, MPI_ERRORS_RETURN, &commhandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Comm_create_from_group returned error: %d\n", rc);
    }

    MPI_Comm_rank(commhandle, &rank);
    MPI_Comm_size(commhandle, &size);

    int sum;
    MPI_Reduce(&rank, &sum, 1, MPI_INT, MPI_SUM, 0, commhandle);
    if (rank == 0) {
        if (sum != (size - 1) * size / 2) {
            fprintf(stderr, "MPI_Reduce: expect %d, got %d\n", (size - 1) * size / 2, sum);
            errs++;
        }
    }

    rc = MPI_Session_finalize(&shandle);
    if (rc == MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned no error on strict_finalize\n");
        goto fn_exit;
    }

    MPI_Group_free(&ghandle);

    rc = MPI_Session_finalize(&shandle);
    if (rc == MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned no error on strict_finalize\n");
        goto fn_exit;
    }

    MPI_Comm_free(&commhandle);

    /* Here we should be able to finalize the session */
    rc = MPI_Session_finalize(&shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned error: %d\n", rc);
    }

  fn_exit:
    if (sinfo != MPI_INFO_NULL) {
        MPI_Info_free(&sinfo);
    }

    if (ghandle != MPI_GROUP_NULL) {
        MPI_Group_free(&ghandle);
    }

    if (commhandle != MPI_COMM_NULL) {
        MPI_Comm_free(&commhandle);
    }

    if (rank == 0) {
        if (errs == 0) {
            fprintf(stdout, " No Errors\n");
        } else {
            fprintf(stderr, "%d Errors\n", errs);
        }
    }

    return MTestReturnValue(errs);
}
