/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
 * Test user-defined operations that uses MPI_Type_get_envelope and MPI_Type_get_contents.
 * Catches the case of recursive locking in THREAD_MULTIPLE.
 * Reference to user reported issue #5259.
 */

struct string_int {
    char str[100];
    int is_equal;
};

void myop(void *in, void *out, int *count, MPI_Datatype * dtype);

/*
 * myop takes a datatype that is of struct string_int and compares the string.
 */
void myop(void *in, void *out, int *count, MPI_Datatype * dtype)
{
    int n_ints, n_aints, n_datatypes, combiner;
    MPI_Type_get_envelope(*dtype, &n_ints, &n_aints, &n_datatypes, &combiner);
    assert(combiner == MPI_COMBINER_STRUCT && n_ints == 3 && n_aints == 2 && n_datatypes == 2);

    int ints[3];
    MPI_Aint aints[2];
    MPI_Datatype types[2];
    MPI_Type_get_contents(*dtype, n_ints, n_aints, n_datatypes, ints, aints, types);
    assert(ints[0] == 2 && types[0] == MPI_CHAR && types[1] == MPI_INT);

    int *p_in_eq = (int *) ((char *) in + aints[1]);
    int *p_out_eq = (int *) ((char *) out + aints[1]);

    if (*p_in_eq == 0 || *p_out_eq == 0 || strncmp(in, out, ints[1]) != 0) {
        *p_out_eq = 0;
    }
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int wsize, wrank;
    MPI_Datatype eq_type;
    MPI_Op op;

    MTest_Init(&argc, &argv);
    MPI_Op_create(myop, 0, &op);

    MPI_Datatype types[2] = { MPI_CHAR, MPI_INT };
    int blklens[2] = { 100, 1 };
    MPI_Aint displs[2] = { 0, 100 };
    MPI_Type_create_struct(2, blklens, displs, types, &eq_type);
    MPI_Type_commit(&eq_type);

    MPI_Comm_size(MPI_COMM_WORLD, &wsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    struct string_int data, result;
    strcpy(data.str, "Testing String.");
    data.is_equal = 1;
    MPI_Reduce(&data, &result, 1, eq_type, op, 0, MPI_COMM_WORLD);
    if (!result.is_equal) {
        printf("Expect result to be equal, result not equal!\n");
        errs++;
    }
    MPI_Op_free(&op);
    MPI_Type_free(&eq_type);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
