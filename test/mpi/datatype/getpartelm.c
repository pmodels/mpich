/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTest_descrip[] = "Receive partial datatypes and check that\
MPI_Getelements gives the correct version";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    MPI_Datatype outtype, oldtypes[2];
    MPI_Aint offsets[2];
    int blklens[2];
    MPI_Comm comm;
    int size, rank, src, dest, tag;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (size < 2) {
        errs++;
        printf("This test requires at least 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    src = 0;
    dest = 1;

    if (rank == src) {
        int buf[128], position, cnt;
        /* sender */

        /* Create a datatype and send it (multiple of sizeof(int)) */
        /* Create a send struct type */
        oldtypes[0] = MPI_INT;
        oldtypes[1] = MPI_CHAR;
        blklens[0] = 1;
        blklens[1] = 4 * sizeof(int);
        offsets[0] = 0;
        offsets[1] = sizeof(int);
        MPI_Type_struct(2, blklens, offsets, oldtypes, &outtype);
        MPI_Type_commit(&outtype);

        buf[0] = 4 * sizeof(int);
        /* printf("About to send to %d\n", dest); */
        MPI_Send(buf, 1, outtype, dest, 0, comm);
        MPI_Type_free(&outtype);

        /* Create a datatype and send it (not a multiple of sizeof(int)) */
        /* Create a send struct type */
        oldtypes[0] = MPI_INT;
        oldtypes[1] = MPI_CHAR;
        blklens[0] = 1;
        blklens[1] = 4 * sizeof(int) + 1;
        offsets[0] = 0;
        offsets[1] = sizeof(int);
        MPI_Type_struct(2, blklens, offsets, oldtypes, &outtype);
        MPI_Type_commit(&outtype);

        buf[0] = 4 * sizeof(int) + 1;
        MPI_Send(buf, 1, outtype, dest, 1, comm);
        MPI_Type_free(&outtype);

        /* Pack data and send as packed */
        position = 0;
        cnt = 7;
        MPI_Pack(&cnt, 1, MPI_INT, buf, 128 * sizeof(int), &position, comm);
        MPI_Pack((void *) "message", 7, MPI_CHAR, buf, 128 * sizeof(int), &position, comm);
        MPI_Send(buf, position, MPI_PACKED, dest, 2, comm);
    }
    else if (rank == dest) {
        MPI_Status status;
        int buf[128], i, elms, count;

        /* Receiver */
        /* Create a receive struct type */
        oldtypes[0] = MPI_INT;
        oldtypes[1] = MPI_CHAR;
        blklens[0] = 1;
        blklens[1] = 256;
        offsets[0] = 0;
        offsets[1] = sizeof(int);
        MPI_Type_struct(2, blklens, offsets, oldtypes, &outtype);
        MPI_Type_commit(&outtype);

        for (i = 0; i < 3; i++) {
            tag = i;
            /* printf("about to receive tag %d from %d\n", i, src); */
            MPI_Recv(buf, 1, outtype, src, tag, comm, &status);
            MPI_Get_elements(&status, outtype, &elms);
            if (elms != buf[0] + 1) {
                errs++;
                printf("For test %d, Get elements gave %d but should be %d\n", i, elms, buf[0] + 1);
            }
            MPI_Get_count(&status, outtype, &count);
            if (count != MPI_UNDEFINED) {
                errs++;
                printf("For partial send, Get_count did not return MPI_UNDEFINED\n");
            }
        }
        MPI_Type_free(&outtype);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;

}
