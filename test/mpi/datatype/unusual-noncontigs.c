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
int struct_negdisp_test(void);
int vector_negstride_test(void);
int indexed_negdisp_test(void);

int main(int argc, char *argv[])
{
    int err, errs = 0;

    /* Initialize MPI */
    MPI_Init(&argc, &argv);
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    err = struct_negdisp_test();
    if (verbose && err) fprintf(stderr, "error in struct_negdisp_test\n");
    errs += err;

    err = vector_negstride_test();
    if (verbose && err) fprintf(stderr, "error in vector_negstride_test\n");
    errs += err;

    err = indexed_negdisp_test();
    if (verbose && err) fprintf(stderr, "error in indexed_negdisp_test\n");
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

/* test uses a struct type that describes data that is contiguous,
 * but processed in a noncontiguous way.
 */
int struct_negdisp_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype mystruct;
    MPI_Request request;
    MPI_Status status;

    MPI_Aint disps[2]     = { 0,       -1*((int) sizeof(int)) };
    int blks[2]           = { 1,       1, };
    MPI_Datatype types[2] = { MPI_INT, MPI_INT };

    err = MPI_Type_struct(2, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_struct returned error\n");
	}
    }

    MPI_Type_commit(&mystruct);

    err = MPI_Irecv(recvbuf+1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Irecv returned error\n");
	}
    }

    err = MPI_Send(sendbuf+2, 2, mystruct, 0, 0, MPI_COMM_SELF);
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
    if (recvbuf[0] != -1) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
	}
    }
    if (recvbuf[1] != 3) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
	}
    }
    if (recvbuf[2] != 2) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
	}
    }
    if (recvbuf[3] != 5) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
	}
    }
    if (recvbuf[4] != 4) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
	}
    }
    if (recvbuf[5] != -6) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
	}
    }

    MPI_Type_free(&mystruct);

    return errs;
}

/* test uses a vector type that describes data that is contiguous,
 * but processed in a noncontiguous way.  this is effectively the
 * same type as in the struct_negdisp_test above.
 */
int vector_negstride_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype myvector;
    MPI_Request request;
    MPI_Status status;

    err = MPI_Type_vector(2, 1, -1, MPI_INT, &myvector);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_vector returned error\n");
	}
    }

    MPI_Type_commit(&myvector);

    err = MPI_Irecv(recvbuf+1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Irecv returned error\n");
	}
    }

    err = MPI_Send(sendbuf+2, 2, myvector, 0, 0, MPI_COMM_SELF);
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
    if (recvbuf[0] != -1) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
	}
    }
    if (recvbuf[1] != 3) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
	}
    }
    if (recvbuf[2] != 2) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
	}
    }
    if (recvbuf[3] != 5) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
	}
    }
    if (recvbuf[4] != 4) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
	}
    }
    if (recvbuf[5] != -6) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
	}
    }

    MPI_Type_free(&myvector);

    return errs;
}

/* test uses a indexed type that describes data that is contiguous,
 * but processed in a noncontiguous way.  this is effectively the same
 * type as in the two tests above.
 */
int indexed_negdisp_test(void)
{
    int err, errs = 0;
    int sendbuf[6] = { 1, 2, 3, 4, 5, 6 };
    int recvbuf[6] = { -1, -2, -3, -4, -5, -6 };
    MPI_Datatype myindexed;
    MPI_Request request;
    MPI_Status status;

    int disps[2]     = { 0, -1 };
    int blks[2]           = { 1, 1 };

    err = MPI_Type_indexed(2, blks, disps, MPI_INT, &myindexed);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_indexed returned error\n");
	}
    }

    MPI_Type_commit(&myindexed);

    err = MPI_Irecv(recvbuf+1, 4, MPI_INT, 0, 0, MPI_COMM_SELF, &request);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Irecv returned error\n");
	}
    }

    err = MPI_Send(sendbuf+2, 2, myindexed, 0, 0, MPI_COMM_SELF);
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
    if (recvbuf[0] != -1) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[0] = %d; should be %d\n", recvbuf[0], -1);
	}
    }
    if (recvbuf[1] != 3) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[1] = %d; should be %d\n", recvbuf[1], 3);
	}
    }
    if (recvbuf[2] != 2) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[2] = %d; should be %d\n", recvbuf[2], 2);
	}
    }
    if (recvbuf[3] != 5) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[3] = %d; should be %d\n", recvbuf[3], 5);
	}
    }
    if (recvbuf[4] != 4) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[4] = %d; should be %d\n", recvbuf[4], 4);
	}
    }
    if (recvbuf[5] != -6) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "recvbuf[5] = %d; should be %d\n", recvbuf[5], -6);
	}
    }

    MPI_Type_free(&myindexed);

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
