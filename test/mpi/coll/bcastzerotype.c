/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <mpi.h>

/* test broadcast behavior with non-zero counts but zero-sized types */

int main(int argc, char *argv[])
{
    int i, type_size;
    MPI_Datatype type = MPI_DATATYPE_NULL;
    char *buf = NULL;
    int wrank, wsize;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);
    MPI_Comm_size(MPI_COMM_WORLD, &wsize);

    /* a random non-zero sized buffer */
#define NELEM (10)
    buf = malloc(NELEM * sizeof(int));
    assert(buf);

    for (i = 0; i < NELEM; i++) {
        buf[i] = wrank * NELEM + i;
    }

    /* create a zero-size type */
    MPI_Type_contiguous(0, MPI_INT, &type);
    MPI_Type_commit(&type);
    MPI_Type_size(type, &type_size);
    assert(type_size == 0);

    /* do the broadcast, which will break on some MPI implementations */
    MPI_Bcast(buf, NELEM, type, 0, MPI_COMM_WORLD);

    /* check that the buffer remains unmolested */
    for (i = 0; i < NELEM; i++) {
        assert(buf[i] == wrank * NELEM + i);
    }

    MPI_Type_free(&type);
    MPI_Finalize();

    if (wrank == 0) {
        printf(" No errors\n");
    }

    return 0;
}
