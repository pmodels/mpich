/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* Exercise user callback error propagation. */

static int query_error = MPI_SUCCESS;
static int query_fn(void *ctx, MPI_Status *s) { return query_error; }
static int free_fn(void *ctx) { return MPI_SUCCESS; }
static int cancel_fn (void *ctx, int c) { return MPI_SUCCESS; }

int main(int argc, char **argv)
{
    int errs = 0, err, flag;
    MPI_Request request = MPI_REQUEST_NULL;

#if !defined(USE_STRICT_MPI) && defined(MPICH)
    int errcls, rlen;
    char errstr[MPI_MAX_ERROR_STRING], str[64];
#endif

    MTest_Init(&argc, &argv);

    MPI_Grequest_start(query_fn, free_fn, cancel_fn, NULL, &request);
    MPI_Grequest_complete(request);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    query_error = MPI_SUCCESS;
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err != MPI_SUCCESS) {
        errs++;
        printf("callback return code was not MPI_SUCCESS\n");
    }

    /* callback returns an error code which is unknown to MPI */
    query_error = 42000;
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("callback return error code was MPI_SUCCESS\n");
    }
#if !defined(USE_STRICT_MPI) && defined(MPICH)
    MPI_Error_class(err, &errcls);
    MPI_Error_string(err, errstr, &rlen);
    snprintf(str, sizeof(str), "error code %d", query_error);
    if (err == query_error) {
        errs++;
        printf("callback return error code was 'query_error'\n");
    }
    if (errcls != MPI_ERR_OTHER) {
        errs++;
        printf("callback return error class was not MPI_ERR_OTHER\n");
    }
    if (strstr(errstr, str) == NULL) {
        errs++;
        printf("error string did not contain 'error code %d'\n", query_error);
    }
#endif

    /* callback returns a predefined error class */
    query_error = MPI_ERR_ARG;
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err == MPI_SUCCESS) {
        errs++;
        printf("callback return error code was MPI_SUCCESS\n");
    }
#if !defined(USE_STRICT_MPI) && defined(MPICH)
    MPI_Error_class(err, &errcls);
    MPI_Error_string(err, errstr, &rlen);
    snprintf(str, sizeof(str), "error code %d", query_error);
    if (err == MPI_ERR_ARG) {
        errs++;
        printf("callback return error code was MPI_ERR_ARG\n");
    }
    if (errcls != MPI_ERR_ARG) {
        errs++;
        printf("callback return error class was not MPI_ERR_ARG\n");
    }
    if (strstr(errstr, str) == NULL) {
        errs++;
        printf("error string did not contain 'error code %d'\n", query_error);
    }
#endif

    /* callback returns a dynamic error class */
    MPI_Add_error_class(&query_error);
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err != query_error) {
        errs++;
        printf("callback return error code was not dynamic error class\n");
    }
#if 10*MPI_VERSION+MPI_SUBVERSION >= 41
    MPI_Remove_error_class(query_error);
#endif

    /* callback returns a a dynamic error code */
    MPI_Add_error_code(MPI_ERR_ARG, &query_error);
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err != query_error) {
        errs++;
        printf("callback return error code was not dynamic error code\n");
    }
#if 10*MPI_VERSION+MPI_SUBVERSION >= 41
    MPI_Remove_error_code(query_error);
#endif

    query_error = MPI_SUCCESS;
    err = MPI_Request_get_status(request, &flag, MPI_STATUS_IGNORE);
    if (err != MPI_SUCCESS) {
        errs++;
        printf("callback function return code was not MPI_SUCCESS\n");
    }

    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);
    MPI_Request_free(&request);

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
