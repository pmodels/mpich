/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/* adapted from init/session.c */

#define NTHREADS 4

int thread_errs[NTHREADS];

MTEST_THREAD_RETURN_TYPE library_foo_test(void *p);

int main(int argc, char *argv[])
{
    int provided;
    MTest_init_thread_pkg();
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    for (int i = 1; i < NTHREADS; i++) {
        MTest_Start_thread(library_foo_test, (void *) (long) i);
    }

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    thread_errs[0] += MTestTestIntracomm(MPI_COMM_WORLD);

    MTest_Join_threads();
    MPI_Finalize();
    MTest_finalize_thread_pkg();

    int errs = 0;
    for (int i = 0; i < NTHREADS; i++) {
        errs += thread_errs[i];
    }

    if (rank == 0 && errs == 0) {
        printf("No Errors\n");
    }
    return MTestReturnValue(errs);
}

static bool library_foo_init(int thread_idx, MPI_Session * p_session, MPI_Comm * p_comm);
static void library_foo_finalize(int thread_idx, MPI_Session * p_session, MPI_Comm * p_comm);

MTEST_THREAD_RETURN_TYPE library_foo_test(void *p)
{
    int rank, size;
    int thread_idx = (int) (long) p;

    MPI_Session lib_shandle = MPI_SESSION_NULL;
    MPI_Comm lib_comm = MPI_COMM_NULL;
    if (library_foo_init(thread_idx, &lib_shandle, &lib_comm)) {
        MPI_Comm_size(lib_comm, &size);
        MPI_Comm_rank(lib_comm, &rank);

        thread_errs[thread_idx] += MTestTestIntracomm(lib_comm);

        library_foo_finalize(thread_idx, &lib_shandle, &lib_comm);
    }

    return MTEST_THREAD_RETVAL_IGN;
}

static bool library_foo_init(int thread_idx, MPI_Session * p_session, MPI_Comm * p_comm)
{
    int rc, flag;
    int ret = MPI_SUCCESS;
    const char *pset_name;
    char out_value[100];        /* large enough */

    /* Let's test both WORLD and SELF. e.g. with 4 threads, thread 2 will run on SELF */
    if (thread_idx % 2) {
        pset_name = "mpi://WORLD";
    } else {
        pset_name = "mpi://SELF";
    }

    MPI_Group wgroup = MPI_GROUP_NULL;
    rc = MPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, p_session);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        printf("MPI_Session_init failed in thread %d\n", thread_idx);
        goto fn_exit;
    }

    /* check we got thread support level foo library needs */
    MPI_Info tinfo = MPI_INFO_NULL;
    const char mt_key[] = "thread_level";
    const char mt_value[] = "MPI_THREAD_MULTIPLE";
    rc = MPI_Session_get_info(*p_session, &tinfo);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        goto fn_exit;
    }

    MPI_Info_get(tinfo, mt_key, sizeof(out_value), out_value, &flag);
    if (flag != 1) {
        thread_errs[thread_idx]++;
        printf("Could not find key %s\n", mt_key);
        goto fn_exit;
    }
    if (strcmp(out_value, mt_value)) {
        thread_errs[thread_idx]++;
        printf("Did not get thread multiple support, got %s\n", out_value);
        goto fn_exit;
    }

    /* create a group from the WORLD process set */
    rc = MPI_Group_from_session_pset(*p_session, pset_name, &wgroup);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        printf("MPI_Group_from_session_pset failed in thread %d\n", thread_idx);
        goto fn_exit;
    }

    /* get a communicator */
    char string_tag[20];
    sprintf(string_tag, "thread %d", thread_idx);
    rc = MPI_Comm_create_from_group(wgroup, string_tag, MPI_INFO_NULL, MPI_ERRORS_RETURN, p_comm);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        printf("MPI_Comm_create_from_group failed in thread %d\n", thread_idx);
        goto fn_exit;
    }

    /* free group, library doesn’t need it. */
  fn_exit:
    MPI_Group_free(&wgroup);
    if (tinfo != MPI_INFO_NULL) {
        MPI_Info_free(&tinfo);
    }
    if (ret != MPI_SUCCESS) {
        MPI_Session_finalize(p_session);
    }
    return (ret == MPI_SUCCESS);
}

static void library_foo_finalize(int thread_idx, MPI_Session * p_session, MPI_Comm * p_comm)
{
    int rc;

    rc = MPI_Comm_free(p_comm);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        printf("MPI_Comm_free returned %d\n", rc);
        return;
    }

    rc = MPI_Session_finalize(p_session);
    if (rc != MPI_SUCCESS) {
        thread_errs[thread_idx]++;
        printf("MPI_Session_finalize returned %d\n", rc);
        return;
    }
}
