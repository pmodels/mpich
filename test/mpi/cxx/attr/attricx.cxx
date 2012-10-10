/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*

  Exercise communicator routines for intercommunicators

  This C++ version derived from the C version, attric
 */
#include "mpi.h"
#include "mpitestconf.h"
#ifdef HAVE_IOSTREAM
// Not all C++ compilers have iostream instead of iostream.h
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include <stdio.h>
#include "mpitestcxx.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* #define DEBUG */
static int verbose = 0;
int test_communicators ( void );
int copy_fn ( const MPI::Comm &, int, void *, void *, void *, bool & );
int delete_fn ( MPI::Comm &, int, void *, void * );

#ifdef DEBUG
#define FFLUSH fflush(stdout);
#else
#define FFLUSH
#endif

int main( int argc, char **argv )
{
    int errs = 0;
    MTest_Init();
    
    errs = test_communicators();

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}

int copy_fn( const MPI::Comm &oldcomm, int keyval, void *extra_state,
	     void *attribute_val_in, void *attribute_val_out, bool &flag)
{
    /* Note that if (sizeof(int) < sizeof(void *), just setting the int
       part of attribute_val_out may leave some dirty bits
    */
    *(MPI::Aint *)attribute_val_out = (MPI::Aint)attribute_val_in;
    flag = 1;
    return MPI_SUCCESS;
}

int delete_fn( MPI::Comm &comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    int world_rank;
    world_rank = MPI::COMM_WORLD.Get_rank();
    if ((MPI::Aint)attribute_val != (MPI::Aint)world_rank) {
	cout << "incorrect attribute value %d\n" << *(int*)attribute_val << "\n";
	MPI::COMM_WORLD.Abort( 1005 );
    }
    return MPI_SUCCESS;
}

int test_communicators( void )
{
    MPI::Intercomm dup_comm, comm;
    void *vvalue;
    int flag, world_rank, world_size, key_1, key_3;
    int errs = 0;
    MPI::Aint value;
    int      isLeft;

    world_rank = MPI::COMM_WORLD.Get_rank();
    world_size = MPI::COMM_WORLD.Get_size();
#ifdef DEBUG
    if (world_rank == 0) {
	cout << "*** Communicators ***\n";
    }
#endif

    while (MTestGetIntercomm( comm, isLeft, 2 )) {
	if (verbose) {
	    cout << "start while loop, isLeft=" << isLeft << "\n";
	}

	if (comm == MPI::COMM_NULL) {
	    if (verbose) {
		cout << "got COMM_NULL, skipping\n";
	    }
            continue;
        }

	/*
	  Check Comm_dup by adding attributes to comm & duplicating
	*/
    
	value = 9;
	key_1 = MPI::Comm::Create_keyval(copy_fn,     delete_fn,   &value );
	if (verbose) {
	    cout << "Keyval_create key=" << key_1 << " value=" << value << "\n";
	}
	value = 7;
	key_3 = MPI::Comm::Create_keyval(MPI::Comm::NULL_COPY_FN, 
					 MPI::Comm::NULL_DELETE_FN,
					 &value ); 
	if (verbose) {
	    cout << "Keyval_create key=" << key_3 << " value=" << value << "\n";
	}

	/* This may generate a compilation warning; it is, however, an
	   easy way to cache a value instead of a pointer */
	/* printf( "key1 = %x key3 = %x\n", key_1, key_3 ); */
	comm.Set_attr( key_1, (void *) (MPI::Aint) world_rank );
	comm.Set_attr( key_3, (void *)0 );
	
	if (verbose) {
	    cout << "Comm_dup\n";
	}
	dup_comm = comm.Dup();

	/* Note that if sizeof(int) < sizeof(void *), we can't use
	   (void **)&value to get the value we passed into Attr_put.  To avoid 
	   problems (e.g., alignment errors), we recover the value into 
	   a (void *) and cast to int. Note that this may generate warning
	   messages from the compiler.  */
	flag = dup_comm.Get_attr( key_1, (void **)&vvalue );
	value = (MPI::Aint)vvalue;
	
	if (! flag) {
	    errs++;
	    cout << "dup_comm key_1 not found on " << world_rank << "\n";
	    MPI::COMM_WORLD.Abort( 3004 );
	}
	
	if (value != world_rank) {
	    errs++;
	    cout << "dup_comm key_1 value incorrect: " << (long)value << "\n";
	    MPI::COMM_WORLD.Abort( 3005 );
	}

	flag = dup_comm.Get_attr( key_3, (void **)&vvalue );
	value = (MPI::Aint)vvalue;
	if (flag) {
	    errs++;
	    cout << "dup_comm key_3 found!\n";
	    MPI::COMM_WORLD.Abort( 3008 );
	}
	if (verbose) { 
	    cout << "Keyval_free key=" << key_1 << "\n";
	}
	MPI::Comm::Free_keyval( key_1 );
	if (verbose) {
	    cout << "Keyval_free key=" << key_3 << "\n";
	}
	MPI::Comm::Free_keyval( key_3 );
	/*
	  Free all communicators created
	*/
	if (verbose) {
	    cout << "Comm_free comm\n";
	}
	comm.Free();
	if (verbose) {
	    cout << "Comm_free dup_comm\n";
	}
	dup_comm.Free();
    }

    return errs;
}

