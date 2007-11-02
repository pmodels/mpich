/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*

  Exercise communicator routines.

  This C++ version derived from a Fortran test program from ....
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

int test_communicators ( void );
int copy_fn ( const MPI::Comm &, int, void *, void *, void *, bool & );
int delete_fn ( MPI::Comm &, int, void *, void * );

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
    return MPI::SUCCESS;
}

int delete_fn( MPI::Comm &comm, int keyval, void *attribute_val, 
	       void *extra_state)
{
    int world_rank;
    world_rank = MPI::COMM_WORLD.Get_rank();
    if ((MPI::Aint)attribute_val != (MPI::Aint)world_rank) {
	cout << "incorrect attribute value " << *(int*)attribute_val << "\n";
	MPI::COMM_WORLD.Abort( 1005 );
    }
    return MPI::SUCCESS;
}

int test_communicators( void )
{
    MPI::Intracomm dup_comm_world, lo_comm, rev_comm, dup_comm, 
	split_comm, world_comm;
    MPI::Group world_group, lo_group, rev_group;
    void *vvalue;
    int ranges[1][3];
    int flag, world_rank, world_size, rank, size, n, key_1, key_3;
    int color, key, result;
    int errs = 0;
    MPI::Aint value;

    world_rank = MPI::COMM_WORLD.Get_rank();
    world_size = MPI::COMM_WORLD.Get_size();
#ifdef DEBUG
    if (world_rank == 0) {
	cout << "*** Communicators ***\n";
    }
#endif

    dup_comm_world = MPI::COMM_WORLD.Dup();

    /*
      Exercise Comm_create by creating an equivalent to dup_comm_world
      (sans attributes) and a half-world communicator.
    */

#ifdef DEBUG
    if (world_rank == 0) {
	cout << "    Comm_create\n";
    }
#endif

    world_group = dup_comm_world.Get_group();
    world_comm  = dup_comm_world.Create( world_group );
    rank = world_comm.Get_rank();
    if (rank != world_rank) {
	errs++;
	cout << "incorrect rank in world comm: " << rank << "\n";
	MPI::COMM_WORLD.Abort( 3001 );
    }

    n = world_size / 2;

    ranges[0][0] = 0;
    ranges[0][1] = (world_size - n) - 1;
    ranges[0][2] = 1;

#ifdef DEBUG
    cout << "world rank = " << world_rank << " before range incl\n";
#endif
    lo_group = world_group.Range_incl( 1, ranges );
#ifdef DEBUG
    cout << "world rank = " << world_rank << " after range incl\n";
#endif
    lo_comm = world_comm.Create( lo_group );
#ifdef DEBUG
    cout << "world rank = " << world_rank << " before group free\n";
#endif
    lo_group.Free();
    
#ifdef DEBUG
    cout << "world rank = " << world_rank << " after group free\n";
#endif

    if (world_rank < (world_size - n)) {
	rank = lo_comm.Get_rank();
	if (rank == MPI::UNDEFINED) {
	    errs++;
	    cout << "incorrect lo group rank: " << rank << "\n";
	    MPI::COMM_WORLD.Abort( 3002 );
	}
	else {
	    lo_comm.Barrier();
	}
    }
    else {
	if (lo_comm != MPI::COMM_NULL) {
	    errs++;
	    cout << "incorrect lo comm:\n";
	    MPI::COMM_WORLD.Abort( 3003 );
	}
    }
    
#ifdef DEBUG
    cout << "worldrank = " << world_rank << "\n";
#endif
    world_comm.Barrier();
    
#ifdef DEBUG
    cout << "bar!\n";
#endif
    /*
      Check Comm_dup by adding attributes to lo_comm & duplicating
    */
#ifdef DEBUG
    if (world_rank == 0) {
	cout << "    Comm_dup\n";
    }
#endif
    
    if (lo_comm != MPI::COMM_NULL) {
	value = 9;
	key_1 = MPI::Comm::Create_keyval(copy_fn, delete_fn, &value );
	value = 8;
	value = 7;
	key_3 = MPI::Comm::Create_keyval(MPI::Comm::NULL_COPY_FN, 
					 MPI::Comm::NULL_DELETE_FN, &value ); 
	
	/* This may generate a compilation warning; it is, however, an
	   easy way to cache a value instead of a pointer */
	lo_comm.Set_attr( key_1, (void *) (MPI_Aint) world_rank );
	lo_comm.Set_attr( key_3, (void *)0 );
	
	dup_comm = lo_comm.Dup();

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
	if (flag) {
	    errs++;
	    cout << "dup_comm key_3 found!\n";
	    MPI::COMM_WORLD.Abort( 3008 );
	}
	// Some C++ compilers (e.g., Solaris) refuse to 
	// accept a straight cast to an int.
	// value = (MPI::Aint)vvalue;
	MPI::Comm::Free_keyval( key_1 );
	MPI::Comm::Free_keyval( key_3 );
    }
    /* 
       Split the world into even & odd communicators with reversed ranks.
    */
#ifdef DEBUG
    if (world_rank == 0) {
	cout << "    Comm_split\n";
    }
#endif
    
    color = world_rank % 2;
    key   = world_size - world_rank;
    
    split_comm = dup_comm_world.Split( color, key );
    size = split_comm.Get_size();
    rank = split_comm.Get_rank();
    if (rank != ((size - world_rank/2) - 1)) {
	errs++;
	cout << "incorrect split rank: " << rank << "\n";
	MPI::COMM_WORLD.Abort( 3009 );
    }
    
    split_comm.Barrier();
    /*
      Test each possible Comm_compare result
    */
#ifdef DEBUG
    if (world_rank == 0) {
	cout << "    Comm_compare\n";
    }
#endif
    
    result = MPI::Comm::Compare(world_comm, world_comm );
    if (result != MPI::IDENT) {
	errs++;
	cout << "incorrect ident result: " << result << "\n";
	MPI::COMM_WORLD.Abort( 3010 );
    }
    
    if (lo_comm != MPI::COMM_NULL) {
	result = MPI::Comm::Compare(lo_comm, dup_comm );
	if (result != MPI::CONGRUENT) {
	    errs++;
            cout << "incorrect congruent result: " << result << "\n";
            MPI::COMM_WORLD.Abort( 3011 );
	}
    }
    
    ranges[0][0] = world_size - 1;
    ranges[0][1] = 0;
    ranges[0][2] = -1;

    rev_group = world_group.Range_incl( 1, ranges );
    rev_comm  = world_comm.Create( rev_group );

    result = MPI::Comm::Compare(world_comm, rev_comm );
    if (result != MPI::SIMILAR && world_size != 1) {
	errs++;
	cout << "incorrect similar result: " << result << "\n";
	MPI::COMM_WORLD.Abort( 3012 );
    }
    
    if (lo_comm != MPI::COMM_NULL) {
	result = MPI::Comm::Compare(world_comm, lo_comm );
	if (result != MPI::UNEQUAL && world_size != 1) {
	    errs++;
	    cout << "incorrect unequal result: " << result << "\n";
	    MPI::COMM_WORLD.Abort( 3013 );
	}
    }
    /*
      Free all communicators created
    */
#ifdef DEBUG
    if (world_rank == 0) 
	cout << "    Comm_free\n";
#endif

    world_comm.Free();
    dup_comm_world.Free();

    rev_comm.Free();
    split_comm.Free();

    world_group.Free();
    rev_group.Free();
    
    if (lo_comm != MPI::COMM_NULL) {
	lo_comm.Free();
	dup_comm.Free();
    }
    
    return errs;
}

