/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Test code provided by Hajime Fujita. See Trac ticket #2225. */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"
#include <string.h>

int main(int argc, char *argv[])
{
    MPI_Info info;
    const char *key = "key", *val = "val";
    char buff[3 + 1];           /* strlen("val") + 1 */
    int flag, errs = 0;

    MTest_Init(&argc, &argv);

    MPI_Info_create(&info);
    MPI_Info_set(info, key, val);
    MPI_Info_get(info, key, sizeof(buff) - 1, buff, &flag);
    if (flag) {
        if (strncmp(buff, val, sizeof(buff) - 1) != 0) {
            errs++;
            printf("returned value is %s, should be %s\n", buff, val);
        }
    }
    else {
        errs++;
        printf("key not found\n");
    }
    MPI_Info_free(&info);

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
