/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

/* style: allow:fprintf:21 sig:0 */

#include "mpi_fortimpl.h"

#if defined(HAVE_PRAGMA_WEAK) && defined(HAVE_MULTIPLE_PRAGMA_WEAK)
void mpir_is_bottom_( void *a, int *ierr );
void mpir_is_in_place_( void *a, int *ierr );
/* FIXME probably MPI_WEIGHTS_EMPTY needs support somewhere in this file */
void mpir_is_unweighted_( void *a, int *ierr );
void mpir_is_status_ignore_( void *a, int *ierr );
void mpir_is_statuses_ignore_( void *a, int *ierr );
void mpir_is_errcodes_ignore_( void *a, int *ierr );
void mpir_is_argvs_null_( void *a, int *ierr );

extern void MPIR_IS_BOTTOM( void *a, int *ierr );
extern void mpir_is_bottom( void *a, int *ierr );
extern void mpir_is_bottom__( void *a, int *ierr );
extern void MPIR_IS_IN_PLACE( void *a, int *ierr );
extern void mpir_is_in_place( void *a, int *ierr );
extern void mpir_is_in_place__( void *a, int *ierr );
extern void MPIR_IS_UNWEIGHTED( void *a, int *ierr );
extern void mpir_is_unweighted( void *a, int *ierr );
extern void mpir_is_unweighted__( void *a, int *ierr );
extern void MPIR_IS_STATUS_IGNORE( void *a, int *ierr );
extern void mpir_is_status_ignore( void *a, int *ierr );
extern void mpir_is_status_ignore__( void *a, int *ierr );
extern void MPIR_IS_STATUSES_IGNORE( void *a, int *ierr );
extern void mpir_is_statuses_ignore( void *a, int *ierr );
extern void mpir_is_statuses_ignore__( void *a, int *ierr );
extern void MPIR_IS_ERRCODES_IGNORE( void *a, int *ierr );
extern void mpir_is_errcodes_ignore( void *a, int *ierr );
extern void mpir_is_errcodes_ignore__( void *a, int *ierr );
extern void MPIR_IS_ARGVS_NULL( void *a, int *ierr );
extern void mpir_is_argvs_null( void *a, int *ierr );
extern void mpir_is_argvs_null__( void *a, int *ierr );

  #pragma weak MPIR_IS_BOTTOM   = mpir_is_bottom_
  #pragma weak mpir_is_bottom   = mpir_is_bottom_
  #pragma weak mpir_is_bottom__ = mpir_is_bottom_
  #pragma weak MPIR_IS_IN_PLACE   = mpir_is_in_place_
  #pragma weak mpir_is_in_place   = mpir_is_in_place_
  #pragma weak mpir_is_in_place__ = mpir_is_in_place_
  #pragma weak MPIR_IS_UNWEIGHTED   = mpir_is_unweighted_
  #pragma weak mpir_is_unweighted   = mpir_is_unweighted_
  #pragma weak mpir_is_unweighted__ = mpir_is_unweighted_
  #pragma weak MPIR_IS_STATUS_IGNORE   = mpir_is_status_ignore_
  #pragma weak mpir_is_status_ignore   = mpir_is_status_ignore_
  #pragma weak mpir_is_status_ignore__ = mpir_is_status_ignore_
  #pragma weak MPIR_IS_STATUSES_IGNORE   = mpir_is_statuses_ignore_
  #pragma weak mpir_is_statuses_ignore   = mpir_is_statuses_ignore_
  #pragma weak mpir_is_statuses_ignore__ = mpir_is_statuses_ignore_
  #pragma weak MPIR_IS_ERRCODES_IGNORE   = mpir_is_errcodes_ignore_
  #pragma weak mpir_is_errcodes_ignore   = mpir_is_errcodes_ignore_
  #pragma weak mpir_is_errcodes_ignore__ = mpir_is_errcodes_ignore_
  #pragma weak MPIR_IS_ARGVS_NULL   = mpir_is_argvs_null_
  #pragma weak mpir_is_argvs_null   = mpir_is_argvs_null_
  #pragma weak mpir_is_argvs_null__ = mpir_is_argvs_null_
