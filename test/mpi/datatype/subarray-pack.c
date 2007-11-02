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
int subarray_1d_c_test1(void);
int subarray_1d_fortran_test1(void);
int subarray_2d_c_test1(void);
int subarray_4d_c_test1(void);
int subarray_2d_c_test2(void);
int subarray_2d_fortran_test1(void);
int subarray_4d_fortran_test1(void);

/* helper functions */
static int parse_args(int argc, char **argv);
static int pack_and_unpack(char *typebuf,
			   int count,
			   MPI_Datatype datatype,
			   int typebufsz);

int main(int argc, char **argv)
{
    int err, errs = 0;

    MPI_Init(&argc, &argv); /* MPI-1.2 doesn't allow for MPI_Init(0,0) */
    parse_args(argc, argv);

    /* To improve reporting of problems about operations, we
       change the error handler to errors return */
    MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN );

    /* perform some tests */
    err = subarray_1d_c_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 1d subarray c test 1.\n", err);
    errs += err;

    err = subarray_1d_fortran_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 1d subarray fortran test 1.\n",
				err);
    errs += err;

    err = subarray_2d_c_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray c test 1.\n", err);
    errs += err;

    err = subarray_2d_fortran_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray fortran test 1.\n",
				err);
    errs += err;

    err = subarray_2d_c_test2();
    if (err && verbose) fprintf(stderr,
				"%d errors in 2d subarray c test 2.\n", err);
    errs += err;

    err = subarray_4d_c_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 4d subarray c test 1.\n", err);
    errs += err;

    err = subarray_4d_fortran_test1();
    if (err && verbose) fprintf(stderr,
				"%d errors in 4d subarray fortran test 1.\n", err);
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

/* subarray_1d_c_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_1d_c_test1(void)
{
    MPI_Datatype subarray;
    int array[9] = { -1, 1, 2, 3, -2, -3, -4, -5, -6 };
    int array_size[] = {9};
    int array_subsize[] = {3};
    int array_start[] = {1};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(1, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 3 * sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (3 * sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 9 * sizeof(int));

    for (i=0; i < 9; i++) {
	int goodval;
	switch (i) {
	    case 1:
		goodval = 1;
		break;
	    case 2:
		goodval = 2;
		break;
	    case 3:
		goodval = 3;
		break;
	    default:
		goodval = 0; /* pack_and_unpack() zeros before unpacking */
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&subarray);
    return errs;
}

/* subarray_1d_fortran_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_1d_fortran_test1(void)
{
    MPI_Datatype subarray;
    int array[9] = { -1, 1, 2, 3, -2, -3, -4, -5, -6 };
    int array_size[] = {9};
    int array_subsize[] = {3};
    int array_start[] = {1};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(1, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_FORTRAN,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 3 * sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (3 * sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 9 * sizeof(int));

    for (i=0; i < 9; i++) {
	int goodval;
	switch (i) {
	    case 1:
		goodval = 1;
		break;
	    case 2:
		goodval = 2;
		break;
	    case 3:
		goodval = 3;
		break;
	    default:
		goodval = 0; /* pack_and_unpack() zeros before unpacking */
		break;
	}
	if (array[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "array[%d] = %d; should be %d\n",
				 i, array[i], goodval);
	}
    }

    MPI_Type_free(&subarray);
    return errs;
}


