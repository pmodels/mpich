/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include "mpi.h"

#define NUM_CPUS 16
#define STRLEN (NUM_CPUS * 10)

int main(int argc, char **argv)
{
    int rank, size, ret, i, num_cpus = -1;
    char str[STRLEN], foo[STRLEN];
    cpu_set_t mask;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    while (++argv && *argv) {
        if (!strcmp(argv[0], "--num-cpus")) {
            num_cpus = atoi(argv[1]);
            argv += 2;
        }
        else if (!strcmp(argv[0], "--help") || !strcmp(argv[0], "-help") || !strcmp(argv[0], "-h")) {
            fprintf(stderr, "Usage: ./binding {--num-cpus [CPUs]}\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    if (num_cpus < 0)
        num_cpus = NUM_CPUS;

    CPU_ZERO(&mask);
    ret = sched_getaffinity(getpid(), sizeof(cpu_set_t), &mask);
    if (ret < 0) {
        fprintf(stderr, "sched_getaffinity failure\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    snprintf(foo, STRLEN, "[%d] ", rank);
    for (i = 0; i < num_cpus; i++) {
        snprintf(str, STRLEN, "%s %d ", foo, CPU_ISSET(i, &mask));
        snprintf(foo, STRLEN, "%s", str);
    }
    fprintf(stderr, "%s\n", str);

    MPI_Finalize();

    return 0;
}
