/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* Regression test for MPICH trac ticket #972, originally written by
 * Rob Latham <robl@mcs.anl.gov> as a simplification of a type
 * encountered by the HDF5 library.
 *
 * Should be run with 1 process. */

#include <stdio.h>
#include "mpi.h"
#include "mpitest.h"

/* uncomment to use debugging routine in MPICH
extern int MPIR_Datatype_debug(MPI_Datatype type, int depth);
*/

int makeHDF5type0(MPI_Datatype * type);
int makeHDF5type0(MPI_Datatype * type)
{
    MPI_Datatype ctg, vect, structype, vec2, structype2, vec3, structype3, vec4, structype4, vec5;

    int b;
    MPI_Aint d;
    MPI_Datatype tmp_type;

    MPI_Type_contiguous(4, MPI_BYTE, &ctg);

    MPI_Type_vector(1, 5, 1, ctg, &vect);

    MPI_Type_create_resized(vect, 0, 40, &structype);

    MPI_Type_vector(1, 5, 1, structype, &vec2);

    b = 1;
    d = 2000;
    MPI_Type_create_struct(1, &b, &d, &vec2, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, 400, &structype2);
    MPI_Type_free(&tmp_type);

    MPI_Type_vector(1, 5, 1, structype2, &vec3);

    MPI_Type_create_resized(vec3, 0, 4000, &structype3);

    MPI_Type_vector(1, 5, 1, structype3, &vec4);

    MPI_Type_create_resized(vec4, 0, 40000, &structype4);

    MPI_Type_vector(1, 1, 1, structype4, &vec5);

    b = 1;
    d = 160000;
    MPI_Type_create_struct(1, &b, &d, &vec5, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, 200000, type);
    MPI_Type_free(&tmp_type);

    MPI_Type_free(&ctg);
    MPI_Type_free(&vect);
    MPI_Type_free(&structype);
    MPI_Type_free(&vec2);
    MPI_Type_free(&structype2);
    MPI_Type_free(&vec3);
    MPI_Type_free(&structype3);
    MPI_Type_free(&vec4);
    MPI_Type_free(&structype4);
    MPI_Type_free(&vec5);
    MPI_Type_commit(type);

    return 0;
}

int makeHDF5type1(MPI_Datatype * type);
int makeHDF5type1(MPI_Datatype * type)
{
    MPI_Datatype ctg, vect, structype, vec2, structype2, vec3, structype3, vec4, structype4, vec5;

    int b;
    MPI_Aint d;
    MPI_Datatype tmp_type;

    MPI_Type_contiguous(4, MPI_BYTE, &ctg);

    MPI_Type_vector(1, 5, 1, ctg, &vect);

    b = 1;
    d = 20;
    MPI_Type_create_struct(1, &b, &d, &vect, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, 40, &structype);
    MPI_Type_free(&tmp_type);

    MPI_Type_vector(1, 5, 1, structype, &vec2);

    MPI_Type_create_resized(vec2, 0, 400, &structype2);

    MPI_Type_vector(1, 5, 1, structype2, &vec3);

    MPI_Type_create_resized(vec3, 0, 4000, &structype3);

    MPI_Type_vector(1, 5, 1, structype3, &vec4);

    MPI_Type_create_resized(vec4, 0, 40000, &structype4);

    MPI_Type_vector(1, 1, 1, structype4, &vec5);

    b = 1;
    d = 160000;
    MPI_Type_create_struct(1, &b, &d, &vec5, &tmp_type);
    MPI_Type_create_resized(tmp_type, 0, 200000, type);
    MPI_Type_free(&tmp_type);

    MPI_Type_free(&ctg);
    MPI_Type_free(&vect);
    MPI_Type_free(&structype);
    MPI_Type_free(&vec2);
    MPI_Type_free(&structype2);
    MPI_Type_free(&vec3);
    MPI_Type_free(&structype3);
    MPI_Type_free(&vec4);
    MPI_Type_free(&structype4);
    MPI_Type_free(&vec5);
    MPI_Type_commit(type);

    return 0;
}

int makeHDF5type(MPI_Datatype * type);
int makeHDF5type(MPI_Datatype * type)
{
    int i;

#define NTYPES 2

    int blocklens[NTYPES];
    MPI_Aint disps[NTYPES];

    MPI_Datatype types[NTYPES];
    makeHDF5type0(&(types[0]));
    makeHDF5type1(&(types[1]));

    for (i = 0; i < NTYPES; i++) {
        blocklens[i] = 1;
        disps[i] = 0;
    }

    MPI_Type_create_struct(NTYPES, blocklens, disps, types, type);
    MPI_Type_commit(type);

    for (i = 0; i < NTYPES; i++) {
        MPI_Type_free(&(types[i]));
    }
    return 0;
}

int main(int argc, char **argv)
{
    MPI_Datatype hdf5type;

    MTest_Init(&argc, &argv);
    makeHDF5type(&hdf5type);

    /*MPIR_Datatype_debug(hdf5type, 32); */

    MPI_Type_free(&hdf5type);
    MTest_Finalize(0);

    return 0;
}
