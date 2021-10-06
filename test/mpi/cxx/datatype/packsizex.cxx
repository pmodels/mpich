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
// Those that do often need the std namespace; otherwise, a bare "cerr"
// is likely to fail to compile
using namespace std;
#endif
#else
#include <iostream.h>
#endif
#include "mpitestcxx.h"
extern "C" {
#include "dtpools.h"
}
#include <stdlib.h>
int main(int argc, char *argv[])
{
    int err, errs = 0;
    int i, j;
    MPI_Aint count, maxbufsize;
    MPI::Datatype type;
    char *basic_type;
    MPI::Intracomm comm;
    DTP_pool_s dtp;
    DTP_obj_s msobj, mrobj;
    int size1, size2, tnlen;
    int seed, testsize;

    MTest_Init();

    comm = MPI::COMM_WORLD;

    MTestArgList *head = MTestArgListCreate(argc, argv);
    seed = MTestArgListGetInt(head, "seed");
    testsize = MTestArgListGetInt(head, "testsize");
    count = MTestArgListGetLong(head, "count");
    basic_type = MTestArgListGetString(head, "type");

    maxbufsize = MTestDefaultMaxBufferSize();

    err = DTP_pool_create(basic_type, count, seed, &dtp);
    if (err != DTP_SUCCESS) {
        cerr << "Error while creating dtp pool (" << basic_type << ",1)\n";
    }

    MTestArgListDestroy(head);

    for (i = 0; i < testsize; i++) {
        err = DTP_obj_create(dtp, &msobj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        err += DTP_obj_create(dtp, &mrobj, maxbufsize);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        type = msobj.DTP_datatype;

        // Testing the pack size is tricky, since this is the
        // size that is stored when packed with type.Pack, and
        // is not easily defined.  We look for consistency
        size1 = type.Pack_size(1, comm);
        size2 = type.Pack_size(2, comm);
        if (size1 <= 0 || size2 <= 0) {
            errs++;
            cerr << "Pack size of datatype " << DTP_obj_get_description(msobj) << " is not positive\n";
        }
        if (size1 >= size2) {
            errs++;
            cerr << "Pack size of 2 of " << DTP_obj_get_description(msobj) <<
                " is smaller or the same as the pack size of 1 instance\n";
        }

        if (mrobj.DTP_datatype != msobj.DTP_datatype) {
            type = mrobj.DTP_datatype;

            // Testing the pack size is tricky, since this is the
            // size that is stored when packed with type.Pack, and
            // is not easily defined.  We look for consistency
            size1 = type.Pack_size(1, comm);
            size2 = type.Pack_size(2, comm);
            if (size1 <= 0 || size2 <= 0) {
                errs++;
                cerr << "Pack size of datatype " << DTP_obj_get_description(mrobj) << " is not positive\n";
            }
            if (size1 >= size2) {
                errs++;
                cerr << "Pack size of 2 of " << DTP_obj_get_description(mrobj) <<
                    " is smaller or the same as the pack size of 1 instance\n";
            }
        }
        DTP_obj_free(mrobj);
        DTP_obj_free(msobj);
    }

    DTP_pool_free(dtp);

    MTest_Finalize(errs);

    return 0;
}
