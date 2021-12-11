/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run attr_attrdeleteget
int run(const char *arg);
#endif

static int key = MPI_KEYVAL_INVALID;
static char a[100];

static int delete_fn(MPI_Comm, int, void *, void *);

int run(const char *arg)
{
    int errs = 0;

    MPI_Comm scomm;
    MPI_Comm_split(MPI_COMM_WORLD, 1, 0, &scomm);
    MPI_Comm_create_keyval(MPI_NULL_COPY_FN, delete_fn, &key, &errs);
    MPI_Comm_set_attr(scomm, key, a);
    MPI_Comm_free(&scomm);
    MPI_Comm_free_keyval(&key);

    return errs;
}

static int delete_fn(MPI_Comm comm, int keyval, void *attr_val, void *extra_state)
{
    /* The standard is not explicit that the 'comm' argument of
     * delete_fn must be valid, so this test is only in effect when
     * !USE_STRICT_MPI. */
#ifndef USE_STRICT_MPI
    int err, flg, *errs = extra_state;
    void *ptr;

    if (comm == MPI_COMM_NULL) {
        printf("MPI_COMM_NULL passed to delete_fn\n");
        (*errs)++;
    }
    err = MPI_Comm_get_attr(comm, key, &ptr, &flg);
    if (err != MPI_SUCCESS) {
        printf("MPI_Comm_get_attr returned error %d, presumably due to invalid communicator\n",
               err);
        (*errs)++;
    }
#endif
    return 0;
}
