/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpitest.h"

#ifdef MULTI_TESTS
#define run coll_coll9
int run(const char *arg);
#endif

static void addem(int *invec, int *inoutvec, int *len, MPI_Datatype * dtype)
{
    int i;
    for (i = 0; i < *len; i++)
        inoutvec[i] += invec[i];
}

int run(const char *arg)
{
    int rank, size, i;
    int data;
    int errors = 0;
    int result = -100;
    int correct_result;
    MPI_Op op;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    data = rank;
    MPI_Op_create((MPI_User_function *) addem, 1, &op);
    MPI_Reduce(&data, &result, 1, MPI_INT, op, 0, MPI_COMM_WORLD);
    MPI_Bcast(&result, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Op_free(&op);
    correct_result = 0;
    for (i = 0; i < size; i++)
        correct_result += i;
    if (result != correct_result)
        errors++;

    return errors;
}
