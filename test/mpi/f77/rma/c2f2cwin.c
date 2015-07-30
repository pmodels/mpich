/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
 * This file contains the C routines used in testing the c2f and f2c
 * handle conversion functions for MPI_Win
 *
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
#define c2fwin_ C2FWIN
#define f2cwin_ F2CWIN

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define c2fwin_ c2fwin
#define f2cwin_ f2cwin

#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else
#error 'Unrecognized Fortran name mapping'
#endif

/* Prototypes to keep compilers happy */
int c2fwin_(int *);
void f2cwin_(int *);

int c2fwin_(int *win)
{
    MPI_Win cWin = MPI_Win_f2c(*win);
    MPI_Group group, wgroup;
    int result;

    MPI_Win_get_group(cWin, &group);
    MPI_Comm_group(MPI_COMM_WORLD, &wgroup);

    MPI_Group_compare(group, wgroup, &result);
    if (result != MPI_IDENT) {
        fprintf(stderr, "Win: did not get expected group\n");
        return 1;
    }

    MPI_Group_free(&group);
    MPI_Group_free(&wgroup);

    return 0;
}

/*
 * The following routines provide handles to the calling Fortran program
 */
void f2cwin_(int *win)
{
    MPI_Win cWin;
    MPI_Win_create(0, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &cWin);
    *win = MPI_Win_c2f(cWin);
}
