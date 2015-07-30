/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  Changes to this example
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
 * This example is taken from MPI-The complete reference, Vol 1,
 * pages 222-224.
 *
 * Lines after the "--CUT HERE--" were added to make this into a complete
 * test program.
 */

/* Specify the maximum number of errors to report. */
#define MAX_ERRORS 10

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 64

MPI_Datatype transpose_type(int M, int m, int n, MPI_Datatype type);
MPI_Datatype submatrix_type(int N, int m, int n, MPI_Datatype type);
void Transpose(float *localA, float *localB, int M, int N, MPI_Comm comm);
void Transpose(float *localA, float *localB, int M, int N, MPI_Comm comm)
/* transpose MxN matrix A that is block distributed (1-D) on
   processes of comm onto block distributed matrix B  */
{
    int i, j, extent, myrank, p, n[2], m[2];
    int lasti, lastj;
    int *sendcounts, *recvcounts;
    int *sdispls, *rdispls;
    MPI_Datatype xtype[2][2], stype[2][2], *sendtypes, *recvtypes;

    MTestPrintfMsg(2, "M = %d, N = %d\n", M, N);

    /* compute parameters */
    MPI_Comm_size(comm, &p);
    MPI_Comm_rank(comm, &myrank);
    extent = sizeof(float);

    /* allocate arrays */
    sendcounts = (int *) malloc(p * sizeof(int));
    recvcounts = (int *) malloc(p * sizeof(int));
    sdispls = (int *) malloc(p * sizeof(int));
    rdispls = (int *) malloc(p * sizeof(int));
    sendtypes = (MPI_Datatype *) malloc(p * sizeof(MPI_Datatype));
    recvtypes = (MPI_Datatype *) malloc(p * sizeof(MPI_Datatype));

    /* compute block sizes */
    m[0] = M / p;
    m[1] = M - (p - 1) * (M / p);
    n[0] = N / p;
    n[1] = N - (p - 1) * (N / p);

    /* compute types */
    for (i = 0; i <= 1; i++)
        for (j = 0; j <= 1; j++) {
            xtype[i][j] = transpose_type(N, m[i], n[j], MPI_FLOAT);
            stype[i][j] = submatrix_type(M, m[i], n[j], MPI_FLOAT);
        }

    /* prepare collective operation arguments */
    lasti = myrank == p - 1;
    for (j = 0; j < p; j++) {
        lastj = j == p - 1;
        sendcounts[j] = 1;
        sdispls[j] = j * n[0] * extent;
        sendtypes[j] = xtype[lasti][lastj];
        recvcounts[j] = 1;
        rdispls[j] = j * m[0] * extent;
        recvtypes[j] = stype[lastj][lasti];
    }

    /* communicate */
    MTestPrintfMsg(2, "Begin Alltoallw...\n");
    /* -- Note that the book incorrectly uses &localA and &localB
     * as arguments to MPI_Alltoallw */
    MPI_Alltoallw(localA, sendcounts, sdispls, sendtypes,
                  localB, recvcounts, rdispls, recvtypes, comm);
    MTestPrintfMsg(2, "Done with Alltoallw\n");

    /* Free buffers */
    free(sendcounts);
    free(recvcounts);
    free(sdispls);
    free(rdispls);
    free(sendtypes);
    free(recvtypes);

    /* Free datatypes */
    for (i = 0; i <= 1; i++)
        for (j = 0; j <= 1; j++) {
            MPI_Type_free(&xtype[i][j]);
            MPI_Type_free(&stype[i][j]);
        }
}


/* Define an n x m submatrix in a n x M local matrix (this is the
   destination in the transpose matrix */
MPI_Datatype submatrix_type(int M, int m, int n, MPI_Datatype type)
/* computes a datatype for an mxn submatrix within an MxN matrix
   with entries of type type */
{
    /* MPI_Datatype subrow; */
    MPI_Datatype submatrix;

    /* The book, MPI: The Complete Reference, has the wrong type constructor
     * here.  Since the stride in the vector type is relative to the input
     * type, the stride in the book's code is n times as long as is intended.
     * Since n may not exactly divide N, it is better to simply use the
     * blocklength argument in Type_vector */
    /*
     * MPI_Type_contiguous(n, type, &subrow);
     * MPI_Type_vector(m, 1, N, subrow, &submatrix);
     */
    MPI_Type_vector(n, m, M, type, &submatrix);
    MPI_Type_commit(&submatrix);

    /* Add a consistency test: the size of submatrix should be
     * n * m * sizeof(type) and the extent should be ((n-1)*M+m) * sizeof(type) */
    {
        int tsize;
        MPI_Aint textent, lb;
        MPI_Type_size(type, &tsize);
        MPI_Type_get_extent(submatrix, &lb, &textent);

        if (textent != tsize * (M * (n - 1) + m)) {
            fprintf(stderr, "Submatrix extent is %ld, expected %ld (%d,%d,%d)\n",
                    (long) textent, (long) (tsize * (M * (n - 1) + m)), M, n, m);
        }
    }
    return (submatrix);
}

