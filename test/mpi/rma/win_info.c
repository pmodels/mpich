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
    MPI_Comm shm_comm = MPI_COMM_NULL;

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

    /*   #4.3: setting "accumulate_ordering" to "rar,waw" */
    win_info_set(win, "accumulate_ordering", "rar,waw");
    errors += check_win_info_get(win, "accumulate_ordering", "rar,waw");


    /* Test#5: getting/setting "accumulate_ops" */
    /*   #5.1: is the default "same_op_no_op" as stated in the standard? */
    errors += check_win_info_get(win, "accumulate_ops", "same_op_no_op");

    /*   #5.2: setting "accumulate_ops" to "same_op" */
    win_info_set(win, "accumulate_ops", "same_op");
    errors += check_win_info_get(win, "accumulate_ops", "same_op");


    /* Test#6: setting "same_size" (no default value) */
    win_info_set(win, "same_size", "false");
    errors += check_win_info_get(win, "same_size", "false");

    win_info_set(win, "same_size", "true");
    errors += check_win_info_get(win, "same_size", "true");


    /* Test#7: setting "same_disp_unit" (no default value) */
    win_info_set(win, "same_disp_unit", "false");
    errors += check_win_info_get(win, "same_disp_unit", "false");

    win_info_set(win, "same_disp_unit", "true");
    errors += check_win_info_get(win, "same_disp_unit", "true");

    /* TODO: check alloc_shm as implementation-specific test */

    /* Test#8: setting "alloc_shared_noncontig" (no default value) in shared window. */
    MPI_Win_free(&win);

    /*   #8.1: setting at window allocation */
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, rank, MPI_INFO_NULL, &shm_comm);

    MPI_Info_create(&info_in);
    MPI_Info_set(info_in, "alloc_shared_noncontig", "true");
    MPI_Win_allocate_shared(sizeof(int), sizeof(int), info_in, shm_comm, &base, &win);
    errors += check_win_info_get(win, "alloc_shared_noncontig", "true");
    MPI_Info_free(&info_in);

    /*   #8.2: setting info */
    win_info_set(win, "alloc_shared_noncontig", "false");
    errors += check_win_info_get(win, "alloc_shared_noncontig", "false");
    MPI_Comm_free(&shm_comm);

    MPI_Win_free(&win);

    MPI_Reduce(&errors, &all_errors, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0 && all_errors == 0)
        printf(" No Errors\n");

    MPI_Finalize();

    return 0;
}
