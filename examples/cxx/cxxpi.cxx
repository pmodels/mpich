/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpi.h"
/* Support both new (e.g., has iostream) and old (requires iostream.h) 
   C++ compilers */
#ifdef HAVE_CXX_IOSTREAM
#include <iostream>
#ifdef HAVE_NAMESPACE_STD
// Those that do often need the std namespace; otherwise, a bare "cout"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#ifdef HAVE_CXX_MATH
#include <math>
#else
#include <math.h>
#endif

double f(double);

double f(double a)
{
    return (4.0 / (1.0 + a*a));
}

int main(int argc,char **argv)
{
    int n, myid, numprocs, i;
    double PI25DT = 3.141592653589793238462643;
    double mypi, pi, h, sum, x;
    double startwtime = 0.0, endwtime;
    int  namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI::Init(argc,argv);
    numprocs = MPI::COMM_WORLD.Get_size();
    myid     = MPI::COMM_WORLD.Get_rank();
    MPI::Get_processor_name(processor_name,namelen);

    cout << "Process " << myid << " of " << numprocs << " is on " <<
	processor_name << endl;

    n = 10000;			/* default # of rectangles */
    if (myid == 0)
	startwtime = MPI::Wtime();

    MPI::COMM_WORLD.Bcast(&n, 1, MPI_INT, 0);

    h   = 1.0 / (double) n;
    sum = 0.0;
    /* A slightly better approach starts from large i and works back */
    for (i = myid + 1; i <= n; i += numprocs)
    {
	x = h * ((double)i - 0.5);
	sum += f(x);
    }
    mypi = h * sum;

    MPI::COMM_WORLD.Reduce(&mypi, &pi, 1, MPI_DOUBLE, MPI_SUM, 0);

    if (myid == 0) {
	endwtime = MPI::Wtime();
	cout << "pi is approximately " << pi << " Error is " <<
	    fabs(pi - PI25DT) << endl;
	cout << "wall clock time = " << endwtime-startwtime << endl;
    }

    MPI::Finalize();
    return 0;
}
