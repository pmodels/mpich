/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define PATH_MAX 1000

#include "mpi.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    int rank, size, namelen;
    FILE *fp;
    char path[PATH_MAX];
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Get_processor_name(processor_name, &namelen);

    fp = popen("grep Cpus_allowed_list /proc/$$/status", "r");

    while (fgets(path, PATH_MAX, fp) != NULL) {
        printf("%s[%d]: %s", processor_name, rank, path);
    }

    pclose(fp);

    fflush(stdout);

    MPI_Finalize();

    return 0;
}
