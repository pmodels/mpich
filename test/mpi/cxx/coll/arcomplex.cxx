/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2008 by Argonne National Laboratory.
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
#include <complex>
#include "mpitestcxx.h"

int main( int argc, char **argv )
{
    MPI::Intracomm comm = MPI::COMM_WORLD;
    int errs = 0;
    int size, rank;
    long sum;
    complex<float> c[2], c_out[2];
    complex<double> cd[2], cd_out[2];
#ifdef HAVE_LONG_DOUBLE
    complex<long double> cld[2], cld_out[2];
#endif

    MTest_Init( );

    size = comm.Get_size();
    rank = comm.Get_rank();
    
    c[0]   = ((float)rank)*complex<float>(1,2);
    c[1]   = ((float)rank)*complex<float>(-1,4);
    cd[0]  = ((double)rank)*complex<double>(1,2);
    cd[1]  = ((double)rank)*complex<double>(-1,4);
#ifdef HAVE_LONG_DOUBLE
    cld[0] = ((long double)rank)*complex<long double>(1,2);
    cld[1] = ((long double)rank)*complex<long double>(-1,4);
#endif
    
    // Sums are easy - real and imaginary parts separate
    // result should be sum(0:size-1) *{ complex(1,2), complex(-1,4) }
    // The sum is (size*(size-1))/2
    comm.Allreduce( c, c_out, 2, MPI::COMPLEX, MPI::SUM );
    sum = (size * (size-1) ) / 2;
    if (c_out[0] != ((float)sum) * complex<float>(1,2)) {
	errs++;
	cout << "c_out[0] was " << c_out[0] << " expected " << 
	    ((float)sum) * complex<float>(1,2);
    }
    if (c_out[1] != ((float)sum) * complex<float>(-1,4)) {
	errs++;
	cout << "c_out[1] was " << c_out[1] << " expected " << 
	    ((float)sum) * complex<float>(-1,4);
    }
    comm.Allreduce( cd, cd_out, 2, MPI::DOUBLE_COMPLEX, MPI::SUM );
    if (cd_out[0] != ((double)sum) * complex<double>(1,2)) {
	errs++;
	cout << "cd_out[0] was " << cd_out[0] << " expected " << 
	    ((double)sum) * complex<double>(1,2);
    }
    if (cd_out[1] != ((double)sum) * complex<double>(-1,4)) {
	errs++;
	cout << "cd_out[1] was " << cd_out[1] << " expected " << 
	    ((double)sum) * complex<double>(-1,4);
    }
#ifdef HAVE_LONG_DOUBLE
    comm.Allreduce( cld, cld_out, 2, MPI::LONG_DOUBLE_COMPLEX, MPI::SUM );
    if (cld_out[0] != ((long double)sum) * complex<long double>(1,2)) {
	errs++;
	cout << "cld_out[0] was " << cld_out[0] << " expected " << 
	    ((long double)sum) * complex<long double>(1,2);
    }
    if (cld_out[1] != ((long double)sum) * complex<long double>(-1,4)) {
	errs++;
	cout << "cld_out[1] was " << cld_out[1] << " expected " << 
	    ((long double)sum) * complex<long double>(-1,4);
    }
#endif

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
