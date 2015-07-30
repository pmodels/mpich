/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_MIN operations on optional datatypes dupported by MPICH";
*/

/*
 * This test looks at the handling of char and types that  are not required
 * integers (e.g., long long).  MPICH allows
 * these as well.  A strict MPI test should not include this test.
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size;
    MPI_Comm comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg(10, "Reduce of MPI_CHAR\n");
    cinbuf[0] = 1;
    cinbuf[1] = 0;
    cinbuf[2] = (rank & 0x7f);

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    MPI_Reduce(cinbuf, coutbuf, 3, MPI_CHAR, MPI_MIN, 0, comm);
    if (rank == 0) {
        if (coutbuf[0] != 1) {
            errs++;
            fprintf(stderr, "char MIN(1) test failed\n");
        }
        if (coutbuf[1] != 0) {
            errs++;
            fprintf(stderr, "char MIN(0) test failed\n");
        }
        if (coutbuf[2] != 0) {
            errs++;
            fprintf(stderr, "char MIN(>) test failed\n");
        }
    }
#endif /* USE_STRICT_MPI */

    /* signed char */
    MTestPrintfMsg(10, "Reduce of MPI_SIGNED_CHAR\n");
    scinbuf[0] = 1;
    scinbuf[1] = 0;
    scinbuf[2] = (rank & 0x7f);

    scoutbuf[0] = 0;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    MPI_Reduce(scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_MIN, 0, comm);
    if (rank == 0) {
        if (scoutbuf[0] != 1) {
            errs++;
            fprintf(stderr, "signed char MIN(1) test failed\n");
        }
        if (scoutbuf[1] != 0) {
            errs++;
            fprintf(stderr, "signed char MIN(0) test failed\n");
        }
        if (scoutbuf[2] != 0) {
            errs++;
            fprintf(stderr, "signed char MIN(>) test failed\n");
        }
    }

    /* unsigned char */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_CHAR\n");
    ucinbuf[0] = 1;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank & 0x7f);

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    MPI_Reduce(ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_MIN, 0, comm);
    if (rank == 0) {
        if (ucoutbuf[0] != 1) {
            errs++;
            fprintf(stderr, "unsigned char MIN(1) test failed\n");
        }
        if (ucoutbuf[1]) {
            errs++;
            fprintf(stderr, "unsigned char MIN(0) test failed\n");
        }
        if (ucoutbuf[2] != 0) {
            errs++;
            fprintf(stderr, "unsigned char MIN(>) test failed\n");
        }
    }

#ifdef HAVE_LONG_DOUBLE
    {
        long double ldinbuf[3], ldoutbuf[3];
        /* long double */
        ldinbuf[0] = 1;
        ldinbuf[1] = 0;
        ldinbuf[2] = rank;

        ldoutbuf[0] = 0;
        ldoutbuf[1] = 1;
        ldoutbuf[2] = 1;
        if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
            MTestPrintfMsg(10, "Reduce of MPI_LONG_DOUBLE\n");
            MPI_Reduce(ldinbuf, ldoutbuf, 3, MPI_LONG_DOUBLE, MPI_MIN, 0, comm);
            if (rank == 0) {
                if (ldoutbuf[0] != 1) {
                    errs++;
                    fprintf(stderr, "long double MIN(1) test failed\n");
                }
                if (ldoutbuf[1] != 0.0) {
                    errs++;
                    fprintf(stderr, "long double MIN(0) test failed\n");
                }
                if (ldoutbuf[2] != 0.0) {
                    errs++;
                    fprintf(stderr, "long double MIN(>) test failed\n");
                }
            }
        }
    }
#endif /* HAVE_LONG_DOUBLE */

#ifdef HAVE_LONG_LONG
    {
        long long llinbuf[3], lloutbuf[3];
        /* long long */
        llinbuf[0] = 1;
        llinbuf[1] = 0;
        llinbuf[2] = rank;

        lloutbuf[0] = 0;
        lloutbuf[1] = 1;
        lloutbuf[2] = 1;
        if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
            MTestPrintfMsg(10, "Reduce of MPI_LONG_LONG\n");
            MPI_Reduce(llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_MIN, 0, comm);
            if (rank == 0) {
                if (lloutbuf[0] != 1) {
                    errs++;
                    fprintf(stderr, "long long MIN(1) test failed\n");
                }
                if (lloutbuf[1] != 0) {
                    errs++;
                    fprintf(stderr, "long long MIN(0) test failed\n");
                }
                if (lloutbuf[2] != 0) {
                    errs++;
                    fprintf(stderr, "long long MIN(>) test failed\n");
                }
            }
        }
    }
#endif /* HAVE_LONG_LONG */

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
