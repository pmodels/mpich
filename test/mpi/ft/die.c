/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpi.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char **argv)
{
    int rank, size;
    pid_t pid;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 1) {
        pid = getpid();
        kill(pid, SIGSTOP);
    }

    MPI_Finalize();

    if (rank == 0) {
        printf("No Errors\n");
    }

    return 0;
}
