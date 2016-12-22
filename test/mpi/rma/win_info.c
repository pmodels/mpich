/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include "mpitest.h"

#define VERBOSE 0

int main(int argc, char **argv)
{
    int rank, nproc;
    MPI_Info info_in, info_out;
    int errors = 0, all_errors = 0;
    MPI_Win win;
    void *base;
    char invalid_key[] = "invalid_test_key";
    char buf[MPI_MAX_INFO_VAL];
    int flag;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    /* Test#1: setting a valid key at window-create time */

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "no_locks", "true");

    MPI_Win_allocate(sizeof(int), sizeof(int), info_in, MPI_COMM_WORLD, &base, &win);

    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, "no_locks", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strncmp(buf, "true", strlen("true")) != 0) {
        if (!flag)
            printf("%d: no_locks is not defined\n", rank);
        else
            printf("%d: no_locks = %s, expected true\n", rank, buf);
        errors++;
    }

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);

    /* We create a new window with no info argument for the next text to ensure that we have the
     * default settings */

    MPI_Win_free(&win);
    MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &base, &win);

    /* Test#2: setting and getting invalid key */

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, invalid_key, "true");

    MPI_Win_set_info(win, info_in);
    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, invalid_key, MPI_MAX_INFO_VAL, buf, &flag);
#ifndef USE_STRICT_MPI
    /* Check if our invalid key was ignored.  Note, this check's MPICH's
     * behavior, but this behavior may not be required for a standard
     * conforming MPI implementation. */
    if (flag) {
        printf("%d: %s was not ignored\n", rank, invalid_key);
        errors++;
    }
#endif

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);

    /* Test#3: setting info key "no_lock" to false and getting the key */

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "no_locks", "false");

    MPI_Win_set_info(win, info_in);
    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, "no_locks", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strncmp(buf, "false", strlen("false")) != 0) {
        if (!flag)
            printf("%d: no_locks is not defined\n", rank);
        else
            printf("%d: no_locks = %s, expected false\n", rank, buf);
        errors++;
    }
    if (flag && VERBOSE)
        printf("%d: no_locks = %s\n", rank, buf);

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);

    /* Test#4: setting info key "no_lock" to true and getting the key */

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "no_locks", "true");

    MPI_Win_set_info(win, info_in);
    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, "no_locks", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strncmp(buf, "true", strlen("true")) != 0) {
        if (!flag)
            printf("%d: no_locks is not defined\n", rank);
        else
            printf("%d: no_locks = %s, expected true\n", rank, buf);
        errors++;
    }
    if (flag && VERBOSE)
        printf("%d: no_locks = %s\n", rank, buf);

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);

    /* Test#5: getting/setting "accumulate_ordering" */
    MPI_Win_get_info(win, &info_out);

    /*   #5.1: is the default "rar,raw,war,waw" as stated in the standard? */
    MPI_Info_get(info_out, "accumulate_ordering", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strcmp(buf, "rar,raw,war,waw") != 0) {
        if (flag)
            printf("%d: accumulate_ordering: expected \"rar,raw,war,waw\" but got %s\n", rank, buf);
        else
            printf("%d: accumulate_ordering not defined\n", rank);
        errors++;
    }
    else if (flag && VERBOSE)
        printf("%d: accumulate_ordering = %s\n", rank, buf);

    MPI_Info_free(&info_out);

    /*   #5.2: setting "accumulate_ordering" to "none" */

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "accumulate_ordering", "none");

    MPI_Win_set_info(win, info_in);
    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, "accumulate_ordering", MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strcmp(buf, "none") != 0) {
        if (flag)
            printf("%d: accumulate_ordering: expected \"none\" but got %s\n", rank, buf);
        else
            printf("%d: accumulate_ordering not defined\n", rank);
        errors++;
    }
    else if (flag && VERBOSE)
        printf("%d: accumulate_ordering = %s\n", rank, buf);

    MPI_Info_free(&info_in);
    MPI_Info_free(&info_out);

    /* Test#6: getting other info keys */

    MPI_Win_get_info(win, &info_out);

    MPI_Info_get(info_out, "accumulate_ops", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE)
        printf("%d: accumulate_ops = %s\n", rank, buf);

    MPI_Info_get(info_out, "same_size", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE)
        printf("%d: same_size = %s\n", rank, buf);

    MPI_Info_get(info_out, "alloc_shm", MPI_MAX_INFO_VAL, buf, &flag);
    if (flag && VERBOSE)
        printf("%d: alloc_shm = %s\n", rank, buf);

    MPI_Info_free(&info_out);
    MPI_Win_free(&win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