/* Extract an m x n submatrix within an m x N matrix and transpose it.
   Assume storage by rows; the defined datatype accesses by columns */
MPI_Datatype transpose_type(int N, int m, int n, MPI_Datatype type)
/* computes a datatype for the transpose of an mxn matrix
   with entries of type type */
{
    MPI_Datatype subrow, subrow1, submatrix;
    MPI_Aint lb, extent;

    MPI_Type_vector(m, 1, N, type, &subrow);
    MPI_Type_get_extent(type, &lb, &extent);
    MPI_Type_create_resized(subrow, 0, extent, &subrow1);
    MPI_Type_contiguous(n, subrow1, &submatrix);
    MPI_Type_commit(&submatrix);
    MPI_Type_free(&subrow);
    MPI_Type_free(&subrow1);

    /* Add a consistency test: the size of submatrix should be
     * n * m * sizeof(type) and the extent should be ((m-1)*N+n) * sizeof(type) */
    {
        int tsize;
        MPI_Aint textent, llb;
        MPI_Type_size(type, &tsize);
        MPI_Type_get_true_extent(submatrix, &llb, &textent);

        if (textent != tsize * (N * (m - 1) + n)) {
            fprintf(stderr, "Transpose Submatrix extent is %ld, expected %ld (%d,%d,%d)\n",
                    (long) textent, (long) (tsize * (N * (m - 1) + n)), N, n, m);
        }
    }

    return (submatrix);
}

/* -- CUT HERE -- */

int main(int argc, char *argv[])
{
    int gM, gN, lm, lmlast, ln, lnlast, i, j, errs = 0;
    int size, rank;
    float *localA, *localB;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);

    gM = 20;
    gN = 30;

    /* Each block is lm x ln in size, except for the last process,
     * which has lmlast x lnlast */
    lm = gM / size;
    lmlast = gM - (size - 1) * lm;
    ln = gN / size;
    lnlast = gN - (size - 1) * ln;

    /* Create the local matrices.
     * Initialize the input matrix so that the entries are
     * consequtive integers, by row, starting at 0.
     */
    if (rank == size - 1) {
        localA = (float *) malloc(gN * lmlast * sizeof(float));
        localB = (float *) malloc(gM * lnlast * sizeof(float));
        for (i = 0; i < lmlast; i++) {
            for (j = 0; j < gN; j++) {
                localA[i * gN + j] = (float) (i * gN + j + rank * gN * lm);
            }
        }

    }
    else {
        localA = (float *) malloc(gN * lm * sizeof(float));
        localB = (float *) malloc(gM * ln * sizeof(float));
        for (i = 0; i < lm; i++) {
            for (j = 0; j < gN; j++) {
                localA[i * gN + j] = (float) (i * gN + j + rank * gN * lm);
            }
        }
    }

    MTestPrintfMsg(2, "Allocated local arrays\n");
    /* Transpose */
    Transpose(localA, localB, gM, gN, comm);

    /* check the transposed matrix
     * In the global matrix, the transpose has consequtive integers,
     * organized by columns.
     */
    if (rank == size - 1) {
        for (i = 0; i < lnlast; i++) {
            for (j = 0; j < gM; j++) {
                int expected = i + gN * j + rank * ln;
                if ((int) localB[i * gM + j] != expected) {
                    if (errs < MAX_ERRORS)
                        printf("Found %d but expected %d\n", (int) localB[i * gM + j], expected);
                    errs++;
                }
            }
        }

    }
    else {
        for (i = 0; i < ln; i++) {
            for (j = 0; j < gM; j++) {
                int expected = i + gN * j + rank * ln;
                if ((int) localB[i * gM + j] != expected) {
                    if (errs < MAX_ERRORS)
                        printf("Found %d but expected %d\n", (int) localB[i * gM + j], expected);
                    errs++;
                }
            }
        }
    }

    /* Free storage */
    free(localA);
    free(localB);

    MTest_Finalize(errs);

    MPI_Finalize();

    return 0;
}
