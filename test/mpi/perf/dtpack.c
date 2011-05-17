/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This code may be used to test the performance of some of the 
 * noncontiguous datatype operations, including vector and indexed
 * pack and unpack operations.  To simplify the use of this code for 
 * tuning an MPI implementation, it uses no communication, just the
 * MPI_Pack and MPI_Unpack routines.  In addition, the individual tests are
 * in separate routines, making it easier to compare the compiler-generated
 * code for the user (manual) pack/unpack with the code used by 
 * the MPI implementation.  Further, to be fair to the MPI implementation,
 * the routines are passed the source and destination buffers; this ensures
 * that the compiler can't optimize for statically allocated buffers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

/* Needed for restrict and const definitions */
#include "mpitestconf.h"

static int verbose = 0;

#define N_REPS 1000
#define THRESHOLD 0.10
#define VARIANCE_THRESHOLD ((THRESHOLD * THRESHOLD) / 2)
#define NTRIALS 10

double mean(double *list, int count);
double mean(double *list, int count)
{
	double retval;
	int i;

	retval = 0;
	for (i = 0; i < count; i++)
		retval += list[i];
	retval /= count;

	return retval;
}

double noise(double *list, int count);
double noise(double *list, int count)
{
	double *margin, retval;
	int i;

	if (!(margin = malloc(count * sizeof(double)))) {
		printf("Unable to allocate memory\n");
		return -1;
	}

	for (i = 0; i < count; i++)
		margin[i] = list[i] / mean(list, count);

	retval = 0;
	for (i = 0; i < count; i++) {
		retval += ((margin[i] - 1) * (margin[i] - 1));
	}
	retval /= count;
	if (retval < 0) retval = -retval;

	return retval;
}

/* Here are the tests */

/* Test packing a vector of individual doubles */
/* We don't use restrict in the function args because assignments between
   restrict pointers is not valid in C and some compilers, such as the 
   IBM xlc compilers, flag that use as an error.*/
int TestVecPackDouble( int n, int stride, 
		       double *avgTimeUser, double *avgTimeMPI,
		       double *dest, const double *src );
int TestVecPackDouble( int n, int stride, 
		       double *avgTimeUser, double *avgTimeMPI,
		       double *dest, const double *src )
{
	double *restrict d_dest;
	const double *restrict d_src;
	register int i, j;
	int          rep, position;
	double       t1, t2, t[NTRIALS];
	MPI_Datatype vectype;

	/* User code */
	if (verbose) printf("TestVecPackDouble (USER): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			i = n;
			d_dest = dest;
			d_src  = src;
			while (i--) {
				*d_dest++ = *d_src;
				d_src += stride;
			}
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
		return 0;
	}
	*avgTimeUser = mean(t, NTRIALS) / N_REPS;

	/* MPI Vector code */
	MPI_Type_vector( n, 1, stride, MPI_DOUBLE, &vectype );
	MPI_Type_commit( &vectype );

	if (verbose) printf("TestVecPackDouble (MPI): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			position = 0;
			MPI_Pack( (void *)src, 1, vectype, dest, n*sizeof(double),
				  &position, MPI_COMM_SELF );
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
	}
	else {
	    *avgTimeMPI = mean(t, NTRIALS) / N_REPS;
	}

	MPI_Type_free( &vectype );

	return 0;
}

/* Test unpacking a vector of individual doubles */
/* See above for why restrict is not used in the function args */
int TestVecUnPackDouble( int n, int stride, 
		       double *avgTimeUser, double *avgTimeMPI,
		       double *dest, const double *src );
