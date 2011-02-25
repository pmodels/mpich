/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*

  Exercise communicator routines for intercommunicators

  This C version derived from attrt, which in turn was
  derived from a Fortran test program from ...

 */
#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* #define DEBUG */
int test_communicators ( void );
int copy_fn ( MPI_Comm, int, void *, void *, void *, int * );
int delete_fn ( MPI_Comm, int, void *, void * );
#ifdef DEBUG
#define FFLUSH fflush(stdout);
#else
#define FFLUSH
#endif

int main( int argc, char **argv )
{
    int errs = 0;
    MTest_Init( &argc, &argv );
    
    errs = test_communicators();

    MTest_Finalize( errs );
    MPI_Finalize();
    return 0;
}

int copy_fn( MPI_Comm oldcomm, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, int *flag)
{
    /* Note that if (sizeof(int) < sizeof(void *), just setting the int
       part of attribute_val_out may leave some dirty bits
    */
    *(MPI_Aint *)attribute_val_out = (MPI_Aint)attribute_val_in;
    *flag = 1;
    return MPI_SUCCESS;
}

int delete_fn( MPI_Comm comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    int world_rank;
    MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
    if ((MPI_Aint)attribute_val != (MPI_Aint)world_rank) {
	printf( "incorrect attribute value %d\n", *(int*)attribute_val );
	MPI_Abort(MPI_COMM_WORLD, 1005 );
    }
    return MPI_SUCCESS;
}

int test_communicators( void )
{
    MPI_Comm dup_comm, comm;
    void *vvalue;
    int flag, world_rank, world_size, key_1, key_3;
    int errs = 0;
    MPI_Aint value;
    int      isLeft;

    MPI_Comm_rank( MPI_COMM_WORLD, &world_rank );
    MPI_Comm_size( MPI_COMM_WORLD, &world_size );
#ifdef DEBUG
    if (world_rank == 0) {
	printf( "*** Communicators ***\n" ); fflush(stdout);
    }
#endif

    while (MTestGetIntercomm( &comm, &isLeft, 2 )) {
        MTestPrintfMsg(1, "start while loop, isLeft=%s\n", (isLeft ? "TRUE" : "FALSE"));

	if (comm == MPI_COMM_NULL) {
            MTestPrintfMsg(1, "got COMM_NULL, skipping\n");
            continue;
        }

	/*
	  Check Comm_dup by adding attributes to comm & duplicating
	*/
    
	value = 9;
	MPI_Keyval_create(copy_fn,     delete_fn,   &key_1, &value );
        MTestPrintfMsg(1, "Keyval_create key=%#x value=%d\n", key_1, value);
	value = 7;
	MPI_Keyval_create(MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,
			  &key_3, &value ); 
        MTestPrintfMsg(1, "Keyval_create key=%#x value=%d\n", key_3, value);

	/* This may generate a compilation warning; it is, however, an
	   easy way to cache a value instead of a pointer */
	/* printf( "key1 = %x key3 = %x\n", key_1, key_3 ); */
	MPI_Attr_put(comm, key_1, (void *) (MPI_Aint) world_rank );
	MPI_Attr_put(comm, key_3, (void *)0 );
	
        MTestPrintfMsg(1, "Comm_dup\n" );
	MPI_Comm_dup(comm, &dup_comm );

	/* Note that if sizeof(int) < sizeof(void *), we can't use
	   (void **)&value to get the value we passed into Attr_put.  To avoid 
	   problems (e.g., alignment errors), we recover the value into 
	   a (void *) and cast to int. Note that this may generate warning
	   messages from the compiler.  */
	MPI_Attr_get(dup_comm, key_1, (void **)&vvalue, &flag );
	value = (MPI_Aint)vvalue;
	
	if (! flag) {
	    errs++;
	    printf( "dup_comm key_1 not found on %d\n", world_rank );
	    fflush( stdout );
	    MPI_Abort(MPI_COMM_WORLD, 3004 );
	}
	
	if (value != world_rank) {
	    errs++;
	    printf( "dup_comm key_1 value incorrect: %ld\n", (long)value );
	    fflush( stdout );
	    MPI_Abort(MPI_COMM_WORLD, 3005 );
	}

	MPI_Attr_get(dup_comm, key_3, (void **)&vvalue, &flag );
	value = (MPI_Aint)vvalue;
	if (flag) {
	    errs++;
	    printf( "dup_comm key_3 found!\n" );
	    fflush( stdout );
	    MPI_Abort(MPI_COMM_WORLD, 3008 );
	}
        MTestPrintfMsg(1, "Keyval_free key=%#x\n", key_1);
	MPI_Keyval_free(&key_1 );
        MTestPrintfMsg(1, "Keyval_free key=%#x\n", key_3);
	MPI_Keyval_free(&key_3 );
	/*
	  Free all communicators created
	*/
        MTestPrintfMsg(1, "Comm_free comm\n");
	MPI_Comm_free( &comm );
        MTestPrintfMsg(1, "Comm_free dup_comm\n");
	MPI_Comm_free( &dup_comm );
    }

    return errs;
}

