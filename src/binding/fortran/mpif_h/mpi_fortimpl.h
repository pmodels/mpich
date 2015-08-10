/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Portions of this code were written by Microsoft. Those portions are
 * Copyright (c) 2007 Microsoft Corporation. Microsoft grants
 * permission to use, reproduce, prepare derivative works, and to
 * redistribute to others. The code is licensed "as is." The User
 * bears the risk of using it. Microsoft gives no express warranties,
 * guarantees or conditions. To the extent permitted by law, Microsoft
 * excludes the implied warranties of merchantability, fitness for a
 * particular purpose and non-infringement.
 */
#include "mpichconf.h"

/* Handle different mechanisms for passing Fortran CHARACTER to routines.
 * 
 * In the case where MPI_Fint is a different size from int, it appears that
 * compilers use an int, rather than an MPI_Fint, for the length.  However,
 * there is no standard for this, so some compiler may choose to use
 * an MPI_Fint instead of an int.  In that case, we will need an additional
 * case.
 */
/*
 * Many Fortran compilers at some point changed from passing the length of
 * a CHARACTER*(N) in an int to using a size_t.  This definition allows
 * us to select either size_t or int.  Determining which one the compiler
 * is using may require reading the documentation or examining the generated
 * code.
 *
 * The default is left as "int" because that is the correct legacy choice.
 * Configure may need to check the compiler vendor and version to decide
 * whether to select size_t.  And despite documentation to the contrary,
 * in our experiments, gfortran used int instead of size_t, which was
 * verified by inspecting the assembly code.
 */
#ifdef USE_FORT_STR_LEN_SIZET
#define FORT_SIZE_INT size_t
#else
#define FORT_SIZE_INT int
#endif

#ifdef USE_FORT_MIXED_STR_LEN
#define FORT_MIXED_LEN_DECL   , FORT_SIZE_INT
#define FORT_END_LEN_DECL
#define FORT_MIXED_LEN(a)     , FORT_SIZE_INT a
#define FORT_END_LEN(a)
#else
#define FORT_MIXED_LEN_DECL
#define FORT_END_LEN_DECL     , FORT_SIZE_INT
#define FORT_MIXED_LEN(a)
#define FORT_END_LEN(a)       , FORT_SIZE_INT a
#endif

/* ------------------------------------------------------------------------- */
/* The following definitions are used to support the Microsoft compilers

   The following C preprocessor macros are not discoved by configure.  
   Instead, they must be defined separately; this is normally done as part of 
   the Windows-specific configuration process.

   USE_FORT_STDCALL - Use __stdcall for the calling convention
   USE_FORT_CDECL   - Use __cdelc for the calling convention
       These define the macro FORT_CALL ; for other systems, FORT_CALL is 
       empty

       Note: It may be that these should be USE_MSC_xxx to indicate that
       these can only be used with the MS C compiler.

   USE_MSC_DLLSPEC - Use __declspec to control the import or export of
                     symbols in a generated dynamic link library (DLL)
       This defines the macros FORT_DECL_SPEC and FORT_C_DECL_SPEC ; for 
       other systems, these names both expand to empty
       If USE_MSC_DLLSPEC is defined, then the macros FORTRAN_EXPORTS and
       FORTRAN_FROM_C_EXPORTS controls whether dllexport or dllimport is 
       specified.       

       The name (USE_MSC_xxx) here indicates that the MS C compiler is 
       required for this.

 */

/* Support Windows extension to specify calling convention */
#ifdef USE_FORT_CDECL
#define FORT_CALL __cdecl
#elif defined (USE_FORT_STDCALL)
#define FORT_CALL __stdcall
#else
#define FORT_CALL
#endif

/* Support Windows extension to specify which functions are exported from
   shared (DLL) libraries */
/* Backward compatibility (was HAVE_FORTRAN_API) */
#if defined(HAVE_FORTRAN_API) && !defined(USE_MSC_DLLSPEC)
#define USE_MSC_DLLSPEC
#endif

#ifdef USE_MSC_DLLSPEC
# ifdef FORTRAN_EXPORTS
#  define FORT_DLL_SPEC __declspec(dllexport)
# else
#  define FORT_DLL_SPEC __declspec(dllimport)
# endif
#else
# define FORT_DLL_SPEC
#endif


/* ------------------------------------------------------------------------- */

/* Support an alternate approach for declaring a weak symbol supported by
   some versions of gcc */
#ifdef USE_WEAK_ATTRIBUTE
#define FUNC_ATTRIBUTES(name) __attribute__ ((weak,alias(#name)))
#else
#define FUNC_ATTRIBUTES(name)
#endif

/* ------------------------------------------------------------------------- */

/* mpi.h includes the definitions of MPI_Fint */
#include "mpi.h"
#include "mpiutil.h"

/* Include prototypes of helper functions.
   These include MPIR_Keyval_set_fortran, fortran90, and 
   Grequest_set_lang_f77 */
#include "mpi_f77interface.h"
/* Include the attribute access routines that permit access to the 
   attribute or its pointer, needed for cross-language access to attributes */
#include "mpi_attr.h"

/* mpi_lang.h - Prototypes for language specific routines. Currently used to
 * set keyval attribute callbacks
 */
#include "mpi_lang.h"

/* If there is no MPI I/O support, and we are still using MPIO_Request,
   make sure that one is defined */
#ifndef MPIO_REQUEST_DEFINED
#define MPIO_Request MPI_Request
#endif