int TestVecUnPackDouble( int n, int stride, 
		       double *avgTimeUser, double *avgTimeMPI,
		       double *dest, const double *src )
{
	double *restrict d_dest;
	const double *restrict d_src;
	register int i, j;
	int          rep, position;
	double       t1, t2, t[NTRIALS];
	MPI_Datatype vectype;

	/* User code */
	if (verbose) printf("TestVecUnPackDouble (USER): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			i = n;
			d_dest = dest;
			d_src  = src;
			while (i--) {
				*d_dest = *d_src++;
				d_dest += stride;
			}
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
		return 0;
	}
	*avgTimeUser = mean(t, NTRIALS) / N_REPS;
    
	/* MPI Vector code */
	MPI_Type_vector( n, 1, stride, MPI_DOUBLE, &vectype );
	MPI_Type_commit( &vectype );

	if (verbose) printf("TestVecUnPackDouble (MPI): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			position = 0;
			MPI_Unpack( (void *)src, n*sizeof(double), 
				    &position, dest, 1, vectype, MPI_COMM_SELF );
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
	}
	else {
	    *avgTimeMPI = mean(t, NTRIALS) / N_REPS;
	}

	MPI_Type_free( &vectype );

	return 0;
}

/* Test packing a vector of 2-individual doubles */
/* See above for why restrict is not used in the function args */
int TestVecPack2Double( int n, int stride, 
			double *avgTimeUser, double *avgTimeMPI,
			double *dest, const double *src );
int TestVecPack2Double( int n, int stride, 
			double *avgTimeUser, double *avgTimeMPI,
			double *dest, const double *src )
{
	double *restrict d_dest;
	const double *restrict d_src;
	register int i, j;
	int          rep, position;
	double       t1, t2, t[NTRIALS];
	MPI_Datatype vectype;

	/* User code */
	if (verbose) printf("TestVecPack2Double (USER): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			i = n;
			d_dest = dest;
			d_src  = src;
			while (i--) {
				*d_dest++ = d_src[0];
				*d_dest++ = d_src[1];
				d_src += stride;
			}
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
		return 0;
	}
	*avgTimeUser = mean(t, NTRIALS) / N_REPS;
    
	/* MPI Vector code */
	MPI_Type_vector( n, 2, stride, MPI_DOUBLE, &vectype );
	MPI_Type_commit( &vectype );
    
	if (verbose) printf("TestVecPack2Double (MPI): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			position = 0;
			MPI_Pack( (void *)src, 1, vectype, dest, 2*n*sizeof(double),
				  &position, MPI_COMM_SELF );
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
	}
	else {
	    *avgTimeMPI = mean(t, NTRIALS) / N_REPS;
	}
	MPI_Type_free( &vectype );

	return 0;
}

/* This creates an indexed type that is like a vector (for simplicity
   of construction).  There is a possibility that the MPI implementation 
   will recognize and simplify this (e.g., in MPI_Type_commit); if so,
   let us know and we'll add a version that is not as regular 
*/
/* See above for why restrict is not used in the function args */
int TestIndexPackDouble( int n, int stride, 
			 double *avgTimeUser, double *avgTimeMPI,
			 double *dest, const double *src );
int TestIndexPackDouble( int n, int stride, 
			 double *avgTimeUser, double *avgTimeMPI,
			 double *dest, const double *src )
{
	double *restrict d_dest;
	const double *restrict d_src;
	register int i, j;
	int          rep, position;
	int          *restrict displs = 0;
	double       t1, t2, t[NTRIALS];
	MPI_Datatype indextype;

	displs = (int *)malloc( n * sizeof(int) );
	for (i=0; i<n; i++) displs[i] = i * stride;

	/* User code */
	if (verbose) printf("TestIndexPackDouble (USER): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			i = n;
			d_dest = dest;
			d_src  = src;
			for (i=0; i<n; i++) {
				*d_dest++ = d_src[displs[i]];
			}
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
		return 0;
	}
	*avgTimeUser = mean(t, NTRIALS) / N_REPS;
    
	/* MPI Index code */
	MPI_Type_create_indexed_block( n, 1, displs, MPI_DOUBLE, &indextype );
	MPI_Type_commit( &indextype );

	free( displs );
    
	if (verbose) printf("TestIndexPackDouble (MPI): ");
	for (j = 0; j < NTRIALS; j++) {
		t1 = MPI_Wtime();
		for (rep=0; rep<N_REPS; rep++) {
			position = 0;
			MPI_Pack( (void *)src, 1, indextype, dest, n*sizeof(double),
				  &position, MPI_COMM_SELF );
		}
		t2 = MPI_Wtime() - t1;
		t[j] = t2;
		if (verbose) printf("%.3f ", t[j]);
	}
	if (verbose) printf("[%.3f]\n", noise(t, NTRIALS));
	/* If there is too much noise, discard the test */
	if (noise(t, NTRIALS) > VARIANCE_THRESHOLD) {
		*avgTimeUser = 0;
		*avgTimeMPI = 0;
		if (verbose)
			printf("Too much noise; discarding measurement\n");
	}
	else {
	    *avgTimeMPI = mean(t, NTRIALS) / N_REPS;
	}
	MPI_Type_free( &indextype );

	return 0;
}

