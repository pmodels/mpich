/* This file created from test/mpi/f77/io/c2f2cio.c with f77tof90 */
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This file contains the C routines used in testing the c2f and f2c
 * handle conversion functions for MPI_File
 * The tests follow this pattern:
 *
 *  Fortran main program
 *     calls c routine with each handle type, with a prepared
 *     and valid handle (often requires constructing an object)
 *
 *     C routine uses xxx_f2c routine to get C handle, checks some
 *     properties (i.e., size and rank of communicator, contents of datatype)
 *
 *     Then the Fortran main program calls a C routine that provides
 *     a handle, and the Fortran program performs similar checks.
 *
 * We also assume that a C int is a Fortran integer.  If this is not the
 * case, these tests must be modified.
 */

/* style: allow:fprintf:1 sig:0 */
#include <stdio.h>
#include "mpi.h"
#include "../../include/mpitestconf.h"
#include <string.h>

/*
   Name mapping.  All routines are created with names that are lower case
   with a single trailing underscore.  This matches many compilers.
   We use #define to change the name for Fortran compilers that do
   not use the lowercase/underscore pattern
*/

#ifdef F77_NAME_UPPER
#define c2ffile_ C2FFILE
#define f2cfile_ F2CFILE

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define c2ffile_ c2ffile
#define f2cfile_ f2cfile

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else
#error 'Unrecognized Fortran name mapping'
#endif

/* Prototypes to keep compilers happy */
int c2ffile_(int *);
void f2cfile_(int *);

int c2ffile_(int *file)
{
    MPI_File cFile = MPI_File_f2c(*file);
    MPI_Group group, wgroup;
    int result;

    MPI_File_get_group(cFile, &group);
    MPI_Comm_group(MPI_COMM_WORLD, &wgroup);

    MPI_Group_compare(group, wgroup, &result);
    if (result != MPI_IDENT) {
        fprintf(stderr, "File: did not get expected group\n");
        return 1;
    }

    MPI_Group_free(&group);
    MPI_Group_free(&wgroup);
    return 0;
}

/*
 * The following routines provide handles to the calling Fortran program
 */
void f2cfile_(int *file)
{
    MPI_File cFile;
    int rc;
    rc = MPI_File_open(MPI_COMM_WORLD, (char *) "temp",
                       MPI_MODE_RDWR | MPI_MODE_DELETE_ON_CLOSE | MPI_MODE_CREATE,
                       MPI_INFO_NULL, &cFile);
    if (rc) {
        *file = 0;
    }
    else {
        *file = MPI_File_c2f(cFile);
    }
}
