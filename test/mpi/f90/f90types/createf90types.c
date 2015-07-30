/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "mpitest.h"

static int cases[6][2] = { {3, 10}, {3, MPI_UNDEFINED}, {MPI_UNDEFINED, 10},
{7, 30}, {7, MPI_UNDEFINED}, {MPI_UNDEFINED, 30}
};

/*
static char MTEST_Descrip[] = "Test the routines to access the Fortran 90 datatypes from C";
*/

#define check_err(fn_name_)                                   \
        do {                                                  \
            if (err) {                                        \
                char errstr[MPI_MAX_ERROR_STRING] = {'\0'};   \
                int resultlen = 0;                            \
                errs++;                                       \
                MPI_Error_string(err, errstr, &resultlen);    \
                printf("err in " #fn_name_ ": %s\n", errstr); \
            }                                                 \
        } while (0)

/* Check the return from the routine */
static int checkType(const char str[], int p, int r, int f90kind, int err, MPI_Datatype dtype)
{
    int errs = 0;
    if (dtype == MPI_DATATYPE_NULL) {
        printf("Unable to find a real type for (p=%d,r=%d) in %s\n", p, r, str);
        errs++;
    }
    if (err) {
        errs++;
        MTestPrintError(err);
    }

    if (!errs) {
        int nints, nadds, ndtypes, combiner;

        /* Check that we got the correct type */
        MPI_Type_get_envelope(dtype, &nints, &nadds, &ndtypes, &combiner);
        if (combiner != f90kind) {
            errs++;
            printf("Wrong combiner type (got %d, should be %d) for %s\n", combiner, f90kind, str);
        }
        else {
            int parms[2];
            MPI_Datatype outtype;
            parms[0] = 0;
            parms[1] = 0;

            if (ndtypes != 0) {
                errs++;
                printf
                    ("Section 8.6 states that the array_of_datatypes entry is empty for the create_f90 types\n");
            }
            MPI_Type_get_contents(dtype, 2, 0, 1, parms, 0, &outtype);
            switch (combiner) {
            case MPI_COMBINER_F90_REAL:
            case MPI_COMBINER_F90_COMPLEX:
                if (nints != 2) {
                    errs++;
                    printf("Returned %d integer values, 2 expected for %s\n", nints, str);
                }
                if (parms[0] != p || parms[1] != r) {
                    errs++;
                    printf("Returned (p=%d,r=%d); expected (p=%d,r=%d) for %s\n",
                           parms[0], parms[1], p, r, str);
                }
                break;
            case MPI_COMBINER_F90_INTEGER:
                if (nints != 1) {
                    errs++;
                    printf("Returned %d integer values, 1 expected for %s\n", nints, str);
                }
                if (parms[0] != p) {
                    errs++;
                    printf("Returned (p=%d); expected (p=%d) for %s\n", parms[0], p, str);
                }
                break;
            default:
                errs++;
                printf("Unrecognized combiner for %s\n", str);
                break;
            }

        }
    }

    if (!errs) {
        char buf0[64];          /* big enough to hold any single type */
        char buf1[64];          /* big enough to hold any single type */
        MPI_Request req[2];
        int dt_size = 0;

        /* check that we can actually use the type for communication,
         * regression for tt#1028 */
        err = MPI_Type_size(dtype, &dt_size);
        check_err(MPI_Type_size);
        assert(dt_size <= sizeof(buf0));
        memset(buf0, 0, sizeof(buf0));
        memset(buf1, 0, sizeof(buf1));
        if (!errs) {
            err = MPI_Isend(&buf0, 1, dtype, 0, 42, MPI_COMM_SELF, &req[0]);
            check_err(MPI_Isend);
        }
        if (!errs) {
            err = MPI_Irecv(&buf1, 1, dtype, 0, 42, MPI_COMM_SELF, &req[1]);
            check_err(MPI_Irecv);
        }
        if (!errs) {
            err = MPI_Waitall(2, req, MPI_STATUSES_IGNORE);
            check_err(MPI_Waitall);
        }
    }

    return errs;
}

int main(int argc, char *argv[])
{
    int p, r;
    int errs = 0;
    int err;
    int i, j, nLoop = 1;
    MPI_Datatype newtype;

    MTest_Init(0, 0);

    if (argc > 1) {
        nLoop = atoi(argv[1]);
    }
    /* Set the handler to errors return, since according to the
     * standard, it is invalid to provide p and/or r that are unsupported */

    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    for (i = 0; i < nLoop; i++) {

        /* These should be a valid type similar to MPI_REAL and MPI_REAL8 */
        for (j = 0; j < 6; j++) {
            p = cases[j][0];
            r = cases[j][1];
            err = MPI_Type_create_f90_real(p, r, &newtype);
            errs += checkType("REAL", p, r, MPI_COMBINER_F90_REAL, err, newtype);
        }

        /* These should be a valid type similar to MPI_COMPLEX and MPI_COMPLEX8 */
        for (j = 0; j < 6; j++) {
            p = cases[j][0];
            r = cases[j][1];
            err = MPI_Type_create_f90_complex(p, r, &newtype);
            errs += checkType("COMPLEX", p, r, MPI_COMBINER_F90_COMPLEX, err, newtype);
        }

        /* This should be a valid type similar to MPI_INTEGER */
        p = 3;
        r = 10;
        err = MPI_Type_create_f90_integer(p, &newtype);
        errs += checkType("INTEGER", p, r, MPI_COMBINER_F90_INTEGER, err, newtype);
    }

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
