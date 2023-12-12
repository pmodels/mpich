/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
 * Adapted from the code provided by hook@nas.nasa.gov (Edward C. Hook)
 */

#include "mpitest.h"

#include <string.h>
#include <errno.h>

#ifdef MULTI_TESTS
#define run coll_coll13
int run(const char *arg);
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

int run(const char *arg)
{
    int rank, size;
    int chunk = 128;
    int i;
    int *sb;
    int *rb;
    int status;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    MTestArgList *head = MTestArgListCreate_arg(arg);
    chunk = MTestArgListGetInt_with_default(head, "chunk", 128);
    MTestArgListDestroy(head);

    sb = (int *) malloc(size * chunk * sizeof(int));
    if (!sb) {
        perror("can't allocate send buffer");
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    rb = (int *) malloc(size * chunk * sizeof(int));
    if (!rb) {
        perror("can't allocate recv buffer");
        free(sb);
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }
    for (i = 0; i < size * chunk; ++i) {
        sb[i] = rank + 1;
        rb[i] = 0;
    }

    /* fputs("Before MPI_Alltoall\n",stdout); */

    /* This should really send MPI_CHAR, but since sb and rb were allocated
     * as chunk*size*sizeof(int), the buffers are large enough */
    status = MPI_Alltoall(sb, chunk, MPI_INT, rb, chunk, MPI_INT, MPI_COMM_WORLD);

    /* fputs("Before MPI_Allreduce\n",stdout); */

    free(sb);
    free(rb);

    return status;
}
