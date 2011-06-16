/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
      The MPI-2.2 specification makes it clear that attributes are called on
      MPI_COMM_WORLD and MPI_COMM_SELF at the very beginning of MPI_Finalize in
      LIFO order with respect to the order in which they are set.  This is
      useful for tools that want to perform the MPI equivalent of an "at_exit"
      action.
 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* 20 ought to be enough attributes to ensure that hash-table based MPI
 * implementations do not accidentally pass the test except by being extremely
 * "lucky".  There are (20!) possible permutations which means that there is
 * about a 1 in 2.43e18 chance of getting LIFO ordering out of a hash table,
 * assuming a decent hash function is used. */
#define NUM_TEST_ATTRS (20)

static int exit_keys[NUM_TEST_ATTRS]; /* init to MPI_KEYVAL_INVALID */
static int was_called[NUM_TEST_ATTRS];
int foundError = 0;
int delete_fn (MPI_Comm, int, void *, void *);

int main(int argc, char **argv)
{
    int errs = 0, wrank;
    int i;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

#if MTEST_HAVE_MIN_MPI_VERSION(2,2)
    for (i = 0; i < NUM_TEST_ATTRS; ++i) {
        exit_keys[i] = MPI_KEYVAL_INVALID;
        was_called[i] = 0;

        /* create the keyval for the exit handler */
        MPI_Comm_create_keyval(MPI_COMM_NULL_COPY_FN, delete_fn, &exit_keys[i], NULL);
        /* attach to comm_self */
        MPI_Comm_set_attr(MPI_COMM_SELF, exit_keys[i], (void*)(long)i);
    }

    /* we can free the keys now */
    for (i = 0; i < NUM_TEST_ATTRS; ++i) {
        MPI_Comm_free_keyval(&exit_keys[i]);
    }

    /* now, exit MPI */
    MPI_Finalize();

    /* check that the exit handlers were called in LIFO order, and without error */
    if (wrank == 0) {
        /* In case more than one process exits MPI_Finalize */
        for (i = 0; i < NUM_TEST_ATTRS; ++i) {
            if (was_called[i] < 1) {
                errs++;
                printf("Attribute delete function on MPI_COMM_SELF was not called for idx=%d\n", i);
            }
            else if (was_called[i] > 1) {
                errs++;
                printf("Attribute delete function on MPI_COMM_SELF was called multiple times for idx=%d\n", i);
            }
        }
        if (foundError != 0) {
            errs++;
            printf("Found %d errors while executing delete function in MPI_COMM_SELF\n", foundError);
        }
        if (errs == 0) {
            printf(" No Errors\n");
        }
        else {
            printf(" Found %d errors\n", errs);
        }
        fflush(stdout);
    }
#else /* this is a pre-MPI-2.2 implementation, ordering is not defined */
    MPI_Finalize();
    if (wrank == 0)
        printf(" No Errors\n");
#endif

    return 0;
}

int delete_fn(MPI_Comm comm, int keyval, void *attribute_val, void *extra_state)
{
    int flag;
    int i;
    int my_idx = (int)(long)attribute_val;

    if (my_idx < 0 || my_idx > NUM_TEST_ATTRS) {
        printf("internal error, my_idx=%d is invalid!\n", my_idx);
        fflush(stdout);
    }

    was_called[my_idx]++;

    MPI_Finalized(&flag);
    if (flag) {
        printf("my_idx=%d, MPI_Finalized returned %d, should have been 0", my_idx, flag);
        foundError++;
    }

    /* since attributes were added in 0..(NUM_TEST_ATTRS-1) order, they will be
     * called in (NUM_TEST_ATTRS-1)..0 order */
    for (i = 0; i < my_idx; ++i) {
        if (was_called[i] != 0) {
            printf("my_idx=%d, was_called[%d]=%d but should be 0\n", my_idx, i, was_called[i]);
            foundError++;
        }
    }
    for (i = my_idx; i < NUM_TEST_ATTRS; ++i) {
        if (was_called[i] != 1) {
            printf("my_idx=%d, was_called[%d]=%d but should be 1\n", my_idx, i, was_called[i]);
            foundError++;
        }
    }

    return MPI_SUCCESS;
}

