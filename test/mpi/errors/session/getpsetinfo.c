/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Test for error handling of MPI_Session_get_pset_info */

int main(int argc, char *argv[])
{
    int errs, rc;
    errs = 0;

    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Session shandle = MPI_SESSION_NULL;

    rc = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned error code: %i\n", rc);
        goto fn_exit;
    }

    MPI_Session_set_errhandler(shandle, MPI_ERRORS_RETURN);

    /* Test positive case */
    rc = MPI_Session_get_pset_info(shandle, "mpi://WORLD", &sinfo);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Session_get_pset_info: got error code for mpi://WORLD pset: %i\n", rc);
        errs++;
    }
    if (sinfo == MPI_INFO_NULL) {
        fprintf(stderr, "MPI_Session_get_pset_info: returned no error but info is MPI_INFO_NULL\n");
        errs++;
    } else {
        MPI_Info_free(&sinfo);
    }

    /* Test negative case */
    rc = MPI_Session_get_pset_info(shandle, "does-not-exist", &sinfo);
    if (rc == MPI_SUCCESS) {
        fprintf(stderr, "MPI_Session_get_pset_info: got MPI_SUCCESS for non-existing pset\n");
        MPI_Info_free(&sinfo);
        errs++;
    }

    rc = MPI_Session_finalize(&shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_finalize returned error code: %i\n", rc);
    }

  fn_exit:
    if (errs == 0) {
        fprintf(stdout, " No Errors\n");
    } else {
        fprintf(stderr, "%d Errors\n", errs);
    }
    return MTestReturnValue(errs);
}
