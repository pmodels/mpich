/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#define NKEYS 3
int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Info info;
    char *keys1[NKEYS] = { (char *) "file", (char *) "soft", (char *) "host" };
    char *values1[NKEYS] = { (char *) "runfile.txt", (char *) "2:1000:4,3:1000:7",
        (char *) "myhost.myorg.org"
    };

    char value[MPI_MAX_INFO_VAL];
    int i, flag;

    MTest_Init(&argc, &argv);

    /* 1,2,3 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_set(info, keys1[i], values1[i]);
    }

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    /* 3,2,1 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    for (i = NKEYS - 1; i >= 0; i--) {
        MPI_Info_set(info, keys1[i], values1[i]);
    }

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    /* 1,3,2 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    MPI_Info_set(info, keys1[0], values1[0]);
    MPI_Info_set(info, keys1[2], values1[2]);
    MPI_Info_set(info, keys1[1], values1[1]);

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    /* 2,1,3 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    MPI_Info_set(info, keys1[1], values1[1]);
    MPI_Info_set(info, keys1[0], values1[0]);
    MPI_Info_set(info, keys1[2], values1[2]);

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    /* 2,3,1 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    MPI_Info_set(info, keys1[1], values1[1]);
    MPI_Info_set(info, keys1[2], values1[2]);
    MPI_Info_set(info, keys1[0], values1[0]);

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    /* 3,1,2 */
    MPI_Info_create(&info);
    /* Use only named keys incase the info implementation only supports
     * the predefined keys (e.g., IBM) */
    MPI_Info_set(info, keys1[2], values1[2]);
    MPI_Info_set(info, keys1[0], values1[0]);
    MPI_Info_set(info, keys1[1], values1[1]);

    /* Check that all values are present */
    for (i = 0; i < NKEYS; i++) {
        MPI_Info_get(info, keys1[i], MPI_MAX_INFO_VAL, value, &flag);
        if (!flag) {
            errs++;
            printf("No value for key %s\n", keys1[i]);
        }
        if (strcmp(value, values1[i])) {
            errs++;
            printf("Incorrect value for key %s\n", keys1[i]);
        }
    }
    MPI_Info_free(&info);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
