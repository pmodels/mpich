/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

int main(int argc, char *argv[]);

/* helper functions */
int parse_args(int argc, char **argv);

static int verbose = 0;

int main(int argc, char *argv[])
{
    /* Variable declarations */
    int a[100][100], b[100][100];
    int disp[100], block[100];
    MPI_Datatype ltype;

    int bufsize, position = 0;
    void *buffer;

    int i, j, errs = 0;

    /* Initialize a to some known values and zero out b. */
    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            a[i][j] = 1000 * i + j;
            b[i][j] = 0;
        }
    }

    /* Initialize MPI */
    MTest_Init(&argc, &argv);

    parse_args(argc, argv);

    for (i = 0; i < 100; i++) {
        /* Fortran version has disp(i) = 100*(i-1) + i and block(i) = 100-i. */
        /* This code here is wrong. It compacts everything together,
         * which isn't what we want.
         * What we want is to put the lower triangular values into b and leave
         * the rest of it unchanged, right?
         */
        block[i] = i + 1;
        disp[i] = 100 * i;
    }

    /* Create datatype for lower triangular part. */
    MPI_Type_indexed(100, block, disp, MPI_INT, &ltype);
    MPI_Type_commit(&ltype);

    /* Pack it. */
    MPI_Pack_size(1, ltype, MPI_COMM_WORLD, &bufsize);
    buffer = (void *) malloc((unsigned) bufsize);
    MPI_Pack(a, 1, ltype, buffer, bufsize, &position, MPI_COMM_WORLD);

    /* Unpack the buffer into b. */
    position = 0;
    MPI_Unpack(buffer, bufsize, &position, b, 1, ltype, MPI_COMM_WORLD);

    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            if (j > i && b[i][j] != 0) {
                errs++;
                if (verbose)
                    fprintf(stderr, "b[%d][%d] = %d; should be %d\n", i, j, b[i][j], 0);
            }
            else if (j <= i && b[i][j] != 1000 * i + j) {
                errs++;
                if (verbose)
                    fprintf(stderr, "b[%d][%d] = %d; should be %d\n", i, j, b[i][j], 1000 * i + j);
            }
        }
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}

int parse_args(int argc, char **argv)
{
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0) {
        switch (ret) {
        case 'v':
            verbose = 1;
            break;
        }
    }
    return 0;
}
