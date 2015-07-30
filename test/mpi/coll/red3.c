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
static char MTEST_Descrip[] = "Test MPI_Reduce with non-commutative user-define operations";
*/
/*
 * This tests that the reduce operation respects the noncommutative flag.
 * See red4.c for a version that can distinguish between P_{root} P_{root+1}
 * ... P_{root-1} and P_0 ... P_{size-1} .  The MPI standard clearly
 * specifies that the result is P_0 ... P_{size-1}, independent of the root
 * (see 4.9.4 in MPI-1)
 */

/* This implements a simple matrix-matrix multiply.  This is an associative
   but not commutative operation.  The matrix size is set in matSize;
   the number of matrices is the count argument. The matrix is stored
   in C order, so that
     c(i,j) is cin[j+i*matSize]
 */
#define MAXCOL 256
static int matSize = 0;         /* Must be < MAXCOL */
void uop(void *cinPtr, void *coutPtr, int *count, MPI_Datatype * dtype);
void uop(void *cinPtr, void *coutPtr, int *count, MPI_Datatype * dtype)
{
    const int *cin = (const int *) cinPtr;
    int *cout = (int *) coutPtr;
    int i, j, k, nmat;
    int tempCol[MAXCOL];

    for (nmat = 0; nmat < *count; nmat++) {
        for (j = 0; j < matSize; j++) {
            for (i = 0; i < matSize; i++) {
                tempCol[i] = 0;
                for (k = 0; k < matSize; k++) {
                    /* col[i] += cin(i,k) * cout(k,j) */
                    tempCol[i] += cin[k + i * matSize] * cout[j + k * matSize];
                }
            }
            for (i = 0; i < matSize; i++) {
                cout[j + i * matSize] = tempCol[i];
            }
        }
    }
}

/* Initialize the integer matrix as a permutation of rank with rank+1.
   If we call this matrix P_r, we know that product of P_0 P_1 ... P_{size-2}
   is a left shift by 1.
*/

static void initMat(MPI_Comm comm, int mat[])
{
    int i, size, rank;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    for (i = 0; i < size * size; i++)
        mat[i] = 0;

    /* For each row */
    for (i = 0; i < size; i++) {
        if (rank != size - 1) {
            if (i == rank)
                mat[((i + 1) % size) + i * size] = 1;
            else if (i == ((rank + 1) % size))
                mat[((i + size - 1) % size) + i * size] = 1;
            else
                mat[i + i * size] = 1;
        }
        else {
            mat[i + i * size] = 1;
        }
    }
}

#ifdef FOO
/* Compare a matrix with the identity matrix */
static int isIdentity(MPI_Comm comm, int mat[])
{
    int i, j, size, rank, errs = 0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (i == j) {
                if (mat[j + i * size] != 1) {
                    errs++;
                }
            }
            else {
                if (mat[j + i * size] != 0) {
                    errs++;
                }
            }
        }
    }
    return errs;
}
#endif

/* Compare a matrix with the identity matrix */
static int isShiftLeft(MPI_Comm comm, int mat[])
{
    int i, j, size, rank, errs = 0;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            if (i == ((j + 1) % size)) {
                if (mat[j + i * size] != 1) {
                    errs++;
                }
            }
            else {
                if (mat[j + i * size] != 0) {
                    errs++;
                }
            }
        }
    }
    return errs;
}

int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, root;
    int minsize = 2, count;
    MPI_Comm comm;
    int *buf, *bufout;
    MPI_Op op;
    MPI_Datatype mattype;

    MTest_Init(&argc, &argv);

    MPI_Op_create(uop, 0, &op);

    while (MTestGetIntracommGeneral(&comm, minsize, 1)) {
        if (comm == MPI_COMM_NULL)
            continue;

        MPI_Comm_size(comm, &size);
        MPI_Comm_rank(comm, &rank);

        matSize = size; /* used by the user-defined operation */
        /* Only one matrix for now */
        count = 1;

        /* A single matrix, the size of the communicator */
        MPI_Type_contiguous(size * size, MPI_INT, &mattype);
        MPI_Type_commit(&mattype);

        buf = (int *) malloc(count * size * size * sizeof(int));
        if (!buf)
            MPI_Abort(MPI_COMM_WORLD, 1);
        bufout = (int *) malloc(count * size * size * sizeof(int));
        if (!bufout)
            MPI_Abort(MPI_COMM_WORLD, 1);

        for (root = 0; root < size; root++) {
            initMat(comm, buf);
            MPI_Reduce(buf, bufout, count, mattype, op, root, comm);
            if (rank == root) {
                errs += isShiftLeft(comm, bufout);
            }

            /* Try the same test, but using MPI_IN_PLACE */
            initMat(comm, bufout);
            if (rank == root) {
                MPI_Reduce(MPI_IN_PLACE, bufout, count, mattype, op, root, comm);
            }
            else {
                MPI_Reduce(bufout, NULL, count, mattype, op, root, comm);
            }
            if (rank == root) {
                errs += isShiftLeft(comm, bufout);
            }
        }

        free(buf);
        free(bufout);

        MPI_Type_free(&mattype);

        MTestFreeComm(&comm);
    }

    MPI_Op_free(&op);

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