/* subarray_2d_test()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_c_test1(void)
{
    MPI_Datatype subarray;
    int array[9] = { -1, -2, -3,
		     -4,  1,  2,
		     -5,  3,  4 };
    int array_size[2] = {3, 3};
    int array_subsize[2] = {2, 2};
    int array_start[2] = {1, 1};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 4*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (4*sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 9*sizeof(int));

    for (i=0; i < 9; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
		break;
	    case 7:
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

    MPI_Type_free(&subarray);
    return errs;
}

/* subarray_2d_c_test2()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_c_test2(void)
{
    MPI_Datatype subarray;
    int array[12] = { -1, -2, -3, -4,  1,   2,
		      -5, -6, -7, -8, -9, -10 };
    int array_size[2] = {2, 6};
    int array_subsize[2] = {1, 2};
    int array_start[2] = {0, 4};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 2*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (2*sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 12*sizeof(int));

    for (i=0; i < 12; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
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

    MPI_Type_free(&subarray);
    return errs;
}

/* subarray_4d_c_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_4d_c_test1(void)
{
    MPI_Datatype subarray;
    int array[] = {
	-1111, -1112, -1113, -1114, -1115, -1116,
	-1121, -1122, -1123, -1124, -1125, -1126,
	-1131, -1132, -1133, -1134, -1135, -1136,
	-1211, -1212, -1213, -1214, -1215, -1216,
	-1221, -1222, -1223, -1224, -1225, -1226,
	-1231, -1232, -1233, -1234, -1235, -1236,
	-2111, -2112, -2113, -2114,     1, -2116,
	-2121, -2122, -2123, -2124,     2, -2126,
	-2131, -2132, -2133, -2134,     3, -2136,
	-2211, -2212, -2213, -2214,     4, -2216,
	-2221, -2222, -2223, -2224,     5, -2226,
	-2231, -2232, -2233, -2234,     6, -2236
    };
    
    int array_size[4] = {2, 2, 3, 6};
    int array_subsize[4] = {1, 2, 3, 1};
    int array_start[4] = {1, 0, 0, 4};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(4, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_C,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 6*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (6*sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 72*sizeof(int));

    for (i=0; i < 72; i++) {
	int goodval;
	switch (i) {
	    case 40:
		goodval = 1;
		break;
	    case 46:
		goodval = 2;
		break;
	    case 52:
		goodval = 3;
		break;
	    case 58:
		goodval = 4;
		break;
	    case 64:
		goodval = 5;
		break;
	    case 70:
		goodval = 6;
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

    MPI_Type_free(&subarray);
    return errs;
}
/* subarray_4d_fortran_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_4d_fortran_test1(void)
{
    MPI_Datatype subarray;
    int array[] = {
	-1111, -1112, -1113, -1114, -1115, -1116,
	-1121, -1122, -1123, -1124, -1125, -1126,
	-1131, -1132, -1133, -1134, -1135, -1136,
	-1211, -1212, -1213, -1214, -1215, -1216,
	-1221, -1222, -1223, -1224, -1225, -1226,
	-1231, -1232, -1233, -1234, -1235, -1236,
	-2111, -2112, -2113, -2114,     1, -2116,
	-2121, -2122, -2123, -2124,     2, -2126,
	-2131, -2132, -2133, -2134,     3, -2136,
	-2211, -2212, -2213, -2214,     4, -2216,
	-2221, -2222, -2223, -2224,     5, -2226,
	-2231, -2232, -2233, -2234,     6, -2236
    };
    
    int array_size[4] = {6, 3, 2, 2};
    int array_subsize[4] = {1, 3, 2, 1};
    int array_start[4] = {4, 0, 0, 1};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(4, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_FORTRAN,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 6*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (6*sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 72*sizeof(int));

    for (i=0; i < 72; i++) {
	int goodval;
	switch (i) {
	    case 40:
		goodval = 1;
		break;
	    case 46:
		goodval = 2;
		break;
	    case 52:
		goodval = 3;
		break;
	    case 58:
		goodval = 4;
		break;
	    case 64:
		goodval = 5;
		break;
	    case 70:
		goodval = 6;
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

    MPI_Type_free(&subarray);
    return errs;
}


/* subarray_2d_fortran_test1()
 *
 * Returns the number of errors encountered.
 */
int subarray_2d_fortran_test1(void)
{
    MPI_Datatype subarray;
    int array[12] = { -1, -2, -3, -4,  1,   2,
		      -5, -6, -7, -8, -9, -10 };
    int array_size[2] = {6, 2};
    int array_subsize[2] = {2, 1};
    int array_start[2] = {4, 0};

    int i, err, errs = 0, sizeoftype;

    /* set up type */
    err = MPI_Type_create_subarray(2, /* dims */
				   array_size,
				   array_subsize,
				   array_start,
				   MPI_ORDER_FORTRAN,
				   MPI_INT,
				   &subarray);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_create_subarray call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    MPI_Type_commit(&subarray);
    MPI_Type_size(subarray, &sizeoftype);
    if (sizeoftype != 2*sizeof(int)) {
	errs++;
	if (verbose) fprintf(stderr, "size of type = %d; should be %d\n",
			     sizeoftype, (int) (2*sizeof(int)));
	return errs;
    }

    err = pack_and_unpack((char *) array, 1, subarray, 12*sizeof(int));

    for (i=0; i < 12; i++) {
	int goodval;
	switch (i) {
	    case 4:
		goodval = 1;
		break;
	    case 5:
		goodval = 2;
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

    MPI_Type_free(&subarray);
    return errs;
}

/******************************************************************/

/* pack_and_unpack()
 *
 * Perform packing and unpacking of a buffer for the purposes of checking
 * to see if we are processing a type correctly.  Zeros the buffer between
 * these two operations, so the data described by the type should be in
 * place upon return but all other regions of the buffer should be zero.
 *
 * Parameters:
 * typebuf - pointer to buffer described by datatype and count that
 *           will be packed and then unpacked into
 * count, datatype - description of typebuf
 * typebufsz - size of typebuf; used specifically to zero the buffer
 *             between the pack and unpack steps
 *
 */
static int pack_and_unpack(char *typebuf,
			   int count,
			   MPI_Datatype datatype,
			   int typebufsz)
{
    char *packbuf;
    int err, errs = 0, pack_size, type_size, position;

    err = MPI_Type_size(datatype, &type_size);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Type_size call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    type_size *= count;

    err = MPI_Pack_size(count, datatype, MPI_COMM_SELF, &pack_size);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Pack_size call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }
    packbuf = (char *) malloc(pack_size);
    if (packbuf == NULL) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in malloc call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }

    position = 0;
    err = MPI_Pack(typebuf,
		   count,
		   datatype,
		   packbuf,
		   type_size,
		   &position,
		   MPI_COMM_SELF);

    if (position != type_size) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (pack)\n",
			     position, type_size);
    }

    memset(typebuf, 0, typebufsz);
    position = 0;
    err = MPI_Unpack(packbuf,
		     type_size,
		     &position,
		     typebuf,
		     count,
		     datatype,
		     MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
	errs++;
	if (verbose) {
	    fprintf(stderr,
		    "error in MPI_Unpack call; aborting after %d errors\n",
		    errs);
	}
	return errs;
    }
    free(packbuf);

    if (position != type_size) {
	errs++;
	if (verbose) fprintf(stderr, "position = %d; should be %d (unpack)\n",
			     position, type_size);
    }

    return errs;
}

static int parse_args(int argc, char **argv)
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

