/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* Sanity check that logical operations can be performed on C++ bool types
 * via MPI::BOOL. */

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

int main( int argc, char **argv )
{
    MPI::Intracomm comm = MPI::COMM_WORLD;
    int errs = 0;
    int size, i, count, root, rank;

    MTest_Init( );

    size = comm.Get_size();
    rank = comm.Get_rank();

    for (count = 1; count < 66000; count = count * 2) {
        bool *vin, *vout;
        vin  = new bool[count];
        vout = new bool[count];

        for (root = 0; root < size; root++) {
            for (i=0; i<count; i++) {
                // only rank 0's elements are set to true
                vin[i]  = (rank ? false : true);
                vout[i] = false;
            }
            comm.Reduce(vin, vout, count, MPI::BOOL, MPI::LOR, root);
            if (rank == root) {
                for (i=0; i<count; i++) {
                    if (vout[i] != true) {
                        errs++;
                        if (errs < 10)
                            cerr << rank << ": " << "count=" << count
                                 << " root=" << root
                                 << " vout[" << i << "]=" << vout[i] << endl;
                    }
                }
            }
        }
        delete[] vin;
        delete[] vout;
    }

    MTest_Finalize( errs );
    MPI::Finalize();
    return 0;
}
