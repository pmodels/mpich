/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Simple info test */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

int main(int argc, char *argv[])
{
    MPI_Info i1, i2;
    int errs = 0;
    char value[64];
    int flag;

    MPI_Init(0, 0);

    MPI_Info_create(&i1);
    MPI_Info_create(&i2);

    MPI_Info_set(i1, (char *) "key1", (char *) "value1");
    MPI_Info_set(i2, (char *) "key2", (char *) "value2");

    MPI_Info_get(i1, (char *) "key2", 64, value, &flag);
    if (flag) {
        printf("Found key2 in info1\n");
        errs++;
    }
    MPI_Info_get(i1, (char *) "key1", 64, value, &flag);
    if (!flag) {
        errs++;
        printf("Did not find key1 in info1\n");
    }
    else if (strcmp(value, "value1")) {
        errs++;
        printf("Found wrong value (%s), expected value1\n", value);
    }

    MPI_Info_free(&i1);
    MPI_Info_free(&i2);
    if (errs) {
        printf(" Found %d errors\n", errs);
    }
    else {
        printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}
