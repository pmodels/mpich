/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPIU_TYPE_DEFS_H_INCLUDED)
#define MPIU_TYPE_DEFS_H_INCLUDED

#include "mpichconf.h"

/* Basic typedefs */
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif

/* inttypes.h is supposed to include stdint.h but this is here as
   belt-and-suspenders for platforms that aren't fully compliant */
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

/* stdint.h gives us fixed-width C99 types like int16_t, among others */
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

/* stdbool.h gives us the C boolean type */
#ifdef HAVE_STDBOOL_H
#include <stdbool.h>
#endif

/* complex.h gives us the C complex type */
#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else
#ifndef BOOL
#define BOOL int
#endif
#endif
/* Use this macro for each parameter to a function that is not referenced in
   the body of the function */
#ifdef HAVE_WINDOWS_H
#define MPIU_UNREFERENCED_ARG(a) a
#else
#define MPIU_UNREFERENCED_ARG(a)
#endif

#define MPIU_MAX(a,b)    (((a) > (b)) ? (a) : (b))
#define MPIU_MIN(a,b)    (((a) < (b)) ? (a) : (b))

#include "mpl.h"

typedef MPIU_SIZE_T MPIU_Size_t;

/* Use the MPIU_PtrToXXX macros to convert pointers to and from integer types */

/* The Microsoft compiler will not allow casting of different sized types
 * without
 * printing a compiler warning.  Using these macros allows compiler specific
 * type casting and avoids the warning output.  These macros should only be used
 * in code that can handle loss of bits.
 */

/* PtrToAint converts a pointer to an MPI_Aint type, truncating bits if necessary */
#ifdef HAVE_PTRTOAINT
#define MPIU_PtrToAint(a) ((MPI_Aint)(INT_PTR) (a))
#else
/* An MPI_Aint may be *larger* than a pointer.  By using 2 casts, we can
   keep some compilers from complaining about converting a pointer to an
   integer of a different size */
#define MPIU_PtrToAint(a) ((MPI_Aint)(MPIU_Upint)(a))
#endif

/* AintToPtr converts an MPI_Aint to a pointer type, extending bits if necessary */
#ifdef HAVE_AINTTOPTR
#define MPIU_AintToPtr(a) ((VOID *)(INT_PTR)((MPI_Aint)a))
#else
#define MPIU_AintToPtr(a) (void*)(a)
#endif

/* Adding the 32-bit compute/64-bit I/O related type-casts in here as
 * they are not a part of the MPI standard yet. */
#define MPIU_AINT_CAST_TO_VOID_PTR (void *)(MPIU_Pint)
#define MPIU_VOID_PTR_CAST_TO_MPI_AINT (MPI_Aint)(MPIU_Upint)
#define MPIU_PTR_DISP_CAST_TO_MPI_AINT (MPI_Aint)(MPIU_Pint)

#define MPIU_CONTEXT_ID_T_DATATYPE MPI_UINT16_T
typedef uint16_t MPIU_Context_id_t;
#define MPIU_INVALID_CONTEXT_ID ((MPIU_Context_id_t)0xffff)

#endif /* !defined(MPIU_TYPE_DEFS_H_INCLUDED) */