int Report( const char *name, const char *packname, 
	    double avgTimeMPI, double avgTimeUser );
int Report( const char *name, const char *packname, 
	    double avgTimeMPI, double avgTimeUser )
{
	double diffTime, maxTime;
	int errs=0;

	/* Move this into a common routine */
	diffTime = avgTimeMPI - avgTimeUser;
	if (diffTime < 0) diffTime = - diffTime;
	if (avgTimeMPI > avgTimeUser) maxTime = avgTimeMPI;
	else                          maxTime = avgTimeUser;

	if (verbose) {
		printf( "%-30s:\t%g\t%g\t(%g%%)\n", name, 
			avgTimeMPI, avgTimeUser,
			100 * (diffTime / maxTime) );
		fflush(stdout);
	}
	if (avgTimeMPI > avgTimeUser && (diffTime > THRESHOLD * maxTime)) {
		errs++;
		printf( "%s:\tMPI %s code is too slow: MPI %g\t User %g\n",
			name, packname, avgTimeMPI, avgTimeUser );
	}

	return errs;
}

/* Finally, here's the main program */
int main( int argc, char *argv[] )
{
    int n, stride, err, errs = 0;
    void *dest, *src;
    double avgTimeUser, avgTimeMPI;

    MPI_Init( &argc, &argv );
    if (getenv("MPITEST_VERBOSE")) verbose = 1;

    n      = 30000;
    stride = 4;
    dest = (void *)malloc( n * sizeof(double) );
    src  = (void *)malloc( n * ((1+stride)*sizeof(double)) );
    /* Touch the source and destination arrays */
    memset( src, 0, n * (1+stride)*sizeof(double) );
    memset( dest, 0, n * sizeof(double) );

    err = TestVecPackDouble( n, stride, &avgTimeUser, &avgTimeMPI,
			     dest, src );
    errs += Report( "VecPackDouble", "Pack", avgTimeMPI, avgTimeUser );

    err = TestVecUnPackDouble( n, stride, &avgTimeUser, &avgTimeMPI,
			       src, dest );
    errs += Report( "VecUnPackDouble", "Unpack", avgTimeMPI, avgTimeUser );

    err = TestIndexPackDouble( n, stride, &avgTimeUser, &avgTimeMPI,
			     dest, src );
    errs += Report( "VecIndexDouble", "Pack", avgTimeMPI, avgTimeUser );

    free(dest);
    free(src);
    
    dest = (void *)malloc( 2*n * sizeof(double) );
    src  = (void *)malloc( (1 + n) * ((1+stride)*sizeof(double)) );
    memset( dest, 0, 2*n * sizeof(double) );
    memset( src, 0, (1+n) * (1+stride)*sizeof(double) );
    err = TestVecPack2Double( n, stride, &avgTimeUser, &avgTimeMPI,
			      dest, src );
    errs += Report( "VecPack2Double", "Pack", avgTimeMPI, avgTimeUser );

    free(dest);
    free(src);
    


    if (errs == 0) {
	printf( " No Errors\n" );
    }
    else {
	printf( " Found %d performance problems\n", errs );
    }

    fflush(stdout);
    MPI_Finalize();

    return 0;
}
