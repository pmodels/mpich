/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPII_FORTLOGICAL_H_INCLUDED
#define MPII_FORTLOGICAL_H_INCLUDED

/* Fortran logical values */
#ifndef _CRAY

extern int MPIR_fortran_true;
extern int MPIR_fortran_false;

#define MPII_TO_FLOG(a) ((a) ? MPIR_fortran_true : MPIR_fortran_false)
/*
   Note on true and false.  This code is only an approximation.
   Some systems define either true or false, and allow some or ALL other
   patterns for the other.  This is just like C, where 0 is false and
   anything not zero is true.  Modify this test as necessary for your
   system.

   We check against FALSE instead of TRUE because many (perhaps all at this
   point) Fortran compilers use 0 for false and some non-zero value for
   true.  By using this test, it is possible to use the same Fortran
   interface library for multiple compilers that differ only in the
   value used for Fortran .TRUE. .
 */
#define MPII_FROM_FLOG(a) ((a) == MPIR_fortran_false ? 0 : 1)

#else
/* CRAY Vector processors only; these are defined in /usr/include/fortran.h
   Thanks to lmc@cray.com */
#define MPII_TO_FLOG(a) (_btol(a))
#define MPII_FROM_FLOG(a) (_ltob(&(a))) /* (a) must be a pointer */
#endif

#endif /* MPII_FORTLOGICAL_H_INCLUDED */
