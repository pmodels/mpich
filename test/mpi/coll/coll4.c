/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_coll4
int run(const char *arg);
#endif

#define MAX_PROCESSES 10

int run(const char *arg)
{
    int rank, size, i, j;
    int table[MAX_PROCESSES][MAX_PROCESSES];
    int row[MAX_PROCESSES];
    int errors = 0;
    int participants;
    MPI_Comm comm;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    comm = MPI_COMM_WORLD;

    /* A maximum of MAX_PROCESSES processes can participate */
    if (size > MAX_PROCESSES) {
        participants = MAX_PROCESSES;
        MPI_Comm_split(MPI_COMM_WORLD, rank < MAX_PROCESSES, rank, &comm);
    } else {
        participants = size;
        MPI_Comm_dup(MPI_COMM_WORLD, &comm);
    }
    if ((rank < participants)) {
        int send_count = MAX_PROCESSES;
        int recv_count = MAX_PROCESSES;

        /* If I'm the root (process 0), then fill out the big table */
        if (rank == 0)
            for (i = 0; i < participants; i++)
                for (j = 0; j < MAX_PROCESSES; j++)
                    table[i][j] = i + j;

        /* Scatter the big table to everybody's little table */
        MPI_Scatter(&table[0][0], send_count, MPI_INT, &row[0], recv_count, MPI_INT, 0, comm);

        /* Now see if our row looks right */
        for (i = 0; i < MAX_PROCESSES; i++)
            if (row[i] != i + rank)
                errors++;
    }

    MPI_Comm_free(&comm);

    return errors;
}
