/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2003 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"

/*
static char MTEST_Descrip[] = "Test of the sized types, supported in MPI-2";
*/

int main(int argc, char *argv[])
{
    int errs = 0;
    int size;

    MTest_Init(&argc, &argv);

    MPI_Type_size(MPI_REAL4, &size);
    if (size != 4) {
        errs++;
        printf("MPI_REAL4 has size %d\n", size);
    }
    MPI_Type_size(MPI_REAL8, &size);
    if (size != 8) {
        errs++;
        printf("MPI_REAL8 has size %d\n", size);
    }
    if (MPI_REAL16 != MPI_DATATYPE_NULL) {
        MPI_Type_size(MPI_REAL16, &size);
        if (size != 16) {
            errs++;
            printf("MPI_REAL16 has size %d\n", size);
        }
    }

    MPI_Type_size(MPI_COMPLEX8, &size);
    if (size != 8) {
        errs++;
        printf("MPI_COMPLEX8 has size %d\n", size);
    }
    MPI_Type_size(MPI_COMPLEX16, &size);
    if (size != 16) {
        errs++;
        printf("MPI_COMPLEX16 has size %d\n", size);
    }
    if (MPI_COMPLEX32 != MPI_DATATYPE_NULL) {
        MPI_Type_size(MPI_COMPLEX32, &size);
        if (size != 32) {
            errs++;
            printf("MPI_COMPLEX32 has size %d\n", size);
        }
    }

    MPI_Type_size(MPI_INTEGER1, &size);
    if (size != 1) {
        errs++;
        printf("MPI_INTEGER1 has size %d\n", size);
    }
    MPI_Type_size(MPI_INTEGER2, &size);
    if (size != 2) {
        errs++;
        printf("MPI_INTEGER2 has size %d\n", size);
    }
    MPI_Type_size(MPI_INTEGER4, &size);
    if (size != 4) {
        errs++;
        printf("MPI_INTEGER4 has size %d\n", size);
    }
    if (MPI_INTEGER8 != MPI_DATATYPE_NULL) {
        MPI_Type_size(MPI_INTEGER8, &size);
        if (size != 8) {
            errs++;
            printf("MPI_INTEGER8 has size %d\n", size);
        }
    }
#ifdef HAVE_MPI_INTEGER16
    if (MPI_INTEGER16 != MPI_DATATYPE_NULL) {
        MPI_Type_size(MPI_INTEGER16, &size);
        if (size != 16) {
            errs++;
            printf("MPI_INTEGER16 has size %d\n", size);
        }
    }
#endif

    MTest_Finalize(errs);
    MPI_Finalize();
    return 0;
}
