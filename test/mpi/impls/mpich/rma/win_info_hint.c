/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include "mpitest.h"

#define VERBOSE 0
#define MAX_N_HINTS 10
static char query_key[MAX_N_HINTS][MPI_MAX_INFO_KEY], val[MAX_N_HINTS][MPI_MAX_INFO_VAL];
static int nhints = 0, rank;

static int check_win_info_get(MPI_Win win, const char *key, const char *exp_val)
{
    int flag = 0;
    MPI_Info info_out = MPI_INFO_NULL;
    char buf[MPI_MAX_INFO_VAL];
    int errors = 0;

    MPI_Win_get_info(win, &info_out);
    MPI_Info_get(info_out, key, MPI_MAX_INFO_VAL, buf, &flag);
    if (!flag) {
        fprintf(stderr, "%d: Hint %s was not set\n", rank, key);
        errors++;
    }
    if (strcmp(buf, exp_val)) {
        fprintf(stderr, "%d: Hint %s, expected value: %s, received: %s\n", rank, key, exp_val, buf);
        errors++;
    }

    if (!errors && VERBOSE)
        fprintf(stdout, "%d: %s = %s\n", rank, key, buf);

    MPI_Info_free(&info_out);

    return errors;
}

static void print_usage(char **argv)
{
    fprintf(stdout,
            "Usage: %s -hint=[infokey] -value=[VALUE] (-hint=[infokey] -value=[VALUE]...)\n",
            argv[0]);
    fflush(stdout);
}

static int prepare_hints(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i += 2) {
        memset(query_key[nhints], 0, sizeof(query_key[nhints]));
        memset(val[nhints], 0, sizeof(val[nhints]));

        if (nhints >= MAX_N_HINTS) {
            fprintf(stderr, "This program accepts at most %d pairs of hint and value\n",
                    MAX_N_HINTS);
            return 1;
        }

        if (!strncmp(argv[i], "-hint=", strlen("-hint=")))
            strncpy(query_key[nhints], argv[i] + strlen("-hint="), MPI_MAX_INFO_KEY);
        if (!strncmp(argv[i + 1], "-value=", strlen("-value=")) && i + 1 < argc)
            strncpy(val[nhints], argv[i + 1] + strlen("-value="), MPI_MAX_INFO_KEY);

        if (strlen(query_key[nhints]) == 0 || strlen(val[nhints]) == 0) {
            fprintf(stderr, "Invalid parameters %s, %s\n", argv[i], argv[i + 1]);
            return 1;
        }

        nhints++;
    }
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Info info_in;
    int errors = 0, i;
    MPI_Win win;
    void *base;


    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc < 3 || argc % 2 == 0) {
        fprintf(stderr, "This program requires at least a pair of hint and value, "
                "and each hint and value have to be paired.\n");
        print_usage(argv);
        return MTestReturnValue(1);
    }

    /* Read each pair of info key and value. Return if invalid input is found. */
    if (prepare_hints(argc, argv)) {
        print_usage(argv);
        return MTestReturnValue(1);
    }

    /* Run test */
    MPI_Info_create(&info_in);
    for (i = 0; i < nhints; i++) {
        MPI_Info_set(info_in, query_key[i], val[i]);
    }
    MPI_Win_allocate(sizeof(int), sizeof(int), info_in, MPI_COMM_WORLD, &base, &win);
    for (i = 0; i < nhints; i++) {
        errors += check_win_info_get(win, query_key[i], val[i]);
    }
    MPI_Info_free(&info_in);
    MPI_Win_free(&win);

    MTest_Finalize(errors);
    return MTestReturnValue(errors);
}
