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
int single_struct_test(void);
int array_of_structs_test(void);
int struct_of_structs_test(void);

struct test_struct_1 {
    int a,b;
    char c,d;
    int e;
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

    err = single_struct_test();
    if (verbose && err) fprintf(stderr, "error in single_struct_test\n");
    errs += err;

    err = array_of_structs_test();
    if (verbose && err) fprintf(stderr, "error in array_of_structs_test\n");
    errs += err;

    err = struct_of_structs_test();
    if (verbose && err) fprintf(stderr, "error in struct_of_structs_test\n");
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

int single_struct_test(void)
{
    int err, errs = 0;
    int bufsize, position = 0;
    struct test_struct_1 ts1, ts2;
    MPI_Datatype mystruct;
    char *buffer;

    MPI_Aint disps[3] = {0, 2*sizeof(int), 3*sizeof(int)}; /* guessing... */
    int blks[3] = { 2, 2, 1 };
    MPI_Datatype types[3] = { MPI_INT, MPI_CHAR, MPI_INT };

    ts1.a = 1;
    ts1.b = 2;
    ts1.c = 3;
    ts1.d = 4;
    ts1.e = 5;

    err = MPI_Type_struct(3, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_struct returned error\n");
	}
    }

    MPI_Type_commit(&mystruct);

    MPI_Pack_size(1, mystruct, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc(bufsize);

    err = MPI_Pack(&ts1,
		   1,
		   mystruct,
		   buffer,
		   bufsize,
		   &position,
		   MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Pack returned error\n");
	}
    }

    position = 0;
    err = MPI_Unpack(buffer,
		     bufsize,
		     &position,
		     &ts2,
		     1,
		     mystruct,
		     MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Unpack returned error\n");
	}
    }

    MPI_Type_free(&mystruct);
    free(buffer);

    if (ts1.a != ts2.a) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.a = %d; should be %d\n", ts2.a, ts1.a);
	}
    }
    if (ts1.b != ts2.b) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.b = %d; should be %d\n", ts2.b, ts1.b);
	}
    }
    if (ts1.c != ts2.c) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.c = %d; should be %d\n",
		    (int) ts2.c, (int) ts1.c);
	}
    }
    if (ts1.d != ts2.d) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.d = %d; should be %d\n",
		    (int) ts2.d, (int) ts1.d);
	}
    }
    if (ts1.e != ts2.e) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "ts2.e = %d; should be %d\n", ts2.e, ts1.e);
	}
    }

    return errs;
}

int array_of_structs_test(void)
{
    int i, err, errs = 0;
    int bufsize, position = 0;
    struct test_struct_1 ts1[10], ts2[10];
    MPI_Datatype mystruct;
    char *buffer;

    MPI_Aint disps[3] = {0, 2*sizeof(int), 3*sizeof(int)}; /* guessing... */
    int blks[3] = { 2, 2, 1 };
    MPI_Datatype types[3] = { MPI_INT, MPI_CHAR, MPI_INT };

    for (i=0; i < 10; i++) {
	ts1[i].a = 10*i + 1;
	ts1[i].b = 10*i + 2;
	ts1[i].c = 10*i + 3;
	ts1[i].d = 10*i + 4;
	ts1[i].e = 10*i + 5;

	ts2[i].a = -13;
	ts2[i].b = -13;
	ts2[i].c = -13;
	ts2[i].d = -13;
	ts2[i].e = -13;
    }

    err = MPI_Type_struct(3, blks, disps, types, &mystruct);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Type_struct returned error\n");
	}
    }

    MPI_Type_commit(&mystruct);

    MPI_Pack_size(10, mystruct, MPI_COMM_WORLD, &bufsize);
    buffer = (char *) malloc(bufsize);

    err = MPI_Pack(ts1,
		   10,
		   mystruct,
		   buffer,
		   bufsize,
		   &position,
		   MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Pack returned error\n");
	}
    }

    position = 0;
    err = MPI_Unpack(buffer,
		     bufsize,
		     &position,
		     ts2,
		     10,
		     mystruct,
		     MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "MPI_Unpack returned error\n");
	}
    }

    MPI_Type_free(&mystruct);
    free(buffer);

    for (i=0; i < 10; i++) {
	if (ts1[i].a != ts2[i].a) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].a = %d; should be %d\n",
			i, ts2[i].a, ts1[i].a);
	    }
	}
	if (ts1[i].b != ts2[i].b) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].b = %d; should be %d\n",
			i, ts2[i].b, ts1[i].b);
	    }
	}
	if (ts1[i].c != ts2[i].c) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].c = %d; should be %d\n",
			i, (int) ts2[i].c, (int) ts1[i].c);
	    }
	}
	if (ts1[i].d != ts2[i].d) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].d = %d; should be %d\n",
			i, (int) ts2[i].d, (int) ts1[i].d);
	    }
	}
	if (ts1[i].e != ts2[i].e) {
	    errs++;
	    if (verbose) {
		fprintf(stderr, "ts2[%d].e = %d; should be %d\n",
			i, ts2[i].e, ts1[i].e);
	    }
	}
    }

    return errs;
}

