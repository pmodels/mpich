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
static char MTEST_Descrip[] = "Test MPI_PROD operations on optional datatypes dupported by MPICH";
*/

typedef struct {
    double r, i;
} d_complex;
#ifdef HAVE_LONG_DOUBLE
typedef struct {
    long double r, i;
} ld_complex;
#endif

/*
 * This test looks at the handling of logical and for types that are not
 * integers or are not required integers (e.g., long long).  MPICH allows
 * these as well.  A strict MPI test should not include this test.
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size, maxsize, result[6] = { 1, 1, 2, 6, 24, 120 };
    MPI_Comm comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];
    d_complex dinbuf[3], doutbuf[3];

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    if (size > 5)
        maxsize = 5;
    else
        maxsize = size;

    /* General forumula: If we multiple the values from 1 to n, the
     * product is n!.  This grows very fast, so we'll only use the first
     * five (1! = 1, 2! = 2, 3! = 6, 4! = 24, 5! = 120), with n!
     * stored in the array result[n] */

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg(10, "Reduce of MPI_CHAR\n");
    cinbuf[0] = (rank < maxsize && rank > 0) ? rank : 1;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 1);

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    MPI_Reduce(cinbuf, coutbuf, 3, MPI_CHAR, MPI_PROD, 0, comm);
    if (rank == 0) {
        if (coutbuf[0] != (char) result[maxsize - 1]) {
            errs++;
            fprintf(stderr, "char PROD(rank) test failed (%d!=%d)\n",
                    (int) coutbuf[0], (int) result[maxsize]);
        }
        if (coutbuf[1]) {
            errs++;
            fprintf(stderr, "char PROD(0) test failed\n");
        }
        if (size > 1 && coutbuf[2]) {
            errs++;
            fprintf(stderr, "char PROD(>) test failed\n");
        }
    }
#endif /* USE_STRICT_MPI */

    /* signed char */
    MTestPrintfMsg(10, "Reduce of MPI_SIGNED_CHAR\n");
    scinbuf[0] = (rank < maxsize && rank > 0) ? rank : 1;
    scinbuf[1] = 0;
    scinbuf[2] = (rank > 1);

    scoutbuf[0] = 0;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    MPI_Reduce(scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_PROD, 0, comm);
    if (rank == 0) {
        if (scoutbuf[0] != (signed char) result[maxsize - 1]) {
            errs++;
            fprintf(stderr, "signed char PROD(rank) test failed (%d!=%d)\n",
                    (int) scoutbuf[0], (int) result[maxsize]);
        }
        if (scoutbuf[1]) {
            errs++;
            fprintf(stderr, "signed char PROD(0) test failed\n");
        }
        if (size > 1 && scoutbuf[2]) {
            errs++;
            fprintf(stderr, "signed char PROD(>) test failed\n");
        }
    }

    /* unsigned char */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_CHAR\n");
    ucinbuf[0] = (rank < maxsize && rank > 0) ? rank : 1;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank > 0);

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    MPI_Reduce(ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_PROD, 0, comm);
    if (rank == 0) {
        if (ucoutbuf[0] != (unsigned char) result[maxsize - 1]) {
            errs++;
            fprintf(stderr, "unsigned char PROD(rank) test failed\n");
        }
        if (ucoutbuf[1]) {
            errs++;
            fprintf(stderr, "unsigned char PROD(0) test failed\n");
        }
        if (size > 1 && ucoutbuf[2]) {
            errs++;
            fprintf(stderr, "unsigned char PROD(>) test failed\n");
        }
    }

