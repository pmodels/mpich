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
int indexed_contig_test(void);
int indexed_zeroblock_first_test(void);
int indexed_zeroblock_middle_test(void);
int indexed_zeroblock_last_test(void);

/* helper functions */
int parse_args(int argc, char **argv);
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
    err = indexed_contig_test();
    if (err && verbose) fprintf(stderr,
				"%d errors in indexed_contig_test.\n",
				err);
    errs += err;

    err = indexed_zeroblock_first_test();
    if (err && verbose) fprintf(stderr,
				"%d errors in indexed_zeroblock_first_test.\n",
				err);
    errs += err;

    err = indexed_zeroblock_middle_test();
    if (err && verbose) fprintf(stderr,
				"%d errors in indexed_zeroblock_middle_test.\n",
				err);
    errs += err;

    err = indexed_zeroblock_last_test();
    if (err && verbose) fprintf(stderr,
				"%d errors in indexed_zeroblock_last_test.\n",
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

int indexed_zeroblock_first_test(void)
{
    int err, errs = 0;

    MPI_Datatype type;
    int len[3]  = { 0, 1, 1 };
    int disp[3] = { 0, 1, 4 };
    MPI_Aint lb, ub;

    err = MPI_Type_indexed(3, len, disp, MPI_INT, &type);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating indexed type in indexed_zeroblock_first_test()\n");
	}
	errs += 1;
    }

    MPI_Type_lb(type, &lb);
    if (lb != sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "lb mismatch; is %d, should be %d\n",
		    (int) lb, (int) sizeof(int));
	}
	errs++;
    }
    MPI_Type_ub(type, &ub);
    if (ub != 5 * sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "ub mismatch; is %d, should be %d\n",
		    (int) ub, (int) (5 * sizeof(int)));
	}
	errs++;
    }
    
    MPI_Type_free( &type );
    
    return errs;
}

int indexed_zeroblock_middle_test(void)
{
    int err, errs = 0;

    MPI_Datatype type;
    int len[3]  = { 1, 0, 1 };
    int disp[3] = { 1, 2, 4 };
    MPI_Aint lb, ub;

    err = MPI_Type_indexed(3, len, disp, MPI_INT, &type);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating indexed type in indexed_zeroblock_middle_test()\n");
	}
	errs += 1;
    }

    MPI_Type_lb(type, &lb);
    if (lb != sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "lb mismatch; is %d, should be %d\n",
		    (int) lb, (int) sizeof(int));
	}
	errs++;
    }
    MPI_Type_ub(type, &ub);
    if (ub != 5 * sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "ub mismatch; is %d, should be %d\n",
		    (int) ub, (int) (5 * sizeof(int)));
	}
	errs++;
    }

    MPI_Type_free( &type );
    
    return errs;
}

int indexed_zeroblock_last_test(void)
{
    int err, errs = 0;

    MPI_Datatype type;
    int len[3]  = { 1, 1, 0 };
    int disp[3] = { 1, 4, 8 };
    MPI_Aint lb, ub;

    err = MPI_Type_indexed(3, len, disp, MPI_INT, &type);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating indexed type in indexed_zeroblock_last_test()\n");
	}
	errs += 1;
    }

    MPI_Type_lb(type, &lb);
    if (lb != sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "lb mismatch; is %d, should be %d\n",
		    (int) lb, (int) sizeof(int));
	}
	errs++;
    }
    MPI_Type_ub(type, &ub);
    if (ub != 5 * sizeof(int)) {
	if (verbose) {
	    fprintf(stderr,
		    "ub mismatch; is %d, should be %d\n",
		    (int) ub, (int) (5 * sizeof(int)));
	}
	errs++;
    }

    MPI_Type_free( &type );
    
    return errs;
}

/* indexed_contig_test()
 *
 * Tests behavior with an indexed array that can be compacted but should
 * continue to be stored as an indexed type.  Specifically for coverage.
 *
 * Returns the number of errors encountered.
 */
int indexed_contig_test(void)
{
    int buf[9] = {-1, 1, 2, 3, -2, 4, 5, -3, 6};
    int err, errs = 0;

    int i, count = 5;
    int blklen[]    = { 1, 2, 1, 1, 1 };
    int disp[] = { 1, 2, 5, 6, 8 };
    MPI_Datatype newtype;

    int size, int_size;

    err = MPI_Type_indexed(count,
			   blklen,
			   disp,
			   MPI_INT,
			   &newtype);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error creating indexed type in indexed_contig_test()\n");
	}
	errs++;
    }

    MPI_Type_size(MPI_INT, &int_size);

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
	if (verbose) {
	    fprintf(stderr,
		    "error obtaining type size in indexed_contig_test()\n");
	}
	errs++;
    }
    
    if (size != 6 * int_size) {
	if (verbose) {
	    fprintf(stderr,
		    "error: size != 6 * int_size in indexed_contig_test()\n");
	}
	errs++;
    }    

    MPI_Type_commit(&newtype);

    err = pack_and_unpack((char *) buf, 1, newtype, 9 * sizeof(int));
    if (err != 0) {
	if (verbose) {
	    fprintf(stderr,
		    "error packing/unpacking in indexed_contig_test()\n");
	}
	errs += err;
    }

    for (i=0; i < 9; i++) {
	int goodval;

	switch(i) {
	    case 1:
		goodval = 1;
		break;
	    case 2:
		goodval = 2;
		break;
	    case 3:
		goodval = 3;
		break;
	    case 5:
		goodval = 4;
		break;
	    case 6:
		goodval = 5;
		break;
	    case 8:
		goodval = 6;
		break;
	    default:
		goodval = 0; /* pack_and_unpack() zeros before unpack */
		break;
	}
	if (buf[i] != goodval) {
	    errs++;
	    if (verbose) fprintf(stderr, "buf[%d] = %d; should be %d\n",
				 i, buf[i], goodval);
	}
    }

    MPI_Type_free( &newtype );

    return errs;
}

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

