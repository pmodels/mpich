/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run pt2pt_greq1
int run(const char *arg);
#endif

/*
static char MTEST_Descrip[] = "Simple test of generalized requests";
*/


static int query_fn(void *extra_state, MPI_Status * status)
{
    /* Set a default status */
    status->MPI_SOURCE = MPI_UNDEFINED;
    status->MPI_TAG = MPI_UNDEFINED;
    MPI_Status_set_cancelled(status, 0);
    MPI_Status_set_elements(status, MPI_BYTE, 0);
    return 0;
}

static int free_fn(void *extra_state)
{
    int *b = (int *) extra_state;
    if (b)
        *b = *b - 1;
    /* The value returned by the free function is the error code
     * returned by the wait/test function */
    return 0;
}

static int cancel_fn(void *extra_state, int complete)
{
    return 0;
}

/*
 * This is a very simple test of generalized requests.  Normally, the
 * MPI_Grequest_complete function would be called from another routine,
 * often running in a separate thread.  This simple code allows us to
 * check that requests can be created, tested, and waited on in the
 * case where the request is complete before the wait is called.
 *
 * Note that MPI did *not* define a routine that can be called within
 * test or wait to advance the state of a generalized request.
 * Most uses of generalized requests will need to use a separate thread.
 */
int run(const char *arg)
{
    int errs = 0;
    int counter, flag;
    MPI_Status status;
    MPI_Request request;

    MPI_Grequest_start(query_fn, free_fn, cancel_fn, NULL, &request);

    MPI_Test(&request, &flag, &status);
    if (flag) {
        errs++;
        fprintf(stderr, "Generalized request marked as complete\n");
    }

    MPI_Grequest_complete(request);

    MPI_Wait(&request, &status);

    counter = 1;
    MPI_Grequest_start(query_fn, free_fn, cancel_fn, &counter, &request);
    MPI_Grequest_complete(request);
    MPI_Wait(&request, MPI_STATUS_IGNORE);

    if (counter) {
        errs++;
        fprintf(stderr, "Free routine not called, or not called with extra_data");
    }

    return errs;
}
