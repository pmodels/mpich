/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "mpi.h"

static int verbose = 0;

int main(int argc, char *argv[]);
int parse_args(int argc, char **argv);

int main(int argc, char *argv[])
{
    int i, err, errs = 0, rank, toterrs;

    int         index;
    MPI_Request requests[10];
    MPI_Status  statuses[10];

    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    for (i=0; i < 10; i++) {
	requests[i] = MPI_REQUEST_NULL;
    }

    /* begin testing */
    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = MPI_Waitany(10, requests, &index, statuses);

    if (err != MPI_SUCCESS) {
	errs++;
	fprintf(stderr, "MPI_Waitany did not return MPI_SUCCESS\n");
    }

    if (index != MPI_UNDEFINED) {
	errs++;
	fprintf(stderr, "MPI_Waitany did not set index to MPI_UNDEFINED\n");
    }

    /* end testing */
    
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL );
    MPI_Comm_rank( MPI_COMM_WORLD, & rank );
    MPI_Allreduce( &errs, &toterrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
    if (rank == 0) {
	if (toterrs) {
	    fprintf(stderr, " Found %d errors\n", toterrs);
	}
	else {
	    printf(" No Errors\n");
	}
    }
    MPI_Finalize();
    return 0;
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
