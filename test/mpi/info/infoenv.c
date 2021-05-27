/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

static int verbose = 0;

int main(int argc, char *argv[])
{
    char value[MPI_MAX_INFO_VAL];
    const char *keys[] = { "command", "argv", "maxprocs", "soft", "host", "arch", "wdir", "file",
        "thread_level", "num_nics", "num_close_nics", 0
    };
    int flag, i;

    MTest_Init(&argc, &argv);

    for (i = 0; keys[i]; i++) {
        MPI_Info_get(MPI_INFO_ENV, keys[i], MPI_MAX_INFO_VAL, value, &flag);
        if (flag && verbose)
            printf("%s: %s\n", keys[i], value);
    }

    MTest_Finalize(0);
    return 0;
}
