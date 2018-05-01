/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2010 by Argonne National Laboratory.
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
extern "C" {
#include "dtpools.h"
}
#include <stdlib.h>

typedef struct {
    string type_name;
    MPI_Datatype type;
} Type_t;

Type_t typelist[] = {
    {"MPI_CHAR", MPI_CHAR},
    {"MPI_BYTE", MPI_BYTE},
    {"MPI_WCHAR", MPI_WCHAR},
    {"MPI_SHORT", MPI_SHORT},
    {"MPI_INT", MPI_INT},
    {"MPI_LONG", MPI_LONG},
    {"MPI_LONG_LONG_INT", MPI_LONG_LONG_INT},
    {"MPI_UNSIGNED_CHAR", MPI_UNSIGNED_CHAR},
    {"MPI_UNSIGNED_SHORT", MPI_UNSIGNED_SHORT},
    {"MPI_UNSIGNED", MPI_UNSIGNED},
    {"MPI_UNSIGNED_LONG", MPI_UNSIGNED_LONG},
    {"MPI_UNSIGNED_LONG_LONG", MPI_UNSIGNED_LONG_LONG},
    {"MPI_FLOAT", MPI_FLOAT},
    {"MPI_DOUBLE", MPI_DOUBLE},
    {"MPI_LONG_DOUBLE", MPI_LONG_DOUBLE},
    {"MPI_INT8_T", MPI_INT8_T},
    {"MPI_INT16_T", MPI_INT16_T},
    {"MPI_INT32_T", MPI_INT32_T},
    {"MPI_INT64_T", MPI_INT64_T},
    {"MPI_UINT8_T", MPI_UINT8_T},
    {"MPI_UINT16_T", MPI_UINT16_T},
    {"MPI_UINT32_T", MPI_UINT32_T},
    {"MPI_UINT64_T", MPI_UINT64_T},
    {"MPI_C_COMPLEX", MPI_C_COMPLEX},
    {"MPI_C_FLOAT_COMPLEX", MPI_C_FLOAT_COMPLEX},
    {"MPI_C_DOUBLE_COMPLEX", MPI_C_DOUBLE_COMPLEX},
    {"MPI_C_LONG_DOUBLE_COMPLEX", MPI_C_LONG_DOUBLE_COMPLEX},
    {"MPI_FLOAT_INT", MPI_FLOAT_INT},
    {"MPI_DOUBLE_INT", MPI_DOUBLE_INT},
    {"MPI_LONG_INT", MPI_LONG_INT},
    {"MPI_2INT", MPI_2INT},
    {"MPI_SHORT_INT", MPI_SHORT_INT},
    {"MPI_LONG_DOUBLE_INT", MPI_LONG_DOUBLE_INT},
    {"MPI_DATATYPE_NULL", MPI_DATATYPE_NULL}
};

int main(int argc, char *argv[])
{
    int err, errs = 0;
    int i, j;
    int count;
    MPI::Datatype type;
    MPI::Datatype basic_type;
    MPI::Intracomm comm;
    DTP_t mstype, mrtype;
    char dtypename[MPI_MAX_OBJECT_NAME];
    int size1, size2, tnlen;

    MTest_Init();

    comm = MPI::COMM_WORLD;

    /* TODO: parse input parameters using optarg */
    if (argc < 3) {
        cout << "Usage: " << argv[0] << "-type=[TYPE] -count=[COUNT]\n";
        return 0;
    } else {
        for (i = 1; i < argc; i++) {
            if (!strncmp(argv[i], "-type=", strlen("-type="))) {
                j = 0;
                while (strcmp(typelist[j].type_name.c_str(), "MPI_DATATYPE_NULL") &&
                       strcmp(argv[i]+strlen("-type="), typelist[j].type_name.c_str())) {
                    j++;
                }

                if (strcmp(typelist[j].type_name.c_str(), "MPI_DATATYPE_NULL")) {
                    basic_type = typelist[j].type;
                } else {
                    cout << "Error: datatype not recognized\n";
                    return 0;
                }
            } else if (!strncmp(argv[i], "-count=", strlen("-count="))) {
                count = atoi(argv[i]+strlen("-count="));
            }
        }
    }

    /* TODO: struct types are currently not supported for C++ */

    err = DTP_pool_create(basic_type, count, &mstype);
    if (err != DTP_SUCCESS) {
        basic_type.Get_name(dtypename, tnlen);
        cout << "Error while creating mstype pool (" << dtypename << ",1)\n";
    }

    err = DTP_pool_create(basic_type, count, &mrtype);
    if (err != DTP_SUCCESS) {
        basic_type.Get_name(dtypename, tnlen);
        cout << "Error while creating mrtype pool (" << dtypename << ",1)\n";
    }

    for (i = 0; i < mstype->DTP_num_objs; i++) {
        err = DTP_obj_create(mstype, i, 0, 1, count);
        if (err != DTP_SUCCESS) {
            errs++;
            break;
        }

        for (j = 0; j < mrtype->DTP_num_objs; j++) {
            err = DTP_obj_create(mrtype, j, 0, 1, count);
            if (err != DTP_SUCCESS) {
                errs++;
                break;
            }

            type = mstype->DTP_obj_array[i].DTP_obj_type;

            // Testing the pack size is tricky, since this is the
            // size that is stored when packed with type.Pack, and
            // is not easily defined.  We look for consistency
            size1 = type.Pack_size(1, comm);
            size2 = type.Pack_size(2, comm);
            if (size1 <= 0 || size2 <= 0) {
                errs++;
                type.Get_name(dtypename, tnlen);
                cout << "Pack size of datatype " << dtypename << " is not positive\n";
            }
            if (size1 >= size2) {
                errs++;
                type.Get_name(dtypename, tnlen);
                cout << "Pack size of 2 of " << dtypename <<
                    " is smaller or the same as the pack size of 1 instance\n";
            }

            if (mrtype->DTP_obj_array[j].DTP_obj_type != mstype->DTP_obj_array[i].DTP_obj_type) {
                type = mrtype->DTP_obj_array[j].DTP_obj_type;

                // Testing the pack size is tricky, since this is the
                // size that is stored when packed with type.Pack, and
                // is not easily defined.  We look for consistency
                size1 = type.Pack_size(1, comm);
                size2 = type.Pack_size(2, comm);
                if (size1 <= 0 || size2 <= 0) {
                    errs++;
                    type.Get_name(dtypename, tnlen);
                    cout << "Pack size of datatype " << dtypename << " is not positive\n";
                }
                if (size1 >= size2) {
                    errs++;
                    type.Get_name(dtypename, tnlen);
                    cout << "Pack size of 2 of " << dtypename <<
                        " is smaller or the same as the pack size of 1 instance\n";
                }
            }
            DTP_obj_free(mrtype, j);
        }
        DTP_obj_free(mstype, i);
    }

    DTP_pool_free(mstype);
    DTP_pool_free(mrtype);

    MTest_Finalize(errs);

    return 0;
}
