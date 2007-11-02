/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

static int verbose = 0;

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);
int hindexed_zerotype_test(void);
int hindexed_sparsetype_test(void);

struct test_struct_1 {
    int a,b,c,d;
};

int main(int argc, char *argv[])
{
    int err, errs = 0;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = hindexed_zerotype_test();
    if (verbose && err) fprintf(stderr, "error in hindexed_zerotype_test\n");
    errs += err;

    err = hindexed_sparsetype_test();
    if (verbose && err) fprintf(stderr, "error in hindexed_sparsetype_test\n");
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

/* tests with an hindexed type with all zero length blocks */
int hindexed_zerotype_test(void)
{
    int err, errs = 0;
    int count, elements;
    MPI_Datatype mytype;
    MPI_Request request;
    MPI_Status status;

    int blks[]       = { 0, 0, 0 };
    MPI_Aint disps[] = { 0, 4, 16 };

    err = MPI_Type_hindexed(3, blks, disps, MPI_INT, &mytype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_hindexed returned error\n");
	}
    }

    MPI_Type_commit(&mytype);

    err = MPI_Irecv(NULL, 2, mytype, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Irecv returned error\n");
	}
    }

    err = MPI_Send(NULL, 1, mytype, 0, 0, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Send returned error\n");
	}
    }

    err = MPI_Wait(&request, &status);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Wait returned error\n");
	}
    }

    /* verify count and elements */
    err = MPI_Get_count(&status, mytype, &count);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Get_count returned error\n");
	}
    }
    if (count != 0) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "count = %d; should be 0\n", count);
	}
    }

    err = MPI_Get_elements(&status, mytype, &elements);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Get_elements returned error\n");
	}
    }
    if (elements != 0) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "elements = %d; should be 0\n", elements);
	}
    }

    MPI_Type_free(&mytype);

    return errs;
}

/* tests a short receive into a sparse hindexed type with a zero
 * length block in it.  sort of eccentric, but we've got the basic
 * stuff covered with other tests.
 */
int hindexed_sparsetype_test(void)
{
    int err, errs = 0;
    int i, count, elements;
    MPI_Datatype mytype;
    MPI_Request request;
    MPI_Status status;

    int sendbuf[6]   = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[16];
    int correct[16] = { 1, -2, 4, -4, 2, 3, 5, -8, -9, -10, 6,
			-12, -13, -14, -15, -16 };

    int blks[]       = { 1, 0,             2,             1 };
    MPI_Aint disps[] = { 0, 1*sizeof(int), 4*sizeof(int), 2*sizeof(int) };

    err = MPI_Type_hindexed(4, blks, disps, MPI_INT, &mytype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_hindexed returned error\n");
	}
    }

    MPI_Type_commit(&mytype);

    for (i=0; i < 16; i++) recvbuf[i] = -(i+1);

    err = MPI_Irecv(recvbuf, 2, mytype, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Irecv returned error\n");
	}
    }

    err = MPI_Send(sendbuf, 6, MPI_INT, 0, 0, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Send returned error\n");
	}
    }

    err = MPI_Wait(&request, &status);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Wait returned error\n");
	}
    }
 
    /* verify data */
    for (i=0; i < 16; i++) {
	if (recvbuf[i] != correct[i]) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "recvbuf[%d] = %d; should be %d\n",
			i, recvbuf[i], correct[i]);
	    }
	}
    }

    /* verify count and elements */
    err = MPI_Get_count(&status, mytype, &count);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Get_count returned error\n");
	}
    }
    if (count != MPI_UNDEFINED) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "count = %d; should be MPI_UNDEFINED (%d)\n",
		    count, MPI_UNDEFINED);
	}
    }

    err = MPI_Get_elements(&status, mytype, &elements);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Get_elements returned error\n");
	}
    }
    if (elements != 6) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "elements = %d; should be 6\n", elements);
	}
    }

    MPI_Type_free(&mytype);

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
