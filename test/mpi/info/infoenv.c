/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run info_infoenv
int run(const char *arg);
#endif

static int verbose = 0;

int run(const char *arg)
{
    char value[MPI_MAX_INFO_VAL];
    const char *keys[] = { "command", "argv", "maxprocs", "soft", "host", "arch", "wdir", "file",
        "thread_level", "num_nics", "num_close_nics", 0
    };
    int flag, i;

    for (i = 0; keys[i]; i++) {
        MPI_Info_get(MPI_INFO_ENV, keys[i], MPI_MAX_INFO_VAL, value, &flag);
        if (flag && verbose)
            printf("%s: %s\n", keys[i], value);
    }

    return 0;
}