/* MPI_FAint is used as the C type corresponding to the Fortran type 
   used for addresses.  For now, we make this the same as MPI_Aint.  
   Note that since this is defined only for this private include file,
   we can get away with calling MPI_xxx */
typedef MPI_Aint MPI_FAint;

/* Utility functions */

/* Define the internal values needed for Fortran support */

/* Fortran logicals */
/* The definitions for the Fortran logical values are also needed 
   by the reduction operations in mpi/coll/opland, oplor, and oplxor, 
   so they are defined in src/include/mpi_fortlogical.h */
#include "mpi_fortlogical.h"


/* MPIR_F_MPI_BOTTOM is the address of the Fortran MPI_BOTTOM value */
extern FORT_DLL_SPEC int  MPIR_F_NeedInit;
extern FORT_DLL_SPEC void *MPIR_F_MPI_BOTTOM;
extern FORT_DLL_SPEC void *MPIR_F_MPI_IN_PLACE;
extern FORT_DLL_SPEC void *MPIR_F_MPI_UNWEIGHTED;
extern FORT_DLL_SPEC void *MPIR_F_MPI_WEIGHTS_EMPTY;
/* MPI_F_STATUS(ES)_IGNORE are defined in mpi.h and are intended for C 
   programs. */
/*
extern FORT_DLL_SPEC MPI_Fint *MPI_F_STATUS_IGNORE;
extern FORT_DLL_SPEC MPI_Fint *MPI_F_STATUSES_IGNORE;
*/
/* MPI_F_ERRCODES_IGNORE is defined as a Fortran INTEGER type, so must 
   be declared as MPI_Fint */
extern FORT_DLL_SPEC MPI_Fint  *MPI_F_ERRCODES_IGNORE;
extern FORT_DLL_SPEC void *MPI_F_ARGVS_NULL;
/* MPIR_F_PTR checks for the Fortran MPI_BOTTOM and provides the value 
   MPI_BOTTOM if found 
   See src/pt2pt/addressf.c for why MPIR_F_PTR(a) is just (a)
*/
/*  #define MPIR_F_PTR(a) (((a)==(MPIR_F_MPI_BOTTOM))?MPI_BOTTOM:a) */
#define MPIR_F_PTR(a) (a)

/* Define the name of the function that we call to initialize the
   special symbols */
#if defined(F77_NAME_LOWER_USCORE) || defined(F77_NAME_LOWER_2USCORE)
/* do nothing */
#elif defined(F77_NAME_UPPER)
#define mpirinitf_ MPIRINITF
#else
#define mpirinitf_ mpirinitf
#endif
/* Provide a prototype for the mpirinitf function */
extern void mpirinitf_( void );

/*  
 * These are hooks for Fortran characters.
 * MPID_FCHAR_T is the type of a Fortran character argument
 * MPID_FCHAR_LARG is the "other" argument that some Fortran compilers use
 * MPID_FCHAR_STR gives the pointer to the characters
 */
#ifdef MPID_CHARACTERS_ARE_CRAYPVP
typedef <whatever> MPID_FCHAR_T;
#define MPID_FCHAR_STR(a) (a)->characters   <or whatever>
#define MPID_FCHAR_LARG(d) 
#else
typedef char *MPID_FCHAR_T;
#define MPID_FCHAR_STR(a) a
#define MPID_FCHAR_LARG(d) ,d
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

/* Temporary patch for the space routines.  Eventually, this should use
   (FIXME) *just* the memory definitions currently in mpiimpl.h */
/* style: allow:malloc:1 sig:0 */
/* style: allow:free:1 sig:0 */
/* style: allow:calloc:1 sig:0 */
#ifndef MPIU_Malloc
#define MPIU_Malloc(a)    malloc((unsigned)(a))
#define MPIU_Calloc(a,b)  calloc((unsigned)(a),(unsigned)(b))
#define MPIU_Free(a)      free((void *)(a))
#endif

/* To avoid constant allocation/deallocation of temporary arrays, define
   a small default, predefined array size. */
#ifndef MPIR_USE_LOCAL_ARRAY
#define MPIR_USE_LOCAL_ARRAY 32
#endif

/* Undefine the names that are used in mpi.h for the predefined keyval
   copy and delete functions.  This is necessary in case the Fortran 
   compiler uses uppercase names, because in that case there would be 
   a conflict in the names */
#ifdef MPI_DUP_FN
#undef MPI_DUP_FN
#undef MPI_NULL_COPY_FN
#undef MPI_NULL_DELETE_FN
#undef MPI_COMM_NULL_COPY_FN
#undef MPI_COMM_NULL_DELETE_FN
#undef MPI_COMM_DUP_FN
#undef MPI_TYPE_NULL_COPY_FN
#undef MPI_TYPE_NULL_DELETE_FN
#undef MPI_TYPE_DUP_FN
#undef MPI_WIN_NULL_COPY_FN
#undef MPI_WIN_NULL_DELETE_FN
#undef MPI_WIN_DUP_FN

/* Ditto the null datarep conversion */
#undef MPI_CONVERSION_FN_NULL
#endif /* MPI_DUP_FN */

/* A special case to help out when ROMIO is disabled */
#ifndef MPI_MODE_RDONLY
#ifndef MPI_File_f2c
#define MPI_File_f2c(a) ((MPI_File)(MPI_Aint)(a))
#endif
#endif /* MPI_MODE_RDONLY */
