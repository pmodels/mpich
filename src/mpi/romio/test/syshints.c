/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void handle_error(int errcode, const char *str)
{
    char msg[MPI_MAX_ERROR_STRING];
    int resultlen;
    MPI_Error_string(errcode, msg, &resultlen);
    fprintf(stderr, "%s: %s\n", str, msg);
    MPI_Abort(MPI_COMM_WORLD, 1);
}

#define CHECK(fn) {int errcode; errcode = (fn); if (errcode != MPI_SUCCESS) handle_error(errcode, #fn); }

static int hint_check(MPI_Info info_used, const char *key, const char *expected)
{
    char value[MPI_MAX_INFO_VAL + 1];
    int flag;

    CHECK(MPI_Info_get(info_used, key, MPI_MAX_INFO_VAL, value, &flag));
    if (flag) {
        if (strcmp(expected, value)) {
            fprintf(stderr, "Error: expected value \"%s\" for key \"%s\" got \"%s\"\n",
                    expected, key, value);
            return 1;
        }
    } else {
        fprintf(stderr, "Error: key \"%s\" is missing in info_used\n", key);
        return 1;
    }
    return 0;
}

static void print_info(MPI_Info * info_used)
{
    int i, nkeys;

    MPI_Info_get_nkeys(*info_used, &nkeys);
    for (i = 0; i < nkeys; i++) {
        char key[MPI_MAX_INFO_KEY], value[MPI_MAX_INFO_VAL];
        int valuelen, flag;

        MPI_Info_get_nthkey(*info_used, i, key);
        MPI_Info_get_valuelen(*info_used, key, &valuelen, &flag);
        MPI_Info_get(*info_used, key, valuelen + 1, value, &flag);
        printf("info_used: [%2d] key = %25s, value = %s\n", i, key, value);
    }
}

int main(int argc, char **argv)
{
    MPI_File fh;
    MPI_Info info_used, info_mine;
    int rank, nr_errors = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* read hints from hint file argv[1], as set in environment variable
     * ROMIO_HINTS. File argv[1] should be $srcdir/romio_hintfile
     */
    setenv("ROMIO_HINTS", argv[1], 1);

    MPI_Info_create(&info_mine);
    MPI_Info_set(info_mine, "romio_cb_read", "disable");

    CHECK(MPI_File_open(MPI_COMM_WORLD, argv[1], MPI_MODE_RDONLY, info_mine, &fh));
    CHECK(MPI_File_get_info(fh, &info_used));

    /* check if hint romio_cb_read set in argv[1] overwrites the one set above */
    nr_errors += hint_check(info_used, "romio_cb_read", "enable");

    /* check if hints set in argv[1] overwrites the defaults */
    nr_errors += hint_check(info_used, "ind_rd_buffer_size", "49");
    nr_errors += hint_check(info_used, "romio_no_indep_rw", "true");

    if (nr_errors == 0)
        printf(" No Errors\n");
    else {
        if (rank == 0)
            print_info(&info_used);
    }

    CHECK(MPI_Info_free(&info_mine));
    CHECK(MPI_Info_free(&info_used));
    CHECK(MPI_File_close(&fh));
    MPI_Finalize();
    return (nr_errors > 0);
}
