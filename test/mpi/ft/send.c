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

/* 
 * This test attempts communication between 2 running processes
 * after another process has failed.
 */
int main(int argc, char **argv)
{
    int rank, size, err;
    char buf[10];
    pid_t pid;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 1) {
        pid = getpid();
        kill(pid, SIGKILL);
    }

    MTestSleep(1);

    if (rank == 0) {
        err = MPI_Send("No Errors", 10, MPI_CHAR, 2, 0, MPI_COMM_WORLD);
    }

    if (rank == 2) {
        MPI_Recv(buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("%s\n", buf);
        fflush( stdout );
    }

    MPI_Finalize();

    return 0;
}
