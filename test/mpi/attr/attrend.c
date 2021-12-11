/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
      The MPI-2 specification makes it clear that delect attributes are
      called on MPI_COMM_WORLD and MPI_COMM_SELF at the very beginning of
      MPI_Finalize.  This is useful for tools that want to perform the MPI
      equivalent of an "at_exit" action.
 */
/* NOTE: we modified the test to check the delete attributes behavior at
 * MPI_Comm_free. The behavior at MPI_Finalize for builtin-comms are covered
 * in attrend2.
 */
#include "mpitest.h"

#ifdef MULTI_TESTS
#define run attr_attrend
int run(const char *arg);
#endif

static int wasCalled = 0;
static int foundError = 0;
static int delete_fn(MPI_Comm, int, void *, void *);

int run(const char *arg)
{
    int errs = 0;
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Comm comm_self;
    MPI_Comm_dup(MPI_COMM_SELF, &comm_self);

    /* create the keyval for the exit handler */
    int exit_key = MPI_KEYVAL_INVALID;
    MPI_Keyval_create(MPI_NULL_COPY_FN, delete_fn, &exit_key, (void *) 0);

    /* Attach to comm_self */
    MPI_Attr_put(comm_self, exit_key, (void *) 0);
    /* We can free the key now */
    MPI_Keyval_free(&exit_key);

    MPI_Comm_free(&comm_self);

    /* Check that the exit handler was called, and without error */
    if (rank == 0) {
        /* In case more than one process exits MPI_Finalize */
        if (wasCalled != 1) {
            errs++;
            printf("Attribute delete function on comm_self was not called\n");
        }
        if (foundError != 0) {
            errs++;
            printf("Found %d errors while executing delete function in comm_self\n", foundError);
        }
    }

    return errs;
}

static int delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    int flag;
    wasCalled++;
    MPI_Finalized(&flag);
    if (flag) {
        foundError++;
    }
    return MPI_SUCCESS;
}
