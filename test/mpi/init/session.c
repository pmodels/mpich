/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <assert.h>
#include "mpitest.h"

/* This is example 10.8 from MPI Standard 4.0
 * Simple example illustrating creation of an MPI communicator using the Session
 * Model. The pre-defined "mpi://WORLD" process set can be used to first create
 * a local MPI group and then subsequently to create an MPI communicator from
 * this group.
 */

int errs = 0;

int library_foo_test(void);
void library_foo_init(void);
void library_foo_finalize(void);

int main(int argc, char *argv[])
{
#ifdef MULT_INIT
    int rank, size, provided;
    int num_repeat = 1;

    if (argc > 1) {
        num_repeat = atoi(argv[1]);
        /* basic sanity check */
        assert(num_repeat > 0 && num_repeat < 100);
    }

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    for (int i = 0; i < num_repeat; i++) {
        library_foo_test();
    }
    MPI_Finalize();
#else
    int rank = library_foo_test();
#endif
    if (rank == 0 && errs == 0) {
        printf("No Errors\n");
    }
    return MTestReturnValue(errs);
}

static MPI_Session lib_shandle = MPI_SESSION_NULL;
static MPI_Comm lib_comm = MPI_COMM_NULL;

int library_foo_test(void)
{
    int rank, size;

    library_foo_init();

    MPI_Comm_size(lib_comm, &size);
    MPI_Comm_rank(lib_comm, &rank);

    int sum;
    MPI_Reduce(&rank, &sum, 1, MPI_INT, MPI_SUM, 0, lib_comm);
    if (rank == 0) {
        if (sum != (size - 1) * size / 2) {
            printf("MPI_Reduce: expect %d, got %d\n", (size - 1) * size / 2, sum);
            errs++;
        }
    }

    library_foo_finalize();

    return rank;
}

void library_foo_init(void)
{
    int rc, flag;
    int ret = 0;
    const char pset_name[] = "mpi://WORLD";
    const char mt_key[] = "thread_level";
    const char mt_value[] = "MPI_THREAD_MULTIPLE";
    char out_value[100];        /* large enough */

    MPI_Group wgroup = MPI_GROUP_NULL;
    MPI_Info sinfo = MPI_INFO_NULL;
    MPI_Info tinfo = MPI_INFO_NULL;
    MPI_Info_create(&sinfo);
    MPI_Info_set(sinfo, mt_key, mt_value);
    rc = MPI_Session_init(sinfo, MPI_ERRORS_RETURN, &lib_shandle);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* check we got thread support level foo library needs */
    rc = MPI_Session_get_info(lib_shandle, &tinfo);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    MPI_Info_get(tinfo, mt_key, sizeof(out_value), out_value, &flag);
    if (!flag) {
        printf("Could not find key %s\n", mt_key);
        errs++;
        goto fn_exit;
    }
    if (strcmp(out_value, mt_value)) {
        printf("Did not get thread multiple support, got %s\n", out_value);
        errs++;
        goto fn_exit;
    }

    /* create a group from the WORLD process set */
    rc = MPI_Group_from_session_pset(lib_shandle, pset_name, &wgroup);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* get a communicator */
    rc = MPI_Comm_create_from_group(wgroup, "org.mpi-forum.mpi-v4_0.example-ex10_8",
                                    MPI_INFO_NULL, MPI_ERRORS_RETURN, &lib_comm);
    if (rc != MPI_SUCCESS) {
        errs++;
        goto fn_exit;
    }

    /* free group, library doesn’t need it. */
  fn_exit:
    MPI_Group_free(&wgroup);
    if (sinfo != MPI_INFO_NULL) {
        MPI_Info_free(&sinfo);
    }
    if (tinfo != MPI_INFO_NULL) {
        MPI_Info_free(&tinfo);
    }
    if (ret != MPI_SUCCESS) {
        MPI_Session_finalize(&lib_shandle);
    }
}

void library_foo_finalize(void)
{
    int rc;

    rc = MPI_Comm_free(&lib_comm);
    if (rc != MPI_SUCCESS) {
        printf("MPI_Comm_free returned %d\n", rc);
        errs++;
        return;
    }

    rc = MPI_Session_finalize(&lib_shandle);
    if (rc != MPI_SUCCESS) {
        printf("MPI_Session_finalize returned %d\n", rc);
        errs++;
        return;
    }
}
