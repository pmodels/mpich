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
int derived_resized_test(void);

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
    err = derived_resized_test();
    if (err && verbose) fprintf(stderr, "%d errors in derived_resized test.\n",
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

/* derived_resized_test()
 *
 * Tests behavior with resizing of a simple derived type.
 *
 * Returns the number of errors encountered.
 */
int derived_resized_test(void)
{
    int err, errs = 0;

    int count = 2;
    MPI_Datatype newtype, resizedtype;

    int size;
    MPI_Aint extent;

    err = MPI_Type_contiguous(count,
			     MPI_INT,
			     &newtype);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating type in derived_resized_test()\n");
	}
	errs++;
    }

    err = MPI_Type_create_resized(newtype,
				  (MPI_Aint) 0,
				  (MPI_Aint) (2*sizeof(int) + 10),
				  &resizedtype);

    err = MPI_Type_size(resizedtype, &size);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error obtaining type size in derived_resized_test()\n");
	}
	errs++;
    }
    
    if (size != 2*sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "error: size != %d in derived_resized_test()\n", (int) (2*sizeof(int)));
	}
	errs++;
    }    

    err = MPI_Type_extent(resizedtype, &extent);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error obtaining type extent in derived_resized_test()\n");
	}
	errs++;
    }
    
    if (extent != 2*sizeof(int) + 10) {
	if (verbose) {
	    fprintf(stderr,
		    "error: invalid extent (%d) in derived_resized_test(); should be %d\n",
		    (int) extent,
		    (int) (2*sizeof(int) + 10));
	}
	errs++;
    }    

    MPI_Type_free( &newtype );
    MPI_Type_free( &resizedtype );

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

