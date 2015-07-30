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
static char MTEST_Descrip[] = "Test that freed datatypes have reference count semantics";
*/

#define VEC_NELM 128
#define VEC_STRIDE 8

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, source, dest, i;
    MPI_Comm comm;
    MPI_Status status;
    MPI_Request req;
    MPI_Datatype strideType;
    MPI_Datatype tmpType[1024];
    int *buf = 0;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    if (size < 2) {
        fprintf(stderr, "This test requires at least two processes.");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    source = 0;
    dest = size - 1;

    /*
     * The idea here is to create a simple but non-contig datatype,
     * perform an irecv with it, free it, and then create
     * many new datatypes.  While not a complete test, if the datatype
     * was freed and the space was reused, this test may detect
     * that error
     * A similar test for sends might work by sending a large enough message
     * to force the use of rendezvous send.
     */
    MPI_Type_vector(VEC_NELM, 1, VEC_STRIDE, MPI_INT, &strideType);
    MPI_Type_commit(&strideType);

    if (rank == dest) {
        buf = (int *) malloc(VEC_NELM * VEC_STRIDE * sizeof(int));
        for (i = 0; i < VEC_NELM * VEC_STRIDE; i++)
            buf[i] = -i;
        MPI_Irecv(buf, 1, strideType, source, 0, comm, &req);
        MPI_Type_free(&strideType);

        for (i = 0; i < 1024; i++) {
            MPI_Type_vector(VEC_NELM, 1, 1, MPI_INT, &tmpType[i]);
            MPI_Type_commit(&tmpType[i]);
        }

        MPI_Sendrecv(NULL, 0, MPI_INT, source, 1, NULL, 0, MPI_INT, source, 1, comm, &status);

        MPI_Wait(&req, &status);
        for (i = 0; i < VEC_NELM; i++) {
            if (buf[VEC_STRIDE * i] != i) {
                errs++;
                if (errs < 10) {
                    printf("buf[%d] = %d, expected %d\n", VEC_STRIDE * i, buf[VEC_STRIDE * i], i);
                }
            }
        }
        for (i = 0; i < 1024; i++) {
            MPI_Type_free(&tmpType[i]);
        }
        free(buf);
    }
    else if (rank == source) {
        buf = (int *) malloc(VEC_NELM * sizeof(int));
        for (i = 0; i < VEC_NELM; i++)
            buf[i] = i;
        /* Synchronize with the receiver */
        MPI_Sendrecv(NULL, 0, MPI_INT, dest, 1, NULL, 0, MPI_INT, dest, 1, comm, &status);
        MPI_Send(buf, VEC_NELM, MPI_INT, dest, 0, comm);
        free(buf);
    }

    /* Clean up the strideType */
    if (rank != dest) {
        MPI_Type_free(&strideType);
    }


    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
