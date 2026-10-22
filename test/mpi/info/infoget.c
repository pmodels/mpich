/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Test code provided by Hajime Fujita. See Trac ticket #2225. */

#include "mpitest.h"
#include <string.h>

#ifdef MULTI_TESTS
#define run info_infoget
int run(const char *arg);
#endif

int run(const char *arg)
{
    MPI_Info info;
    const char *key = "key", *val = "val";
    char buff[3 + 1];           /* strlen("val") + 1 */
    int flag, errs = 0;

    MPI_Info_create(&info);
    MPI_Info_set(info, key, val);
    MPI_Info_get(info, key, sizeof(buff) - 1, buff, &flag);
    if (flag) {
        if (strncmp(buff, val, sizeof(buff) - 1) != 0) {
            errs++;
            printf("returned value is %s, should be %s\n", buff, val);
        }
    } else {
        errs++;
        printf("key not found\n");
    }
    MPI_Info_free(&info);

    return errs;
}
