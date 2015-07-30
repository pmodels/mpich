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
static char MTEST_Descrip[] = "Test MPI_BOR operations on optional datatypes dupported by MPICH";
*/

/*
 * This test looks at the handling of logical and for types that are not
 * integers or are not required integers (e.g., long long).  MPICH allows
 * these as well.  A strict MPI test should not include this test.
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int rc;
    int rank, size;
    MPI_Comm comm;
    char cinbuf[3], coutbuf[3];
    signed char scinbuf[3], scoutbuf[3];
    unsigned char ucinbuf[3], ucoutbuf[3];
    short sinbuf[3], soutbuf[3];
    unsigned short usinbuf[3], usoutbuf[3];
    long linbuf[3], loutbuf[3];
    unsigned long ulinbuf[3], uloutbuf[3];
    unsigned uinbuf[3], uoutbuf[3];
    int iinbuf[3], ioutbuf[3];


    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;
    /* Set errors return so that we can provide better information
     * should a routine reject one of the operand/datatype pairs */
    MPI_Errhandler_set(comm, MPI_ERRORS_RETURN);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

#ifndef USE_STRICT_MPI
    /* char */
    MTestPrintfMsg(10, "Reduce of MPI_CHAR\n");
    cinbuf[0] = 0xff;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    rc = MPI_Reduce(cinbuf, coutbuf, 3, MPI_CHAR, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_CHAR", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (coutbuf[0] != (char) 0xff) {
                errs++;
                fprintf(stderr, "char BOR(1) test failed\n");
            }
            if (coutbuf[1]) {
                errs++;
                fprintf(stderr, "char BOR(0) test failed\n");
            }
            if (coutbuf[2] != (char) 0xff && size > 1) {
                errs++;
                fprintf(stderr, "char BOR(>) test failed\n");
            }
        }
    }
