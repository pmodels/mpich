/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include "mpitestconf.h"
#include <stdio.h>
#include <string.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test MPI_MAXLOC operations on datatypes dupported by MPICH";
*/

/*
 * This test looks at the handling of char and types that  are not required
 * integers (e.g., long long).  MPICH allows
 * these as well.  A strict MPI test should not include this test.
 *
 * The rule on max loc is that if there is a tie in the value, the minimum
 * rank is used (see 4.9.3 in the MPI-1 standard)
 */
int main(int argc, char *argv[])
{
    int errs = 0;
    int rank, size;
    MPI_Comm comm;

    MTest_Init(&argc, &argv);

    comm = MPI_COMM_WORLD;

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    /* 2 int */
    {
        struct twoint {
            int val;
            int loc;
        } cinbuf[3], coutbuf[3];

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        MPI_Reduce(cinbuf, coutbuf, 3, MPI_2INT, MPI_MAXLOC, 0, comm);
        if (rank == 0) {
            if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                errs++;
                fprintf(stderr, "2int MAXLOC(1) test failed\n");
            }
            if (coutbuf[1].val != 0) {
                errs++;
                fprintf(stderr, "2int MAXLOC(0) test failed, value = %d, should be zero\n",
                        coutbuf[1].val);
            }
            if (coutbuf[1].loc != 0) {
                errs++;
                fprintf(stderr,
                        "2int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                        coutbuf[1].loc);
            }
            if (coutbuf[2].val != size - 1 || coutbuf[2].loc != size - 1) {
                errs++;
                fprintf(stderr, "2int MAXLOC(>) test failed\n");
            }
        }
    }

    /* float int */
    {
        struct floatint {
            float val;
            int loc;
        } cinbuf[3], coutbuf[3];

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = (float) rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        MPI_Reduce(cinbuf, coutbuf, 3, MPI_FLOAT_INT, MPI_MAXLOC, 0, comm);
        if (rank == 0) {
            if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                errs++;
                fprintf(stderr, "float-int MAXLOC(1) test failed\n");
            }
            if (coutbuf[1].val != 0) {
                errs++;
                fprintf(stderr, "float-int MAXLOC(0) test failed, value = %f, should be zero\n",
                        coutbuf[1].val);
            }
            if (coutbuf[1].loc != 0) {
                errs++;
                fprintf(stderr,
                        "float-int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                        coutbuf[1].loc);
            }
            if (coutbuf[2].val != size - 1 || coutbuf[2].loc != size - 1) {
                errs++;
                fprintf(stderr, "float-int MAXLOC(>) test failed\n");
            }
        }
    }

    /* long int */
    {
        struct longint {
            long val;
            int loc;
        } cinbuf[3], coutbuf[3];

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        MPI_Reduce(cinbuf, coutbuf, 3, MPI_LONG_INT, MPI_MAXLOC, 0, comm);
        if (rank == 0) {
            if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                errs++;
                fprintf(stderr, "long-int MAXLOC(1) test failed\n");
            }
            if (coutbuf[1].val != 0) {
                errs++;
                fprintf(stderr, "long-int MAXLOC(0) test failed, value = %ld, should be zero\n",
                        coutbuf[1].val);
            }
            if (coutbuf[1].loc != 0) {
                errs++;
                fprintf(stderr,
                        "long-int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                        coutbuf[1].loc);
            }
            if (coutbuf[2].val != size - 1 || coutbuf[2].loc != size - 1) {
                errs++;
                fprintf(stderr, "long-int MAXLOC(>) test failed\n");
            }
        }
    }

    /* short int */
    {
        struct shortint {
            short val;
            int loc;
        } cinbuf[3], coutbuf[3];

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        MPI_Reduce(cinbuf, coutbuf, 3, MPI_SHORT_INT, MPI_MAXLOC, 0, comm);
        if (rank == 0) {
            if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                errs++;
                fprintf(stderr, "short-int MAXLOC(1) test failed\n");
            }
            if (coutbuf[1].val != 0) {
                errs++;
                fprintf(stderr, "short-int MAXLOC(0) test failed, value = %d, should be zero\n",
                        coutbuf[1].val);
            }
            if (coutbuf[1].loc != 0) {
                errs++;
                fprintf(stderr,
                        "short-int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                        coutbuf[1].loc);
            }
            if (coutbuf[2].val != size - 1) {
                errs++;
                fprintf(stderr, "short-int MAXLOC(>) test failed, value = %d, should be %d\n",
                        coutbuf[2].val, size - 1);
            }
            if (coutbuf[2].loc != size - 1) {
                errs++;
                fprintf(stderr,
                        "short-int MAXLOC(>) test failed, location of max = %d, should be %d\n",
                        coutbuf[2].loc, size - 1);
            }
        }
    }

    /* double int */
    {
        struct doubleint {
            double val;
            int loc;
        } cinbuf[3], coutbuf[3];

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        MPI_Reduce(cinbuf, coutbuf, 3, MPI_DOUBLE_INT, MPI_MAXLOC, 0, comm);
        if (rank == 0) {
            if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                errs++;
                fprintf(stderr, "double-int MAXLOC(1) test failed\n");
            }
            if (coutbuf[1].val != 0) {
                errs++;
                fprintf(stderr, "double-int MAXLOC(0) test failed, value = %lf, should be zero\n",
                        coutbuf[1].val);
            }
            if (coutbuf[1].loc != 0) {
                errs++;
                fprintf(stderr,
                        "double-int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                        coutbuf[1].loc);
            }
            if (coutbuf[2].val != size - 1 || coutbuf[2].loc != size - 1) {
                errs++;
                fprintf(stderr, "double-int MAXLOC(>) test failed\n");
            }
        }
    }

#ifdef HAVE_LONG_DOUBLE
    /* long double int */
    {
        struct longdoubleint {
            long double val;
            int loc;
        } cinbuf[3], coutbuf[3];

        /* avoid valgrind warnings about padding bytes in the long double */
        memset(&cinbuf[0], 0, sizeof(cinbuf));
        memset(&coutbuf[0], 0, sizeof(coutbuf));

        cinbuf[0].val = 1;
        cinbuf[0].loc = rank;
        cinbuf[1].val = 0;
        cinbuf[1].loc = rank;
        cinbuf[2].val = rank;
        cinbuf[2].loc = rank;

        coutbuf[0].val = 0;
        coutbuf[0].loc = -1;
        coutbuf[1].val = 1;
        coutbuf[1].loc = -1;
        coutbuf[2].val = 1;
        coutbuf[2].loc = -1;
        if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL) {
            MPI_Reduce(cinbuf, coutbuf, 3, MPI_LONG_DOUBLE_INT, MPI_MAXLOC, 0, comm);
            if (rank == 0) {
                if (coutbuf[0].val != 1 || coutbuf[0].loc != 0) {
                    errs++;
                    fprintf(stderr, "long double-int MAXLOC(1) test failed\n");
                }
                if (coutbuf[1].val != 0) {
                    errs++;
                    fprintf(stderr,
                            "long double-int MAXLOC(0) test failed, value = %lf, should be zero\n",
                            (double) coutbuf[1].val);
                }
                if (coutbuf[1].loc != 0) {
                    errs++;
                    fprintf(stderr,
                            "long double-int MAXLOC(0) test failed, location of max = %d, should be zero\n",
                            coutbuf[1].loc);
                }
                if (coutbuf[2].val != size - 1) {
                    errs++;
                    fprintf(stderr,
                            "long double-int MAXLOC(>) test failed, value = %lf, should be %d\n",
                            (double) coutbuf[2].val, size - 1);
                }
                if (coutbuf[2].loc != size - 1) {
                    errs++;
                    fprintf(stderr,
                            "long double-int MAXLOC(>) test failed, location of max = %d, should be %d\n",
                            coutbuf[2].loc, size - 1);
                }
            }
        }
    }
#endif

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
