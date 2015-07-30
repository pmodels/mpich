/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* Regression test for MPICH trac ticket #972, originally written by
 * Rob Latham <robl@mcs.anl.gov> as a simplification of a type
 * encountered by the HDF5 library.
 *
 * Should be run with 1 process. */

#include <stdio.h>
#include "mpi.h"

/* uncomment to use debugging routine in MPICH
extern int MPIDU_Datatype_debug(MPI_Datatype type, int depth);
*/

int makeHDF5type0(MPI_Datatype * type);
int makeHDF5type0(MPI_Datatype * type)
{
    MPI_Datatype ctg, vect, structype, vec2, structype2, vec3, structype3, vec4, structype4, vec5;

    int b[3];
    MPI_Aint d[3];
    MPI_Datatype t[3];

    MPI_Type_contiguous(4, MPI_BYTE, &ctg);

    MPI_Type_vector(1, 5, 1, ctg, &vect);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 40;
    t[0] = MPI_LB;
    t[1] = vect;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype);

    MPI_Type_vector(1, 5, 1, structype, &vec2);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 2000;
    d[2] = 400;
    t[0] = MPI_LB;
    t[1] = vec2;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype2);

    MPI_Type_vector(1, 5, 1, structype2, &vec3);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 4000;
    t[0] = MPI_LB;
    t[1] = vec3;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype3);

    MPI_Type_vector(1, 5, 1, structype3, &vec4);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 40000;
    t[0] = MPI_LB;
    t[1] = vec4;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype4);

    MPI_Type_vector(1, 1, 1, structype4, &vec5);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 160000;
    d[2] = 200000;
    t[0] = MPI_LB;
    t[1] = vec5;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, type);

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

    int b[3];
    MPI_Aint d[3];
    MPI_Datatype t[3];

    MPI_Type_contiguous(4, MPI_BYTE, &ctg);

    MPI_Type_vector(1, 5, 1, ctg, &vect);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 20;
    d[2] = 40;
    t[0] = MPI_LB;
    t[1] = vect;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype);

    MPI_Type_vector(1, 5, 1, structype, &vec2);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 400;
    t[0] = MPI_LB;
    t[1] = vec2;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype2);

    MPI_Type_vector(1, 5, 1, structype2, &vec3);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 4000;
    t[0] = MPI_LB;
    t[1] = vec3;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype3);

    MPI_Type_vector(1, 5, 1, structype3, &vec4);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 0;
    d[2] = 40000;
    t[0] = MPI_LB;
    t[1] = vec4;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, &structype4);

    MPI_Type_vector(1, 1, 1, structype4, &vec5);

    b[0] = b[1] = b[2] = 1;
    d[0] = 0;
    d[1] = 160000;
    d[2] = 200000;
    t[0] = MPI_LB;
    t[1] = vec5;
    t[2] = MPI_UB;
    MPI_Type_create_struct(3, b, d, t, type);

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

    MPI_Init(&argc, &argv);
    makeHDF5type(&hdf5type);

    /*MPIDU_Datatype_debug(hdf5type, 32); */

    MPI_Type_free(&hdf5type);
    MPI_Finalize();

    printf(" No Errors\n");

    return 0;
}
