/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_LOR operations on optional datatypes dupported by MPICH";
*/

/*
 * This test looks at the handling of logical and for types that are not
 * integers or are not required integers (e.g., long long).  MPICH allows
 * these as well.  A strict MPI test should not include this test.
 */
int main(int argc, char *argv[])
{
    int errs = 0, err;
    int rank, size;
    MPI_Comm comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* Some MPI implementations do not implement all of the required
     * (datatype,operations) combinations, and further, they do not
     * always provide clear and specific error messages.  By catching
     * the error, we can provide a higher quality, more specific message.
     */
    MPI_Comm_set_errhandler(comm, MPI_ERRORS_RETURN);

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg(10, "Reduce of MPI_CHAR\n");
    cinbuf[0] = 1;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0);

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    err = MPI_Reduce(cinbuf, coutbuf, 3, MPI_CHAR, MPI_LOR, 0, comm);
    if (err) {
        errs++;
        MTestPrintErrorMsg("MPI_LOR and MPI_CHAR", err);
    } else {
        if (rank == 0) {
            if (!coutbuf[0]) {
                errs++;
                fprintf(stderr, "char OR(1) test failed\n");
            }
            if (coutbuf[1]) {
                errs++;
                fprintf(stderr, "char OR(0) test failed\n");
            }
            if (!coutbuf[2] && size > 1) {
                errs++;
                fprintf(stderr, "char OR(>) test failed\n");
            }
        }
    }
#endif /* USE_STRICT_MPI */

    /* signed char */
    MTestPrintfMsg(10, "Reduce of MPI_SIGNED_CHAR\n");
    scinbuf[0] = 1;
    scinbuf[1] = 0;
    scinbuf[2] = (rank > 0);

    scoutbuf[0] = 0;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    err = MPI_Reduce(scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_LOR, 0, comm);
    if (err) {
        errs++;
        MTestPrintErrorMsg("MPI_LOR and MPI_SIGNED_CHAR", err);
    } else {
        if (rank == 0) {
            if (!scoutbuf[0]) {
                errs++;
                fprintf(stderr, "signed char OR(1) test failed\n");
            }
            if (scoutbuf[1]) {
                errs++;
                fprintf(stderr, "signed char OR(0) test failed\n");
            }
            if (!scoutbuf[2] && size > 1) {
                errs++;
                fprintf(stderr, "signed char OR(>) test failed\n");
            }
        }
    }

    /* unsigned char */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_CHAR\n");
    ucinbuf[0] = 1;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank > 0);

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    err = MPI_Reduce(ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_LOR, 0, comm);
    if (err) {
        errs++;
        MTestPrintErrorMsg("MPI_LOR and MPI_UNSIGNED_CHAR", err);
    } else {
        if (rank == 0) {
            if (!ucoutbuf[0]) {
                errs++;
                fprintf(stderr, "unsigned char OR(1) test failed\n");
            }
            if (ucoutbuf[1]) {
                errs++;
                fprintf(stderr, "unsigned char OR(0) test failed\n");
            }
            if (!ucoutbuf[2] && size > 1) {
                errs++;
                fprintf(stderr, "unsigned char OR(>) test failed\n");
            }
        }
    }

#ifdef HAVE_LONG_LONG
    {
        long long llinbuf[3], lloutbuf[3];
        /* long long */
        llinbuf[0] = 1;
        llinbuf[1] = 0;
        llinbuf[2] = (rank > 0);

        lloutbuf[0] = 0;
        lloutbuf[1] = 1;
        lloutbuf[2] = 1;
        if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
            MTestPrintfMsg(10, "Reduce of MPI_LONG_LONG\n");
            err = MPI_Reduce(llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_LOR, 0, comm);
            if (err) {
                errs++;
                MTestPrintErrorMsg("MPI_LOR and MPI_LONG_LONG", err);
            } else {
                if (rank == 0) {
                    if (!lloutbuf[0]) {
                        errs++;
                        fprintf(stderr, "long long OR(1) test failed\n");
                    }
                    if (lloutbuf[1]) {
                        errs++;
                        fprintf(stderr, "long long OR(0) test failed\n");
                    }
                    if (!lloutbuf[2] && size > 1) {
                        errs++;
                        fprintf(stderr, "long long OR(>) test failed\n");
                    }
                }
            }
        }
    }
#endif

    MPI_Comm_set_errhandler(comm, MPI_ERRORS_ARE_FATAL);
    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
