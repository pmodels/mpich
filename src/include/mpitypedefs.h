/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  $Id: mpitypedefs.h,v 1.9 2007/03/12 20:40:39 gropp Exp $
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#if !defined(MPITYPEDEFS_H_INCLUDED)
#define MPITYPEDEFS_H_INCLUDED

#include "mpichconf.h"

/* ------------------------------------------------------------------------- */
/* mpitypedefs.h */
/* ------------------------------------------------------------------------- */
/* Basic typedefs */
#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif

/* */
/*
#ifndef HAVE_UINT8_T 
#ifdef MPIU_UINT8_T
typedef MPIU_UINT8_T uint8_t;
#else
#error 'Configure did not find a 8-bit unsigned integer type'
#endif
#endif

#ifndef HAVE_INT16_T 
#ifdef MPIU_INT16_T
typedef MPIU_INT16_T int16_t;
#else
#error 'Configure did not find a 16-bit integer type'
#endif
#endif

#ifndef HAVE_UINT16_T 
#ifdef MPIU_UINT16_T
typedef MPIU_UINT16_T uint16_t;
#else
#error 'Configure did not find a 16-bit unsigned integer type'
#endif
#endif

#ifndef HAVE_INT32_T
#ifdef MPIU_INT32_T
typedef MPIU_INT32_T int32_t;
#else
#error 'Configure did not find a 32-bit integer type'
#endif
#endif

#ifndef HAVE_UINT32_T 
#ifdef MPIU_UINT32_T
typedef MPIU_UINT32_T uint32_t;
#else
#error 'Configure did not find a 32-bit unsigned integer type'
#endif
#endif
*/
/*
#ifndef HAVE_INT64_T
#ifdef MPIU_INT64_T
typedef MPIU_INT64_T int64_t;
#else
*/
/* Don't define a 64 bit integer type if we didn't find one, but 
   allow the code to compile as long as we don't need that type */
/*
#endif
#endif
*/
/*
#ifndef HAVE_UINT64_T
#ifdef MPIU_UINT64_T
typedef MPIU_UINT64_T uint64_t;
#else
*/
/* Don't define a 64 bit unsigned integer type if we didn't find one, 
   allow the code to compile as long as we don't need the type */
/*
#endif
#endif
*/

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#else
#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
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

#include "mpiiov.h"

typedef MPIU_SIZE_T MPIU_Size_t;

/* Use the MPIU_PtrToXXX macros to convert pointers to and from integer types */

/* The Microsoft compiler will not allow casting of different sized types 
 * without
 * printing a compiler warning.  Using these macros allows compiler specific
 * type casting and avoids the warning output.  These macros should only be used
 * in code that can handle loss of bits.
 */

/* PtrToLong converts a pointer to a long type, truncating bits if necessary */
#ifdef HAVE_PTRTOLONG
#define MPIU_PtrToLong PtrToLong
#else
#define MPIU_PtrToLong(a) (long)(a)
#endif
/* PtrToInt converts a pointer to a int type, truncating bits if necessary */
#ifdef HAVE_PTRTOINT
#define MPIU_PtrToInt PtrToInt
#else
#define MPIU_PtrToInt(a) (int)(a)
#endif
/* PtrToAint converts a pointer to an MPI_Aint type, truncating bits if necessary */
#ifdef HAVE_PTRTOAINT
#define MPIU_PtrToAint(a) ((MPI_Aint)(INT_PTR) (a) )
#else
#define MPIU_PtrToAint(a) (MPI_Aint)(a)
#endif
/* LongToPtr converts a long to a pointer type, extending bits if necessary */
#ifdef HAVE_LONGTOPTR
#define MPIU_LongToPtr LongToPtr
#else
#define MPIU_LongToPtr(a) (void*)(a)
#endif
/* IntToPtr converts a int to a pointer type, extending bits if necessary */
#ifdef HAVE_INTTOPTR
#define MPIU_IntToPtr IntToPtr
#else
#define MPIU_IntToPtr(a) (void*)(a)
#endif
/* AintToPtr converts an MPI_Aint to a pointer type, extending bits if necessary */
#ifdef HAVE_AINTTOPTR
#define MPIU_AintToPtr(a) ((VOID *)(INT_PTR)((MPI_Aint)a))
#else
#define MPIU_AintToPtr(a) (void*)(a)
#endif
/* ------------------------------------------------------------------------- */
/* end of mpitypedefs.h */
/* ------------------------------------------------------------------------- */

#endif /* !defined(MPITYPEDEFS_H_INCLUDED) */
