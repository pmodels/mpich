/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of a large number of derived-datatype messages eagerly, with no preposted receive so that an MPI implementation may have to queue up messages on the sending side";
*/

#define MAX_MSGS 30

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, dest, source;
    int i, indices[40];
    MPI_Aint extent;
    int *buf, *bufs[MAX_MSGS];
    MPI_Comm comm;
    MPI_Datatype dtype;
    MPI_Request req[MAX_MSGS];

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    source = 0;
    dest = size - 1;

    /* Setup by creating a blocked datatype that is likely to be processed
     * in a piecemeal fashion */
    for (i = 0; i < 30; i++) {
        indices[i] = i * 40;
    }

    /* 30 blocks of size 10 */
    MPI_Type_create_indexed_block(30, 10, indices, MPI_INT, &dtype);
    MPI_Type_commit(&dtype);

    /* Create the corresponding message buffers */
    MPI_Type_extent(dtype, &extent);
    for (i = 0; i < MAX_MSGS; i++) {
        bufs[i] = (int *) malloc(extent);
        if (!bufs[i]) {
            fprintf(stderr, "Unable to allocate buffer %d of size %ld\n", i, (long) extent);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }
    buf = (int *) malloc(10 * 30 * sizeof(int));

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == dest) {
        MTestSleep(2);
        for (i = 0; i < MAX_MSGS; i++) {
            MPI_Recv(buf, 10 * 30, MPI_INT, source, i, comm, MPI_STATUS_IGNORE);
        }
    }
    else if (rank == source) {
        for (i = 0; i < MAX_MSGS; i++) {
            MPI_Isend(bufs[i], 1, dtype, dest, i, comm, &req[i]);
        }
        MPI_Waitall(MAX_MSGS, req, MPI_STATUSES_IGNORE);
    }

    MPI_Type_free(&dtype);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
