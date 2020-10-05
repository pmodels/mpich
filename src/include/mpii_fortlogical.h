/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPII_FORTLOGICAL_H_INCLUDED
#define MPII_FORTLOGICAL_H_INCLUDED

#if !defined(F77_TRUE_VALUE_SET)
#define F77_TRUE_VALUE 1
#define F77_FALSE_VALUE 0
#define F77_TRUE_VALUE_SET
#endif

/* We originally support F77_RUNTIME_VALUES, which I believe it is the
 * reason we require MPII_F_TRUE/FALSE to be global variables.
 * Now that F77_RUNTIME_VALUES support has disappeared, let's remove the
 * global variables and redefine them as boolearn literal. This will remove
 * the complication of resolving symbols between libmpi.so and libmpifort.so.
 */

#define MPII_F_TRUE F77_TRUE_VALUE
#define MPII_F_FALSE F77_FALSE_VALUE

/* Fortran logical values */
#ifndef _CRAY

#define MPII_TO_FLOG(a) ((a) ? MPII_F_TRUE : MPII_F_FALSE)
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
#define MPII_FROM_FLOG(a) ((a) == MPII_F_FALSE ? 0 : 1)

#else
/* CRAY Vector processors only; these are defined in /usr/include/fortran.h
   Thanks to lmc@cray.com */
#define MPII_TO_FLOG(a) (_btol(a))
#define MPII_FROM_FLOG(a) (_ltob(&(a))) /* (a) must be a pointer */
#endif

#endif /* MPII_FORTLOGICAL_H_INCLUDED */
