/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include <mpitest.h>
#include <assert.h>

#ifdef MULTI_TESTS
#define run coll_bcastzerotype
int run(const char *arg);
#endif

/* test broadcast behavior with non-zero counts but zero-sized types */

int run(const char *arg)
{
    int i, type_size;
    MPI_Datatype type = MPI_DATATYPE_NULL;
    char *buf = NULL;
    int wrank, wsize;

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

    free(buf);

    MPI_Type_free(&type);

    return 0;
}
