/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Test for error handling of MPI_Session_get_nth_pset */

int main(int argc, char *argv[])
{
    int errs, rc, n_psets, psetname_len;
    errs = 0;
    n_psets = 0;
    psetname_len = 0;
    char *pset_name;

    MPI_Session shandle = MPI_SESSION_NULL;

    rc = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        fprintf(stderr, "MPI_Session_init returned error code: %i\n", rc);
        goto fn_exit;
    }

    MPI_Session_set_errhandler(shandle, MPI_ERRORS_RETURN);

    /* Test positive case: pset at position 0 always exists because of MPI default psets */
    rc = MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, 0, &psetname_len, NULL);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr,
                "MPI_Session_get_nth_pset: requesting pset name length at index 0 returned error code: %i\n",
                rc);
        errs++;
    }

    /* Provoke error with null pointer as pset_name */
    rc = MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, 0, &psetname_len, NULL);
    if (rc == MPI_SUCCESS) {
        fprintf(stderr, "MPI_Session_get_nth_pset: null pointer pset_name did not return error\n");
        errs++;
    }

    pset_name = (char *) malloc(sizeof(char) * psetname_len);
    rc = MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, 0, &psetname_len, pset_name);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr,
                "MPI_Session_get_nth_pset: requesting pset at index 0 returned error code: %i\n",
                rc);
        errs++;
    }
    free(pset_name);

    /* Test negative case: pset at position n_psets + 1 does not exist */
    rc = MPI_Session_get_num_psets(shandle, MPI_INFO_NULL, &n_psets);
    if (rc != MPI_SUCCESS) {
        fprintf(stderr, "MPI_Session_get_num_psets: returned error code: %i\n", rc);
        errs++;
    }

    psetname_len = 0;
    rc = MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, n_psets + 1, &psetname_len, NULL);
    if (rc == MPI_SUCCESS) {
        fprintf(stderr,
                "MPI_Session_get_nth_pset: requesting pset name length at index n + 1 did not return error\n");
        errs++;
    }

    psetname_len = 10;
    pset_name = (char *) malloc(sizeof(char) * psetname_len);
    rc = MPI_Session_get_nth_pset(shandle, MPI_INFO_NULL, n_psets + 1, &psetname_len, pset_name);
    if (rc == MPI_SUCCESS) {
        fprintf(stderr,
                "MPI_Session_get_nth_pset: requesting pset at index n + 1 did not return error\n");
        errs++;
    }
    free(pset_name);

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
