/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
#include <stdio.h>
#include <stddef.h>
#include <assert.h>

/* Tests MPI_Get_elements with a contiguous datatype that triggered a bug in
 * past versions of MPICH.  See ticket #1467 for more info. */

struct test_struct {
    char a;
    short b;
    int c;
};

int main(int argc, char **argv)
{
    int rank, count;
    struct test_struct sendbuf, recvbuf;
    int blens[3];
    MPI_Aint displs[3];
    MPI_Datatype types[3];
    MPI_Datatype struct_type, contig;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* We use a contig of a struct in order to satisfy two properties:
     * (A) a type that contains more than one element type (the struct portion)
     * (B) a type that has an odd number of ints in its "type contents" (1 in
     *     this case)
     * This triggers a specific bug in some versions of MPICH. */
    blens[0] = 1;
    displs[0] = offsetof(struct test_struct, a);
    types[0] = MPI_CHAR;
    blens[1] = 1;
    displs[1] = offsetof(struct test_struct, b);
    types[1] = MPI_SHORT;
    blens[2] = 1;
    displs[2] = offsetof(struct test_struct, c);
    types[2] = MPI_INT;
    MPI_Type_create_struct(3, blens, displs, types, &struct_type);
    MPI_Type_contiguous(1, struct_type, &contig);
    MPI_Type_commit(&struct_type);
    MPI_Type_commit(&contig);

    sendbuf.a = 20;
    sendbuf.b = 30;
    sendbuf.c = 40;
    recvbuf.a = -1;
    recvbuf.b = -1;
    recvbuf.c = -1;

    /* send to ourself */
    MPI_Sendrecv(&sendbuf, 1, contig, 0, 0, &recvbuf, 1, contig, 0, 0, MPI_COMM_SELF, &status);

    /* sanity */
    assert(sendbuf.a == recvbuf.a);
    assert(sendbuf.b == recvbuf.b);
    assert(sendbuf.c == recvbuf.c);

    /* now check that MPI_Get_elements returns the correct answer and that the
     * library doesn't explode in the process */
    count = 0xdeadbeef;
    MPI_Get_elements(&status, contig, &count);
    MPI_Type_free(&struct_type);
    MPI_Type_free(&contig);

    if (count != 3) {
        printf("unexpected value for count, expected 3, got %d\n", count);
    }
    else {
        if (rank == 0) {
            printf(" No Errors\n");
        }
    }

    MPI_Finalize();
    return 0;
}
