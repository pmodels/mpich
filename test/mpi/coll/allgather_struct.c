/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2015 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdlib.h>
#include "mpi.h"
#include "mpitest.h"

/* Allgather a two-field struct datatype. This test
   may trigger bugs such as when the implementation
   does not handle well misaligned types.*/

typedef struct {
    int first;
    long second;
} int_long_t;

int main(int argc, char **argv)
{
    MPI_Comm comm;
    int minsize = 2;
    int i, errs = 0;
    int rank, size;
    int_long_t object;
    MPI_Datatype type;
    MPI_Aint begin;
    MPI_Aint displacements[2];
    MPI_Datatype types[] = { MPI_INT, MPI_LONG };
    int blocklength[2] = { 1, 1 };
    int_long_t* gathered_objects;

    MTest_Init(&argc, &argv);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {

        if (comm == MPI_COMM_NULL)
            continue;
        /* Determine the sender and receiver */
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        gathered_objects = (int_long_t*) malloc (size*sizeof(int_long_t));

        /* Local object */
        object.first = rank;
        object.second = rank * 10;

        /* Datatype creation */
        MPI_Get_address(&object, &begin);
        MPI_Get_address(&object.first, &displacements[0]);
        MPI_Get_address(&object.second, &displacements[1]);

        for (i = 0; i != 2; ++i)
            displacements[i] -= begin;

        MPI_Type_create_struct(2, &blocklength[0], &displacements[0], &types[0], &type);
        MPI_Type_commit(&type);

        MPI_Allgather(&object, 1, type, gathered_objects, 1, type, comm);

        for (i = 0; i < size; i++) {
            if (gathered_objects[i].first != i || gathered_objects[i].second != i * 10)
                errs++;
        }

        MPI_Type_free(&type);
        MTestFreeComm(&comm);
        free(gathered_objects);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
