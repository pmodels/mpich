/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

static int verbose = 0;
static int test_info(MPI_Info info1, MPI_Info info2);

int main(int argc, char *argv[])
{
    int nerrs = 0;
    MPI_Info info1, info2;

    MPI_Info_create_env(argc, argv, &info1);
    MPI_Info_create_env(argc, argv, &info2);
    nerrs += test_info(info1, info2);

    /* Free info1 and create it again */
    MPI_Info_free(&info1);
    MPI_Info_create_env(argc, argv, &info1);
    nerrs += test_info(info1, info2);

    MPI_Init(&argc, &argv);
    /* After MPI_Init() */

    /* Free info1 and create it again */
    MPI_Info_free(&info1);
    MPI_Info_create_env(argc, argv, &info1);
    nerrs += test_info(info1, info2);

    MPI_Finalize();
    /* After MPI_Finalize() */

    /* Free info1 and create it again */
    MPI_Info_free(&info1);
    MPI_Info_create_env(argc, argv, &info1);
    nerrs += test_info(info1, info2);

    MPI_Info_free(&info1);
    MPI_Info_free(&info2);

    if (nerrs) {
        printf(" Found %d errors\n", nerrs);
    } else {
        printf(" No Errors\n");
    }
    return nerrs ? 1 : 0;
}

static int test_info(MPI_Info info1, MPI_Info info2)
{
    int i, nerrs = 0;
    char value1[MPI_MAX_INFO_VAL], value2[MPI_MAX_INFO_VAL];
    const char *keys[] = { "command", "argv", "maxprocs", "soft", "host", "arch", "wdir", "file",
        "thread_level"
    };
    for (i = 0; i < (int) (sizeof(keys) / sizeof(keys[0])); i++) {
        int flag1, flag2;
        int size1 = MPI_MAX_INFO_VAL, size2 = MPI_MAX_INFO_VAL;
        MPI_Info_get_string(info1, keys[i], &size1, value1, &flag1);
        MPI_Info_get_string(info2, keys[i], &size2, value2, &flag2);
        if (verbose && flag1)
            printf("info1 %s: %s\n", keys[i], value1);
        if (verbose && flag2)
            printf("info2 %s: %s\n", keys[i], value2);
        if (flag1 == 0 && flag2 == 0) {
            /* Both failed. */
            continue;
        } else if (flag1 == 1 && flag2 == 1 && strcmp(value1, value2) == 0) {
            /* Both return the same. */
            continue;
        } else {
            /* info1 != info2 */
            nerrs++;
        }
    }
    return nerrs;
}
