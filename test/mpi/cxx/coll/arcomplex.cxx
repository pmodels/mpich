/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
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

int main(int argc, char **argv)
{
    MPI::Intracomm comm = MPI::COMM_WORLD;
    int errs = 0;
    int size, rank;
    long sum;
    complex < float >c[2], c_out[2], c_expected;
    complex < double >cd[2], cd_out[2], cd_expected;
#ifdef HAVE_LONG_DOUBLE
    complex < long double >cld[2], cld_out[2], cld_expected;
    MTEST_VG_MEM_INIT(cld, 2 * sizeof(complex < long double >));
#endif

    MTest_Init();

    size = comm.Get_size();
    rank = comm.Get_rank();

    c[0] = ((float) rank) * complex < float >(1, 2);
    c[1] = ((float) rank) * complex < float >(-1, 4);
    cd[0] = ((double) rank) * complex < double >(1, 2);
    cd[1] = ((double) rank) * complex < double >(-1, 4);
#ifdef HAVE_LONG_DOUBLE
    cld[0] = ((long double) rank) * complex < long double >(1, 2);
    cld[1] = ((long double) rank) * complex < long double >(-1, 4);
#endif

    // Sums are easy - real and imaginary parts separate
    // result should be sum(0:size-1) *{ complex(1,2), complex(-1,4) }
    // The sum is (size*(size-1))/2
    comm.Allreduce(c, c_out, 2, MPI::COMPLEX, MPI::SUM);
    sum = (size * (size - 1)) / 2;
    c_expected = ((float) sum) * complex < float >(1, 2);
    if (c_out[0].real() != c_expected.real() || c_out[0].imag() != c_expected.imag()) {
        errs++;
        cout << "c_out[0] was " << c_out[0] << " expected " <<
            c_expected;
    }
    c_expected = ((float) sum) * complex < float >(-1, 4);
    if (c_out[1].real() != c_expected.real() || c_out[1].imag() != c_expected.imag()) {
        errs++;
        cout << "c_out[1] was " << c_out[1] << " expected " <<
            c_expected;
    }
    comm.Allreduce(cd, cd_out, 2, MPI::DOUBLE_COMPLEX, MPI::SUM);
    cd_expected = ((double) sum) * complex < double >(1, 2);
    if (cd_out[0].real() != cd_expected.real() || cd_out[0].imag() != cd_expected.imag()) {
        errs++;
        cout << "cd_out[0] was " << cd_out[0] << " expected " <<
            cd_expected;
    }
    cd_expected = ((double) sum) * complex < double >(-1, 4);
    if (cd_out[1].real() != cd_expected.real() || cd_out[1].imag() != cd_expected.imag()) {
        errs++;
        cout << "cd_out[1] was " << cd_out[1] << " expected " <<
            cd_expected;
    }
#ifdef HAVE_LONG_DOUBLE
    comm.Allreduce(cld, cld_out, 2, MPI::LONG_DOUBLE_COMPLEX, MPI::SUM);
    cld_expected = ((long double) sum) * complex < long double >(1, 2);
    if (cld_out[0].real() != cld_expected.real() || cld_out[0].imag() != cld_expected.imag()) {
        errs++;
        cout << "cld_out[0] was " << cld_out[0] << " expected " <<
            cld_expected;
    }
    cld_expected = ((long double) sum) * complex < long double >(-1, 4);
    if (cld_out[1].real() != cld_expected.real() || cld_out[1].imag() != cld_expected.imag()) {
        errs++;
        cout << "cld_out[1] was " << cld_out[1] << " expected " <<
            cld_expected;
    }
#endif

    MTest_Finalize(errs);
    return 0;
}
