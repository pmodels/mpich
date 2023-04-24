/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Test for error handling of MPI_Session_init */

int main(int argc, char *argv[])
{
    int rc;
    int errs = 0;
    const char mt_key[] = "thread_level";
    const char mt_value[] = "MPI_THREAD_MULTIPLE";
    const char mt_invalid[] = "THIS_IS_NOT_VALID";

    MPI_Session shandle = MPI_SESSION_NULL;
    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Info_create(&sinfo);

    MPI_Info_set(sinfo, mt_key, mt_invalid);
    rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, &shandle);
    if (rc == MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned no error for invalid info param\n");
        MPI_Session_finalize(&shandle);
        goto fn_exit;
    }

    MPI_Info_set(sinfo, mt_key, mt_value);
    rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned error code: %i\n", rc);
        goto fn_exit;
    }

    MPI_Session_set_errhandler(shandle, MPI_ERRORS_RETURN);

    rc = MPI_Session_finalize(&shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned error code: %i\n", rc);
    }

  fn_exit:
    if (sinfo != MPI_INFO_NULL) {
        MPI_Info_free(&sinfo);
    }

    if (errs == 0) {
        fprintf(stdout, " No Errors\n");
    } else {
        fprintf(stderr, "%d Errors\n", errs);
    }
    return MTestReturnValue(errs);
}
