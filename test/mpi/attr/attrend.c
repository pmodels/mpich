/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
      The MPI-2 specification makes it clear that delect attributes are
      called on MPI_COMM_WORLD and MPI_COMM_SELF at the very beginning of
      MPI_Finalize.  This is useful for tools that want to perform the MPI
      equivalent of an "at_exit" action.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int exit_key = MPI_KEYVAL_INVALID;
int wasCalled = 0;
int foundError = 0;
/* #define DEBUG */
int delete_fn(MPI_Comm, int, void *, void *);
#ifdef DEBUG
#define FFLUSH fflush(stdout);
#else
#define FFLUSH
#endif

int main(int argc, char **argv)
{
    int errs = 0, wrank;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    /* create the keyval for the exit handler */
    MPI_Keyval_create(MPI_NULL_COPY_FN, delete_fn, &exit_key, (void *) 0);

    /* Attach to comm_self */
    MPI_Attr_put(MPI_COMM_SELF, exit_key, (void *) 0);
    /* We can free the key now */
    MPI_Keyval_free(&exit_key);

    /* Now, exit MPI */
    /* MTest_Finalize(errs); */
    MPI_Finalize();

    /* Check that the exit handler was called, and without error */
    if (wrank == 0) {
        /* In case more than one process exits MPI_Finalize */
        if (wasCalled != 1) {
            errs++;
            printf("Attribute delete function on MPI_COMM_SELF was not called\n");
        }
        if (foundError != 0) {
            errs++;
            printf("Found %d errors while executing delete function in MPI_COMM_SELF\n",
                   foundError);
        }
        if (errs == 0) {
            printf(" No Errors\n");
        }
        else {
            printf(" Found %d errors\n", errs);
        }
        fflush(stdout);
    }

    return 0;
}

int delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    int flag;
    wasCalled++;
    MPI_Finalized(&flag);
    if (flag) {
        foundError++;
    }
    return MPI_SUCCESS;
}
