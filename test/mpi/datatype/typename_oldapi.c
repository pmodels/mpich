/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/* This test is similar to typename.c but checks only MPI_LB and MPI_UB. */

#include "mpi.h"
#include "mpitest.h"
#include <stdio.h>
#include <string.h>

/* Create an array with all of the MPI names in it */

typedef struct mpi_names_t {
    MPI_Datatype dtype;
    const char *name;
} mpi_names_t;

/* The MPI standard specifies that the names must be the MPI names,
   not the related language names (e.g., MPI_CHAR, not char) */

static mpi_names_t mpi_names[] = {
    {MPI_LB, "MPI_LB"},
    {MPI_UB, "MPI_UB"},
    {0, (char *) 0},    /* Sentinel used to indicate the last element */
};

int main(int argc, char **argv)
{
    char name[MPI_MAX_OBJECT_NAME];
    int namelen, i;
    int errs = 0;

    MTest_Init(&argc, &argv);

    /* Sample some datatypes */
    /* See 8.4, "Naming Objects" in MPI-2.  The default name is the same
     * as the datatype name */
    MPI_Type_get_name(MPI_DOUBLE, name, &namelen);
    if (strncmp(name, "MPI_DOUBLE", MPI_MAX_OBJECT_NAME)) {
        errs++;
        fprintf(stderr, "Expected MPI_DOUBLE but got :%s:, namelen %d\n", name, namelen);
    }

    MPI_Type_get_name(MPI_INT, name, &namelen);
    if (strncmp(name, "MPI_INT", MPI_MAX_OBJECT_NAME)) {
        errs++;
        fprintf(stderr, "Expected MPI_INT but got :%s:, namelen %d\n", name, namelen);
    }

    /* Now we try them ALL */
    for (i = 0; mpi_names[i].name != 0; i++) {
        MTestPrintfMsg(10, "Checking type %s\n", mpi_names[i].name);
        name[0] = 0;
        MPI_Type_get_name(mpi_names[i].dtype, name, &namelen);

        if (strncmp(name, mpi_names[i].name, MPI_MAX_OBJECT_NAME)) {
            errs++;
            fprintf(stderr, "Expected %s but got :%s:, namelen %d\n", mpi_names[i].name, name,
                    namelen);
        }
    }

    /* Try resetting the name */
    MPI_Type_set_name(MPI_INT, (char *) "int");
    name[0] = 0;
    MPI_Type_get_name(MPI_INT, name, &namelen);
    if (strncmp(name, "int", MPI_MAX_OBJECT_NAME)) {
        errs++;
        fprintf(stderr, "Expected int but got :%s:, namelen %d\n", name, namelen);
    }
#ifndef HAVE_MPI_INTEGER16
    errs++;
    fprintf(stderr, "MPI_INTEGER16 is not available\n");
#endif

    MTest_Finalize(errs);
    return MTestReturnValue(errs);
}
