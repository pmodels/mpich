/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
  Tests invocation of error handlers on a variety of communicators, including
  COMM_NULL.  
*/

#include "mpitestconf.h"
#include <mpi.h>
#include <iostream>
#include "mpitestcxx.h"

/* returns number of errors found */
int testCommCall( MPI::Comm &comm )
{
    int i;
    bool sawException = false;

    comm.Set_errhandler( MPI::ERRORS_THROW_EXCEPTIONS );
    try {
	// Invalid send
	comm.Send( &i, -1, MPI::DATATYPE_NULL, -1, -10 );
    } catch (MPI::Exception &ex ) {
	// This is good
	sawException = true;
    }
    catch (...) {
	// This is bad
    }
    if (!sawException) {
	int len;
	char commname[MPI::MAX_OBJECT_NAME];
	comm.Get_name( commname, len );
	std::cout << "Did not see MPI exception on invalid call on communicator " <<
	    commname << "\n";
    }
    return sawException ? 0 : 1;
}

int testNullCommCall( void )
{
    int i;
    bool sawException = false;
    const MPI::Comm &comm = MPI::COMM_NULL;

    MPI::COMM_WORLD.Set_errhandler( MPI::ERRORS_THROW_EXCEPTIONS );
    try {
	// Invalid send
	comm.Send( &i, -1, MPI::DATATYPE_NULL, -1, -10 );
    } catch (MPI::Exception &ex ) {
	// This is good
	sawException = true;
    }
    catch (...) {
	// This is bad
    }
    if (!sawException) {
	std::cout << "Did not see MPI exception on invalid call on communicator COMM_NULL\n";
    }
    return sawException ? 0 : 1;
}

int main( int argc, char *argv[] )
{
    int errs = 0;
    bool periods[2] = { false, false };
    int  dims[2] = { 0, 0 };

    MTest_Init( );

    MPI::Compute_dims( MPI::COMM_WORLD.Get_size(), 2, dims );
    MPI::Cartcomm cart = MPI::COMM_WORLD.Create_cart( 2, dims, periods, true );
    cart.Set_name( "Cart comm" );
    // Graphcomm graph = COMM_WORLD.Create_graph( );

    errs += testCommCall( MPI::COMM_WORLD );
    errs += testNullCommCall( );
    errs += testCommCall( MPI::COMM_SELF );
    errs += testCommCall( cart );
    //errs += testCommCall( graph );
    
    MTest_Finalize( errs );

    cart.Free();
    // graph.Free();

    MPI::Finalize();

    return 0;
}
