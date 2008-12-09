/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* style: allow:fprintf:10 sig:0 */
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
#define chkinc_ CHKINC

#elif defined(F77_NAME_LOWER) || defined(F77_NAME_MIXED)
/* Mixed is ok because we use lowercase in all uses */
#define chkinc_ chkinc
#elif defined(F77_NAME_LOWER_2USCORE) || defined(F77_NAME_LOWER_USCORE) || \
      defined(F77_NAME_MIXED_USCORE)
/* Else leave name alone (routines have no underscore, so both
   of these map to a lowercase, single underscore) */
#else 
#error 'Unrecognized Fortran name mapping'
#endif

/* Prototypes to keep compilers happy */
void chkinc( int *, const int *, int * );

int chkinc_ (int *keyval, int *expected, int *ierr)
{
    int      flag;
    MPI_Aint *val;

    MPI_Comm_get_attr( MPI_COMM_WORLD, *keyval, &val, &flag );
    if (!flag) {
	*ierr = 1;
    }
    else {
	if (*val != *expected) {
	    *ierr = *ierr + 1;
	}
    }
}

