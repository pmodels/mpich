/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include "mpitest.h"

#define BAD_ANSWER 100000

int assoc(int *, int *, int *, MPI_Datatype *);

/*
    The operation is inoutvec[i] = invec[i] op inoutvec[i]
    (see 4.9.4).  The order is important.

    Note that the computation is in process rank (in the communicator)
    order, independant of the root.
 */
int assoc(int *invec, int *inoutvec, int *len, MPI_Datatype * dtype)
{
    int i;
    for (i = 0; i < *len; i++) {
        if (inoutvec[i] <= invec[i]) {
            int rank;
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            fprintf(stderr, "[%d] inout[0] = %d, in[0] = %d\n", rank, inoutvec[0], invec[0]);
            inoutvec[i] = BAD_ANSWER;
        } else
            inoutvec[i] = invec[i];
    }
    return (1);
}

int main(int argc, char **argv)
{
    int rank, size;
    int data;
    int errors = 0;
    int result = -100;
    MPI_Op op;

    MTest_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    data = rank;

    MPI_Op_create((MPI_User_function *) assoc, 0, &op);
    MPI_Reduce(&data, &result, 1, MPI_INT, op, size - 1, MPI_COMM_WORLD);
    MPI_Bcast(&result, 1, MPI_INT, size - 1, MPI_COMM_WORLD);
    MPI_Op_free(&op);
    if (result == BAD_ANSWER)
        errors++;

    MTest_Finalize(errors);
    return MTestReturnValue(errors);
}
