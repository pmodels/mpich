/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

#define MAX_MSGS 30

/*
static char MTEST_Descrip[] = "One implementation delivered incorrect data when an MPI recieve uses both ANY_SOURCE and ANY_TAG";
*/

int main(int argc, char *argv[])
{
    int wrank, wsize, master, worker, i, j, idx, count;
    int errs = 0;
    MPI_Request r[MAX_MSGS];
    int buf[MAX_MSGS][MAX_MSGS];
    MPI_Comm comm;
    MPI_Status status;

    MTest_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    comm = MPI_COMM_WORLD;
    master = 0;
    worker = 1;

    /* The test takes advantage of the ordering rules for messages */

    if (wrank == master) {
        /* Initialize the send buffer */
        for (i = 0; i < MAX_MSGS; i++) {
            for (j = 0; j < MAX_MSGS; j++) {
                buf[i][j] = i * MAX_MSGS + j;
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
        for (i = 0; i < MAX_MSGS; i++) {
            MPI_Send(buf[i], MAX_MSGS - i, MPI_INT, worker, 3, comm);
        }
    }
    else if (wrank == worker) {
        /* Initialize the recv buffer */
        for (i = 0; i < MAX_MSGS; i++) {
            for (j = 0; j < MAX_MSGS; j++) {
                buf[i][j] = -1;
            }
        }
        for (i = 0; i < MAX_MSGS; i++) {
            MPI_Irecv(buf[i], MAX_MSGS - i, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &r[i]);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        for (i = 0; i < MAX_MSGS; i++) {
            MPI_Waitany(MAX_MSGS, r, &idx, &status);
            /* Message idx should have length MAX_MSGS-idx */
            MPI_Get_count(&status, MPI_INT, &count);
            if (count != MAX_MSGS - idx) {
                errs++;
            }
            else {
                /* Check for the correct answers */
                for (j = 0; j < MAX_MSGS - idx; j++) {
                    if (buf[idx][j] != idx * MAX_MSGS + j) {
                        errs++;
                        printf("Message %d [%d] is %d, should be %d\n",
                               idx, j, buf[idx][j], idx * MAX_MSGS + j);
                    }
                }
            }
        }
    }
    else {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MTest_Finalize(errs);
    MPI_Finalize();

    return 0;
}
