/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int verbose = 0;

/* tests */
int double_int_test(void);

/* helper functions */
int parse_args(int argc, char **argv);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv); /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    /* perform some tests */
    err = double_int_test();
    if (err && verbose) fprintf(stderr, "%d errors in double_int test.\n",
				err);
    errs += err;

    /* print message and exit */
    if (errs) {
	fprintf(stderr, "Found %d errors\n", errs);
    }
    else {
	printf(" No Errors\n");
    }
    MPI_Finalize();
    return 0;
}

/* send a { double, int, double} tuple and receive as a pair of
 * MPI_DOUBLE_INTs. this should (a) be valid, and (b) result in an
 * element count of 3.
 */
int double_int_test(void)
{
    int err, errs = 0, count;

    struct { double a; int b; double c; } foo;
    struct { double a; int b; double c; int d; } bar;

    int blks[3] = { 1, 1, 1 };
    MPI_Aint disps[3] = { 0, 0, 0 };
    MPI_Datatype types[3] = { MPI_DOUBLE, MPI_INT, MPI_DOUBLE };
    MPI_Datatype stype;

    MPI_Status recvstatus;

    /* fill in disps[1..2] with appropriate offset */
    disps[1] = (MPI_Aint) ((char *) &foo.b - (char *) &foo.a);
    disps[2] = (MPI_Aint) ((char *) &foo.c - (char *) &foo.a);
   
    MPI_Type_create_struct(3, blks, disps, types, &stype);
    MPI_Type_commit(&stype);

    err = MPI_Sendrecv(&foo, 1, stype, 0, 0,
		       &bar, 2, MPI_DOUBLE_INT, 0, 0,
		       MPI_COMM_SELF, &recvstatus);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "MPI_Sendrecv returned error (%d)\n",
			     err);
	return errs;
    }

    err = MPI_Get_elements(&recvstatus, MPI_DOUBLE_INT, &count);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, "MPI_Get_elements returned error (%d)\n",
			     err);
    }

    if (count != 3) {
	errs++;
	if (verbose) fprintf(stderr,
			     "MPI_Get_elements returned count of %d, should be 3\n",
			     count);
    }

    MPI_Type_free( &stype );

    return errs;
}

int parse_args(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
	verbose = 1;
    return 0;
}