int struct_of_structs_test(void)
{
    int i, j, err, errs = 0, bufsize, position;

    char buf[50], buf2[50], *packbuf;

    MPI_Aint disps[3] = {0, 3, 0};
    int blks[3] = {2, 1, 0};
    MPI_Datatype types[3], chartype, tiletype1, tiletype2, finaltype;

    /* build a contig of one char to try to keep optimizations
     * from being applied.
     */
    err = MPI_Type_contiguous(1, MPI_CHAR, &chartype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "chartype create failed\n");
	}
	return errs;
    }

    /* build a type that we can tile a few times */
    types[0] = MPI_CHAR;
    types[1] = chartype;

    err = MPI_Type_struct(2, blks, disps, types, &tiletype1);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "tiletype1 create failed\n");
	}
	return errs;
    }

    /* build the same type again, again to avoid optimizations */
    err = MPI_Type_struct(2, blks, disps, types, &tiletype2);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "tiletype2 create failed\n");
	}
	return errs;
    }

    /* build a combination of those two tiletypes */
    disps[0] = 0;
    disps[1] = 5;
    disps[2] = 10;
    blks[0]  = 1;
    blks[1]  = 1;
    blks[2]  = 1;
    types[0] = tiletype1;
    types[1] = tiletype2;
    types[2] = MPI_UB;
    err = MPI_Type_struct(3, blks, disps, types, &finaltype);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "finaltype create failed\n");
	}
	return errs;
    }

    MPI_Type_commit(&finaltype);
    MPI_Type_free(&chartype);
    MPI_Type_free(&tiletype1);
    MPI_Type_free(&tiletype2);

    MPI_Pack_size(5, finaltype, MPI_COMM_WORLD, &bufsize);

    packbuf = malloc(bufsize);
    if (packbuf == NULL) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "pack buffer allocation (%d bytes) failed\n", bufsize);
	}
	return errs;
    }

    for (j=0; j < 10; j++) {
	for (i=0; i < 5; i++) {
	    if (i == 2 || i == 4) buf[5*j + i] = 0;
	    else                  buf[5*j + i] = i;
	}
    }

    position = 0;
    err = MPI_Pack(buf, 5, finaltype, packbuf, bufsize, &position, MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "pack failed\n");
	}
	return errs;
    }

    memset(buf2, 0, 50);
    position = 0;
    err = MPI_Unpack(packbuf, bufsize, &position, buf2, 5, finaltype, MPI_COMM_WORLD);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr, "unpack failed\n");
	}
	return errs;
    }

    for (j=0; j < 10; j++) {
	for (i=0; i < 5; i++) {
	    if (buf[5*j + i] != buf2[5*j + i]) {
		errs++;
		if (verbose) {
		    fprintf(stderr,
			    "buf2[%d] = %d; should be %d\n",
			    5*j + i,
			    (int) buf2[5*j+i],
			    (int) buf[5*j+i]);
		}
	    }
	}
    }

    free(packbuf);
    MPI_Type_free(&finaltype);
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