#else
  #if defined(F77_NAME_UPPER)
    #define mpir_is_bottom_ MPIR_IS_BOTTOM
    #define mpir_is_in_place_ MPIR_IS_IN_PLACE
    #define mpir_is_unweighted_ MPIR_IS_UNWEIGHTED
    #define mpir_is_status_ignore_ MPIR_IS_STATUS_IGNORE
    #define mpir_is_statuses_ignore_ MPIR_IS_STATUSES_IGNORE
    #define mpir_is_errcodes_ignore_ MPIR_IS_ERRCODES_IGNORE
    #define mpir_is_argvs_null_ MPIR_IS_ARGVS_NULL
  #elif defined(F77_NAME_LOWER_2USCORE)
    #define mpir_is_bottom_ mpir_is_bottom__
    #define mpir_is_in_place_ mpir_is_in_place__
    #define mpir_is_unweighted_ mpir_is_unweighted__
    #define mpir_is_status_ignore_ mpir_is_status_ignore__
    #define mpir_is_statuses_ignore_ mpir_is_statuses_ignore__
    #define mpir_is_errcodes_ignore_ mpir_is_errcodes_ignore__
    #define mpir_is_argvs_null_ mpir_is_argvs_null__
  #elif defined(F77_NAME_LOWER)
    #define mpir_is_bottom_ mpir_is_bottom
    #define mpir_is_in_place_ mpir_is_in_place
    #define mpir_is_unweighted_ mpir_is_unweighted
    #define mpir_is_status_ignore_ mpir_is_status_ignore
    #define mpir_is_statuses_ignore_ mpir_is_statuses_ignore
    #define mpir_is_errcodes_ignore_ mpir_is_errcodes_ignore
    #define mpir_is_argvs_null_ mpir_is_argvs_null
  #endif

void mpir_is_bottom_( void *a, int *ierr );
void mpir_is_in_place_( void *a, int *ierr );
void mpir_is_unweighted_( void *a, int *ierr );
void mpir_is_status_ignore_( void *a, int *ierr );
void mpir_is_statuses_ignore_( void *a, int *ierr );
void mpir_is_errcodes_ignore_( void *a, int *ierr );
void mpir_is_argvs_null_( void *a, int *ierr );

#endif

#include <stdio.h>

/* --BEGIN DEBUG-- */
/*
   Define Fortran functions MPIR_IS_<NAME>() that are callable in Fortran
   to check if the Fortran constants, MPI_<NAME>, are recognized by the MPI
   implementation (in C library).
*/
void mpir_is_bottom_( void *a, int *ierr )
{
     *ierr = ( a == MPIR_F_MPI_BOTTOM ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPIR_F_MPI_BOTTOM=%p, MPI_BOTTOM=%p\n",
             MPIR_F_MPI_BOTTOM, a);
}

void mpir_is_in_place_( void *a, int *ierr )
{
     *ierr = ( a == MPIR_F_MPI_IN_PLACE ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPIR_F_MPI_IN_PLACE=%p, MPI_IN_PLACE=%p\n",
             MPIR_F_MPI_IN_PLACE, a);
}

void mpir_is_unweighted_( void *a, int *ierr )
{
     *ierr = ( a == MPIR_F_MPI_UNWEIGHTED ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPIR_F_MPI_UNWEIGHTED=%p, MPI_UNWEIGHTED=%p\n",
             MPIR_F_MPI_UNWEIGHTED, a);
}

void mpir_is_status_ignore_( void *a, int *ierr )
{
     *ierr = ( a == MPI_F_STATUS_IGNORE ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPI_F_STATUS_IGNORE=%p, MPI_STATUS_IGNORE=%p\n",
             MPI_F_STATUS_IGNORE, a);
}

void mpir_is_statuses_ignore_( void *a, int *ierr )
{
     *ierr = ( a == MPI_F_STATUSES_IGNORE ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPI_F_STATUSES_IGNORE=%p, MPI_STATUSES_IGNORE=%p\n",
             MPI_F_STATUSES_IGNORE, a);
}

void mpir_is_errcodes_ignore_( void *a, int *ierr )
{
     *ierr = ( a == MPI_F_ERRCODES_IGNORE ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPI_F_ERRCODES_IGNORE=%p, MPI_ERRCODES_IGNORE=%p\n",
             MPI_F_ERRCODES_IGNORE, a);
}

void mpir_is_argvs_null_( void *a, int *ierr )
{
     *ierr = ( a == MPI_F_ARGVS_NULL ? 1 : 0 );
     if ( *ierr )
         fprintf(stderr, "Matched : ");
     else
         fprintf(stderr, "Not matched : ");
     fprintf(stderr,"MPI_F_ARGVS_NULL=%p, MPI_ARGVS_NULL=%p\n",
             MPI_F_ARGVS_NULL, a);
}

/* --END DEBUG-- */