#ifndef USE_STRICT_MPI
    /* For some reason, complex is not allowed for sum and prod */
    if (MPI_DOUBLE_COMPLEX != MPI_DATATYPE_NULL) {
        int dc;
#ifdef HAVE_LONG_DOUBLE
        ld_complex ldinbuf[3], ldoutbuf[3];
#endif
        /* Must determine which C type matches this Fortran type */
        MPI_Type_size(MPI_DOUBLE_COMPLEX, &dc);
        if (dc == sizeof(d_complex)) {
            /* double complex; may be null if we do not have Fortran support */
            dinbuf[0].r = (rank < maxsize && rank > 0) ? rank : 1;
            dinbuf[1].r = 0;
            dinbuf[2].r = (rank > 0);
            dinbuf[0].i = 0;
            dinbuf[1].i = 1;
            dinbuf[2].i = -(rank > 0);

            doutbuf[0].r = 0;
            doutbuf[1].r = 1;
            doutbuf[2].r = 1;
            doutbuf[0].i = 0;
            doutbuf[1].i = 1;
            doutbuf[2].i = 1;
            MPI_Reduce(dinbuf, doutbuf, 3, MPI_DOUBLE_COMPLEX, MPI_PROD, 0, comm);
            if (rank == 0) {
                double imag, real;
                if (doutbuf[0].r != (double) result[maxsize - 1] || doutbuf[0].i != 0) {
                    errs++;
                    fprintf(stderr, "double complex PROD(rank) test failed\n");
                }
                /* Multiplying the imaginary part depends on size mod 4 */
                imag = 1.0;
                real = 0.0;     /* Make compiler happy */
                switch (size % 4) {
                case 1:
                    imag = 1.0;
                    real = 0.0;
                    break;
                case 2:
                    imag = 0.0;
                    real = -1.0;
                    break;
                case 3:
                    imag = -1.0;
                    real = 0.0;
                    break;
                case 0:
                    imag = 0.0;
                    real = 1.0;
                    break;
                }
                if (doutbuf[1].r != real || doutbuf[1].i != imag) {
                    errs++;
                    fprintf(stderr, "double complex PROD(i) test failed (%f,%f)!=(%f,%f)\n",
                            doutbuf[1].r, doutbuf[1].i, real, imag);
                }
                if (doutbuf[2].r != 0 || doutbuf[2].i != 0) {
                    errs++;
                    fprintf(stderr, "double complex PROD(>) test failed\n");
                }
            }
        }
#ifdef HAVE_LONG_DOUBLE
        else if (dc == sizeof(ld_complex)) {
            /* double complex; may be null if we do not have Fortran support */
            ldinbuf[0].r = (rank < maxsize && rank > 0) ? rank : 1;
            ldinbuf[1].r = 0;
            ldinbuf[2].r = (rank > 0);
            ldinbuf[0].i = 0;
            ldinbuf[1].i = 1;
            ldinbuf[2].i = -(rank > 0);

            ldoutbuf[0].r = 0;
            ldoutbuf[1].r = 1;
            ldoutbuf[2].r = 1;
            ldoutbuf[0].i = 0;
            ldoutbuf[1].i = 1;
            ldoutbuf[2].i = 1;
            MPI_Reduce(ldinbuf, ldoutbuf, 3, MPI_DOUBLE_COMPLEX, MPI_PROD, 0, comm);
            if (rank == 0) {
                long double imag, real;
                if (ldoutbuf[0].r != (double) result[maxsize - 1] || ldoutbuf[0].i != 0) {
                    errs++;
                    fprintf(stderr, "double complex PROD(rank) test failed\n");
                }
                /* Multiplying the imaginary part depends on size mod 4 */
                imag = 1.0;
                real = 0.0;     /* Make compiler happy */
                switch (size % 4) {
                case 1:
                    imag = 1.0;
                    real = 0.0;
                    break;
                case 2:
                    imag = 0.0;
                    real = -1.0;
                    break;
                case 3:
                    imag = -1.0;
                    real = 0.0;
                    break;
                case 0:
                    imag = 0.0;
                    real = 1.0;
                    break;
                }
                if (ldoutbuf[1].r != real || ldoutbuf[1].i != imag) {
                    errs++;
                    fprintf(stderr, "double complex PROD(i) test failed (%Lf,%Lf)!=(%Lf,%Lf)\n",
                            ldoutbuf[1].r, ldoutbuf[1].i, real, imag);
                }
                if (ldoutbuf[2].r != 0 || ldoutbuf[2].i != 0) {
                    errs++;
                    fprintf(stderr, "double complex PROD(>) test failed\n");
                }
            }
        }
#endif /* HAVE_LONG_DOUBLE */
    }
#endif /* USE_STRICT_MPI */

#ifdef HAVE_LONG_DOUBLE
    {
        long double ldinbuf[3], ldoutbuf[3];
        /* long double */
        ldinbuf[0] = (rank < maxsize && rank > 0) ? rank : 1;
        ldinbuf[1] = 0;
        ldinbuf[2] = (rank > 0);

        ldoutbuf[0] = 0;
        ldoutbuf[1] = 1;
        ldoutbuf[2] = 1;
        if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
            MPI_Reduce(ldinbuf, ldoutbuf, 3, MPI_LONG_DOUBLE, MPI_PROD, 0, comm);
            if (rank == 0) {
                if (ldoutbuf[0] != (long double) result[maxsize - 1]) {
                    errs++;
                    fprintf(stderr, "long double PROD(rank) test failed\n");
                }
                if (ldoutbuf[1]) {
                    errs++;
                    fprintf(stderr, "long double PROD(0) test failed\n");
                }
                if (size > 1 && ldoutbuf[2] != 0) {
                    errs++;
                    fprintf(stderr, "long double PROD(>) test failed\n");
                }
            }
        }
    }
#endif /* HAVE_LONG_DOUBLE */

#ifdef HAVE_LONG_LONG
    {
        long long llinbuf[3], lloutbuf[3];
        /* long long */
        llinbuf[0] = (rank < maxsize && rank > 0) ? rank : 1;
        llinbuf[1] = 0;
        llinbuf[2] = (rank > 0);

        lloutbuf[0] = 0;
        lloutbuf[1] = 1;
        lloutbuf[2] = 1;
        if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
            MPI_Reduce(llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_PROD, 0, comm);
            if (rank == 0) {
                if (lloutbuf[0] != (long long) result[maxsize - 1]) {
                    errs++;
                    fprintf(stderr, "long long PROD(rank) test failed\n");
                }
                if (lloutbuf[1]) {
                    errs++;
                    fprintf(stderr, "long long PROD(0) test failed\n");
                }
                if (size > 1 && lloutbuf[2]) {
                    errs++;
                    fprintf(stderr, "long long PROD(>) test failed\n");
                }
            }
        }
    }
#endif /* HAVE_LONG_LONG */

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
