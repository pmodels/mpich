/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

static int verbose = 0;

/* tests */
int builtin_float_test(void);
int vector_of_vectors_test(void);
int optimizable_vector_of_basics_test(void);

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
    err = builtin_float_test();
    if (err && verbose) fprintf(stderr, "%d errors in builtin float test.\n", 
				err);
    errs += err;

    err = vector_of_vectors_test();
    if (err && verbose) fprintf(stderr, 
				"%d errors in vector of vectors test.\n", err);
    errs += err;

    err = optimizable_vector_of_basics_test();
    if (err && verbose) fprintf(stderr, 
				"%d errors in vector of basics test.\n", err);
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

/* builtin_float_test()
 *
 * Tests functionality of get_envelope() and get_contents() on a MPI_FLOAT.
 *
 * Returns the number of errors encountered.
 */
int builtin_float_test(void)
{
    int nints, nadds, ntypes, combiner;

    int err, errs = 0;

    err = MPI_Type_get_envelope(MPI_FLOAT,
				&nints,
				&nadds,
				&ntypes,
				&combiner);
    
    if (combiner != MPI_COMBINER_NAMED) errs++;

    /* Note: it is erroneous to call MPI_Type_get_contents() on a basic. */
    return errs;
}

/* vector_of_vectors_test()
 *
 * Builds a vector of a vector of ints.  Assuming an int array of size 9 
 * integers, and treating the array as a 3x3 2D array, this will grab the 
 * corners.
 *
 * Returns the number of errors encountered.
 */
int vector_of_vectors_test(void)
{
    MPI_Datatype inner_vector;
    MPI_Datatype outer_vector;
    int array[9] = {  1, -1,  2,
		     -2, -3, -4,
		      3, -5,  4 };

    char *buf;
    int i, err, errs = 0, sizeoftype, position;

    /* set up type */
    err = MPI_Type_vector(2,
			  1,
			  2,
			  MPI_INT,
			  &inner_vector);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, 
			     "error in MPI call; aborting after %d errors\n",
			     errs+1);
	return errs;
    }

    err = MPI_Type_vector(2,
			  1,
			  2,
			  inner_vector,
			  &outer_vector);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) fprintf(stderr, 
			     "error in MPI call; aborting after %d errors\n",
			     errs+1);
	return errs;
    }

    MPI_Type_commit(&outer_vector);
    MPI_Type_size(outer_vector, &sizeoftype);
    if (sizeoftype != 4*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     (int) sizeoftype, (int) (4*sizeof(int)));
	return errs;
    }

    buf = (char *) malloc(sizeoftype);

    position = 0;
    err = MPI_Pack(array,
		   1,
		   outer_vector,
		   buf,
		   sizeoftype,
		   &position,
		   MPI_COMM_WORLD);

    if (position != sizeoftype) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (pack)\n",
			     position, sizeoftype);
    }

    memset(array, 0, 9*sizeof(int));
    position = 0;
    err = MPI_Unpack(buf,
		     sizeoftype,
		     &position,
		     array,
		     1,
		     outer_vector,
		     MPI_COMM_WORLD);

    if (position != sizeoftype) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (unpack)\n",
			     position, sizeoftype);
    }

    for (i=0; i < 9; i++) {
	int goodval;
	switch (i) {
	    case 0:
		goodval = 1;
		break;
	    case 2:
		goodval = 2;
		break;
	    case 6:
		goodval = 3;
		break;
	    case 8:
		goodval = 4;
		break;
	    default:
		goodval = 0;
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&inner_vector);
    MPI_Type_free(&outer_vector);
    return errs;
}

/* optimizable_vector_of_basics_test()
 *
 * Builds a vector of ints.  Count is 10, blocksize is 2, stride is 2, so this
 * is equivalent to a contig of 20.
 *
 * Returns the number of errors encountered.
 */
int optimizable_vector_of_basics_test(void)
{
    MPI_Datatype parent_type;
    int array[20] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		      16, 17, 18, 19 };
    char *buf;
    int i, sizeofint, sizeoftype, position;

    int err, errs = 0;

    MPI_Type_size(MPI_INT, &sizeofint);

    if (sizeofint != sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of MPI_Int = %d; should be %d\n",
			     sizeofint, (int) sizeof(int));
    }

    /* set up type */
    err = MPI_Type_vector(10,
			  2,
			  2,
			  MPI_INT,
			  &parent_type);

    MPI_Type_commit(&parent_type);

    MPI_Type_size(parent_type, &sizeoftype);

    if (sizeoftype != 20 * sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of vector = %d; should be %d\n",
			     (int) sizeoftype, (int) (20 * sizeof(int)));
    }

    buf = (char *) malloc(sizeoftype);

    position = 0;
    err = MPI_Pack(array,
		   1,
		   parent_type,
		   buf,
		   sizeoftype,
		   &position,
		   MPI_COMM_WORLD);

    if (position != sizeoftype) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (pack)\n",
			     position, sizeoftype);
    }

    memset(array, 0, 20 * sizeof(int));
    position = 0;
    err = MPI_Unpack(buf,
		     sizeoftype,
		     &position,
		     array,
		     1,
		     parent_type,
		     MPI_COMM_WORLD);

    if (position != sizeoftype) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (unpack)\n",
			     position, sizeoftype);
    }

    for (i=0; i < 20; i++) {
	if (array[i] != i) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], i);
	}
    }

    MPI_Type_free(&parent_type);
    return errs;
}


int parse_args(int argc, char **argv)
{
    /*
    int ret;

    while ((ret = getopt(argc, argv, "v")) >= 0)
    {
	switch (ret) {
	    case 'v':
		verbose = 1;
		break;
	}
    }
    */
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
	verbose = 1;
    return 0;
}

