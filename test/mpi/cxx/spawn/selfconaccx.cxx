/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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
#include "mpitestcxx.h"

int main( int argc, char *argv[] )
{
    int error;
    int rank, size;
    char port[MPI_MAX_PORT_NAME];
    MPI::Status status;
    MPI::Intercomm comm;

    MPI::Init(argc, argv);

    size = MPI::COMM_WORLD.Get_size();
    rank = MPI::COMM_WORLD.Get_rank();

    if (size < 2)
    {
	cout << "Two processes needed.\n"; 
	MPI::Finalize();
	return 0;
    }

    if (rank == 0)
    {
	MPI::Open_port(MPI::INFO_NULL, port);
	MPI::COMM_WORLD.Send(port, MPI::MAX_PORT_NAME, MPI::CHAR, 1, 0 );
	comm = MPI::COMM_SELF.Accept(port, MPI::INFO_NULL, 0 );
	MPI::Close_port(port);
	comm.Disconnect();
    }
    else if (rank == 1) {
	MPI::COMM_WORLD.Recv(port, MPI::MAX_PORT_NAME, MPI::CHAR, 0, 0 );
	comm = MPI::COMM_SELF.Connect(port, MPI::INFO_NULL, 0 );
	comm.Disconnect();
    }

    MPI::COMM_WORLD.Barrier();

    if (rank == 0) {
	cout << " No Errors\n";
    }
    MPI::Finalize();
    return 0;
}
