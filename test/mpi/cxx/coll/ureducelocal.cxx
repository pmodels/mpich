/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
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

static MPI::Datatype my_datatype;
static int real_count;

void uop(const void *invec, void *inoutvec, int count, const MPI::Datatype & datatype)
{
    int i;

    if (datatype == my_datatype)
        count = real_count;

    for (i = 0; i < count; i++) {
        if (datatype == MPI::INT)
            ((int *)inoutvec)[i] = ((int *)invec)[i] + ((int *)inoutvec)[i];
        else if (datatype == MPI::DOUBLE || datatype == my_datatype)
            ((double *)inoutvec)[i] = ((double *)invec)[i] + ((double *)inoutvec)[i];
        else
            return;
    }
}

int main(int argc, char **argv)
{
    MPI::Op sumop;
    MPI::Intracomm comm = MPI::COMM_WORLD;
    int errs = 0;
    int size, i, count, root, rank;
    int *vin, *vout;
    double *dvin, *dvout;

    MTest_Init();

    sumop.Init(uop, true);
    size = comm.Get_size();
    rank = comm.Get_rank();

    for (count = 1; count < 66000; count = count * 2) {
        /* MPI::INT */
        vin = new int[count];
        vout = new int[count];

        for (root = 0; root < size; root++) {
            for (i = 0; i < count; i++) {
                vin[i] = i;
                vout[i] = -1;
            }
            sumop.Reduce_local(vin, vout, count, MPI::INT);
            for (i = 0; i < count; i++) {
                if (vout[i] != i - 1) {
                    errs++;
                    if (errs < 10)
                        cerr << "vout[" << i << "] = " << vout[i] << endl;
                }
            }
        }
        delete[]vin;
        delete[]vout;

        /* MPI::DOUBLE */
        dvin = new double[count];
        dvout = new double[count];
        for (root = 0; root < size; root++) {
            for (i = 0; i < count; i++) {
                dvin[i] = i;
                dvout[i] = -1;
            }
            sumop.Reduce_local(dvin, dvout, count, MPI::DOUBLE);
            for (i = 0; i < count; i++) {
                if (dvout[i] != i - 1) {
                    errs++;
                    if (errs < 10)
                        cerr << "dvout[" << i << "] = " << dvout[i] << endl;
                }
            }
        }
        delete[]dvin;
        delete[]dvout;

        /* A vector of MPI::DOUBLEs */
        dvin = new double[count];
        dvout = new double[count];
        my_datatype = MPI::DOUBLE.Create_vector(count/2, 1, 2);
        my_datatype.Commit();
        real_count = count;
        for (root = 0; root < size; root++) {
            for (i = 0; i < count; i++) {
                dvin[i] = i;
                dvout[i] = -1;
            }
            sumop.Reduce_local(dvin, dvout, 1, my_datatype);
            for (i = 0; i < count; i += 2) {
                if (dvout[i] != i - 1) {
                    errs++;
                    if (errs < 10)
                        cerr << "dvout[" << i << "] = " << dvout[i] << endl;
                }
            }
        }
        delete[]dvin;
        delete[]dvout;
        my_datatype.Free();
    }

    sumop.Free();
    MTest_Finalize(errs);
    MPI::Finalize();
    return 0;
}
