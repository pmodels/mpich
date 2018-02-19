/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int verbose = 0;

int main(int argc, char *argv[])
{
    char value[MPI_MAX_INFO_VAL];
    char *keys[] = { "command", "argv", "maxprocs", "soft", "host", "arch", "wdir", "file",
        "thread_level", 0
    };
    int flag, i;

    MTest_Init(&argc, &argv);

    for (i = 0; keys[i]; i++) {
        MPI_Info_get(MPI_INFO_ENV, keys[i], MPI_MAX_INFO_VAL, value, &flag);
        if (flag && verbose)
            printf("command: %s\n", value);
    }

    MTest_Finalize(0);
    return 0;
}
