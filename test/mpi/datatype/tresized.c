/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of type resized";
*/

int main(int argc, char *argv[])
{
    int errs = 0, i;
    int rank, size, source, dest;
    int count;
    int *buf;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Datatype newtype;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    /* Determine the sender and receiver */
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    source = 0;
    dest = size - 1;

    MPI_Type_create_resized(MPI_INT, 0, 3 * sizeof(int), &newtype);
    MPI_Type_commit(&newtype);
    for (count = 1; count < 65000; count = count * 2) {
        buf = (int *) malloc(count * 3 * sizeof(int));
        if (!buf) {
            MPI_Abort(comm, 1);
        }
        for (i = 0; i < 3 * count; i++)
            buf[i] = -1;
        if (rank == source) {
            for (i = 0; i < count; i++)
                buf[3 * i] = i;
            MPI_Send(buf, count, newtype, dest, 0, comm);
            MPI_Send(buf, count, newtype, dest, 1, comm);
        }
        else if (rank == dest) {
            MPI_Recv(buf, count, MPI_INT, source, 0, comm, &status);
            for (i = 0; i < count; i++) {
                if (buf[i] != i) {
                    errs++;
                    if (errs < 10) {
                        printf("buf[%d] = %d\n", i, buf[i]);
                    }
                }
            }
            for (i = 0; i < count * 3; i++)
                buf[i] = -1;
            MPI_Recv(buf, count, newtype, source, 1, comm, &status);
            for (i = 0; i < count; i++) {
                if (buf[3 * i] != i) {
                    errs++;
                    if (errs < 10) {
                        printf("buf[3*%d] = %d\n", i, buf[i]);
                    }
                }
            }
        }
    }

    MPI_Type_free(&newtype);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
