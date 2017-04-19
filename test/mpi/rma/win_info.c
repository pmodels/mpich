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
int rank, nproc;

static int check_win_info_get(MPI_Win win, const char *key, const char *exp_val)
{
    int flag = 0;
    MPI_Info info_out = MPI_INFO_NULL;
    char buf[MPI_MAX_INFO_VAL];
    int errors = 0;

    MPI_Win_get_info(win, &info_out);
    MPI_Info_get(info_out, key, MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag || strncmp(buf, exp_val, strlen(exp_val)) != 0) {
        if (flag)
            printf("%d: %s: expected \"%s\" but got %s\n", rank, key, exp_val, buf);
        else
            printf("%d: %s not defined\n", rank, key);
        errors++;
    }
    else if (flag && VERBOSE)
        printf("%d: %s = %s\n", rank, key, buf);

    MPI_Info_free(&info_out);

    return errors;
}

static void win_info_set(MPI_Win win, const char *key, const char *set_val)
{
    MPI_Info info_in = MPI_INFO_NULL;

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, key, set_val);

    MPI_Win_set_info(win, info_in);
    MPI_Info_free(&info_in);
}

int main(int argc, char **argv)
{
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
    errors += check_win_info_get(win, "no_locks", "true");

    MPI_Info_free(&info_in);

    /* We create a new window with no info argument for the next text to ensure that we have the
     * default settings */

    MPI_Win_free(&win);
    MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &base, &win);

    /* Test#2: setting and getting invalid key */

    win_info_set(win, invalid_key, "true");

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

    MPI_Info_free(&info_out);


    /* Test#3: setting info key "no_lock" (no default value) */
    win_info_set(win, "no_locks", "false");
    errors += check_win_info_get(win, "no_locks", "false");

    win_info_set(win, "no_locks", "true");
    errors += check_win_info_get(win, "no_locks", "true");


    /* Test#4: getting/setting "accumulate_ordering" */
    /*   #4.1: is the default "rar,raw,war,waw" as stated in the standard? */
    errors += check_win_info_get(win, "accumulate_ordering", "rar,raw,war,waw");

    /*   #4.2: setting "accumulate_ordering" to "none" */
    win_info_set(win, "accumulate_ordering", "none");
    errors += check_win_info_get(win, "accumulate_ordering", "none");


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