#endif /* USE_STRICT_MPI */

    /* signed char */
    MTestPrintfMsg(10, "Reduce of MPI_SIGNED_CHAR\n");
    scinbuf[0] = 0xff;
    scinbuf[1] = 0;
    scinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    scoutbuf[0] = 0;
    scoutbuf[1] = 1;
    scoutbuf[2] = 1;
    rc = MPI_Reduce(scinbuf, scoutbuf, 3, MPI_SIGNED_CHAR, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_SIGNED_CHAR", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (scoutbuf[0] != (signed char) 0xff) {
                errs++;
                fprintf(stderr, "signed char BOR(1) test failed\n");
            }
            if (scoutbuf[1]) {
                errs++;
                fprintf(stderr, "signed char BOR(0) test failed\n");
            }
            if (scoutbuf[2] != (signed char) 0xff && size > 1) {
                errs++;
                fprintf(stderr, "signed char BOR(>) test failed\n");
            }
        }
    }

    /* unsigned char */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_CHAR\n");
    ucinbuf[0] = 0xff;
    ucinbuf[1] = 0;
    ucinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    ucoutbuf[0] = 0;
    ucoutbuf[1] = 1;
    ucoutbuf[2] = 1;
    rc = MPI_Reduce(ucinbuf, ucoutbuf, 3, MPI_UNSIGNED_CHAR, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_UNSIGNED_CHAR", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (ucoutbuf[0] != 0xff) {
                errs++;
                fprintf(stderr, "unsigned char BOR(1) test failed\n");
            }
            if (ucoutbuf[1]) {
                errs++;
                fprintf(stderr, "unsigned char BOR(0) test failed\n");
            }
            if (ucoutbuf[2] != 0xff && size > 1) {
                errs++;
                fprintf(stderr, "unsigned char BOR(>) test failed\n");
            }
        }
    }

    /* bytes */
    MTestPrintfMsg(10, "Reduce of MPI_BYTE\n");
    cinbuf[0] = 0xff;
    cinbuf[1] = 0;
    cinbuf[2] = (rank > 0) ? 0x3c : 0xc3;

    coutbuf[0] = 0;
    coutbuf[1] = 1;
    coutbuf[2] = 1;
    rc = MPI_Reduce(cinbuf, coutbuf, 3, MPI_BYTE, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_BYTE", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (coutbuf[0] != (char) 0xff) {
                errs++;
                fprintf(stderr, "byte BOR(1) test failed\n");
            }
            if (coutbuf[1]) {
                errs++;
                fprintf(stderr, "byte BOR(0) test failed\n");
            }
            if (coutbuf[2] != (char) 0xff && size > 1) {
                errs++;
                fprintf(stderr, "byte BOR(>) test failed\n");
            }
        }
    }

    /* short */
    MTestPrintfMsg(10, "Reduce of MPI_SHORT\n");
    sinbuf[0] = 0xffff;
    sinbuf[1] = 0;
    sinbuf[2] = (rank > 0) ? 0x3c3c : 0xc3c3;

    soutbuf[0] = 0;
    soutbuf[1] = 1;
    soutbuf[2] = 1;
    rc = MPI_Reduce(sinbuf, soutbuf, 3, MPI_SHORT, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_SHORT", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (soutbuf[0] != (short) 0xffff) {
                errs++;
                fprintf(stderr, "short BOR(1) test failed\n");
            }
            if (soutbuf[1]) {
                errs++;
                fprintf(stderr, "short BOR(0) test failed\n");
            }
            if (soutbuf[2] != (short) 0xffff && size > 1) {
                errs++;
                fprintf(stderr, "short BOR(>) test failed\n");
            }
        }
    }

    /* unsigned short */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_SHORT\n");
    usinbuf[0] = 0xffff;
    usinbuf[1] = 0;
    usinbuf[2] = (rank > 0) ? 0x3c3c : 0xc3c3;

    usoutbuf[0] = 0;
    usoutbuf[1] = 1;
    usoutbuf[2] = 1;
    rc = MPI_Reduce(usinbuf, usoutbuf, 3, MPI_UNSIGNED_SHORT, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_UNSIGNED_SHORT", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (usoutbuf[0] != 0xffff) {
                errs++;
                fprintf(stderr, "short BOR(1) test failed\n");
            }
            if (usoutbuf[1]) {
                errs++;
                fprintf(stderr, "short BOR(0) test failed\n");
            }
            if (usoutbuf[2] != 0xffff && size > 1) {
                errs++;
                fprintf(stderr, "short BOR(>) test failed\n");
            }
        }
    }

    /* unsigned */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED\n");
    uinbuf[0] = 0xffffffff;
    uinbuf[1] = 0;
    uinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    uoutbuf[0] = 0;
    uoutbuf[1] = 1;
    uoutbuf[2] = 1;
    rc = MPI_Reduce(uinbuf, uoutbuf, 3, MPI_UNSIGNED, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_UNSIGNED", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (uoutbuf[0] != 0xffffffff) {
                errs++;
                fprintf(stderr, "unsigned BOR(1) test failed\n");
            }
            if (uoutbuf[1]) {
                errs++;
                fprintf(stderr, "unsigned BOR(0) test failed\n");
            }
            if (uoutbuf[2] != 0xffffffff && size > 1) {
                errs++;
                fprintf(stderr, "unsigned BOR(>) test failed\n");
            }
        }
    }

    /* int */
    MTestPrintfMsg(10, "Reduce of MPI_INT\n");
    iinbuf[0] = 0xffffffff;
    iinbuf[1] = 0;
    iinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    ioutbuf[0] = 0;
    ioutbuf[1] = 1;
    ioutbuf[2] = 1;
    rc = MPI_Reduce(iinbuf, ioutbuf, 3, MPI_INT, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_INT", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (ioutbuf[0] != 0xffffffff) {
                errs++;
                fprintf(stderr, "int BOR(1) test failed\n");
            }
            if (ioutbuf[1]) {
                errs++;
                fprintf(stderr, "int BOR(0) test failed\n");
            }
            if (ioutbuf[2] != 0xffffffff && size > 1) {
                errs++;
                fprintf(stderr, "int BOR(>) test failed\n");
            }
        }
    }

    /* long */
    MTestPrintfMsg(10, "Reduce of MPI_LONG\n");
    linbuf[0] = 0xffffffff;
    linbuf[1] = 0;
    linbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    loutbuf[0] = 0;
    loutbuf[1] = 1;
    loutbuf[2] = 1;
    rc = MPI_Reduce(linbuf, loutbuf, 3, MPI_LONG, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_LONG", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (loutbuf[0] != 0xffffffff) {
                errs++;
                fprintf(stderr, "long BOR(1) test failed\n");
            }
            if (loutbuf[1]) {
                errs++;
                fprintf(stderr, "long BOR(0) test failed\n");
            }
            if (loutbuf[2] != 0xffffffff && size > 1) {
                errs++;
                fprintf(stderr, "long BOR(>) test failed\n");
            }
        }
    }

    /* unsigned long */
    MTestPrintfMsg(10, "Reduce of MPI_UNSIGNED_LONG\n");
    ulinbuf[0] = 0xffffffff;
    ulinbuf[1] = 0;
    ulinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

    uloutbuf[0] = 0;
    uloutbuf[1] = 1;
    uloutbuf[2] = 1;
    rc = MPI_Reduce(ulinbuf, uloutbuf, 3, MPI_UNSIGNED_LONG, MPI_BOR, 0, comm);
    if (rc) {
        MTestPrintErrorMsg("MPI_BOR and MPI_UNSIGNED_LONG", rc);
        errs++;
    }
    else {
        if (rank == 0) {
            if (uloutbuf[0] != 0xffffffff) {
                errs++;
                fprintf(stderr, "unsigned long BOR(1) test failed\n");
            }
            if (uloutbuf[1]) {
                errs++;
                fprintf(stderr, "unsigned long BOR(0) test failed\n");
            }
            if (uloutbuf[2] != 0xffffffff && size > 1) {
                errs++;
                fprintf(stderr, "unsigned long BOR(>) test failed\n");
            }
        }
    }

#ifdef HAVE_LONG_LONG
    {
        long long llinbuf[3], lloutbuf[3];
        /* long long */
        llinbuf[0] = 0xffffffff;
        llinbuf[1] = 0;
        llinbuf[2] = (rank > 0) ? 0x3c3c3c3c : 0xc3c3c3c3;

        lloutbuf[0] = 0;
        lloutbuf[1] = 1;
        lloutbuf[2] = 1;
        if (MPI_LONG_LONG != MPI_DATATYPE_NULL) {
            MTestPrintfMsg(10, "Reduce of MPI_LONG_LONG\n");
            rc = MPI_Reduce(llinbuf, lloutbuf, 3, MPI_LONG_LONG, MPI_BOR, 0, comm);
            if (rc) {
                MTestPrintErrorMsg("MPI_BOR and MPI_LONG_LONG", rc);
                errs++;
            }
            else {
                if (rank == 0) {
                    if (lloutbuf[0] != 0xffffffff) {
                        errs++;
                        fprintf(stderr, "long long BOR(1) test failed\n");
                    }
                    if (lloutbuf[1]) {
                        errs++;
                        fprintf(stderr, "long long BOR(0) test failed\n");
                    }
                    if (lloutbuf[2] != 0xffffffff && size > 1) {
                        errs++;
                        fprintf(stderr, "long long BOR(>) test failed\n");
                    }
                }
            }
        }
    }
#endif

    MPI_Errhandler_set(comm, MPI_ERRORS_ARE_FATAL);
    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
