/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2014 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#define PATH_MAX 100

#include "mpi.h"
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* This example can be used to check on which node+CPU
 * each process is running on. This can serve to confirm
 * The correctness of a Process-CPU binding policy for instance.
 *
 * To get the CPU Id on which a process is running,
 * one can use the getcpu() or sched_getcpu().
 * However, getcpu() is only supported by Linux kernels
 * >= 2.6.19 for x86_64 and i386 architectures. Also,
 * sched_getcpu() is only available since glibc 2.6.
 *
 * This example implements an alternative, for systems
 * not satisfying these requirements, by checking the column
 * 39 of /proc/$pid/stat.
 *
 */

int main(int argc, char **argv)
{
    int rank, size, namelen;
    FILE *fp;
    char path[PATH_MAX];
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Get_processor_name(processor_name, &namelen);

    char shell_cmd[100];
    sprintf(shell_cmd, "cat /proc/%d/stat | awk '{print $39}'", getpid());

    if (rank == 0) {
        printf("----------------------\n");
        printf(" PROCESS/CPU BINDING\n");
        printf("----------------------\n");
    }

    fp = popen(shell_cmd, "r");

    while (fgets(path, PATH_MAX, fp) != NULL) {
        printf("%s[%d]: running on CPU %s", processor_name, rank, path);
    }

    pclose(fp);

    fflush(stdout);


    MPI_Finalize();

    return 0;
}
