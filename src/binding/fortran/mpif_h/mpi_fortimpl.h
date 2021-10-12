/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPI_FORTIMPL_H_INCLUDED
#define MPI_FORTIMPL_H_INCLUDED

#include "mpichconf.h"
#include "mpi.h"
#include "mpir_attr_generic.h"
#include "mpii_f77interface.h"
#include <sys/types.h>  /* for ssize_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* NOTE: both leading and trailing spaces are not counted */
static inline int get_fort_str_len(char *s, int len)
{
    char *p = s + len - 1;
    while (*p == ' ' && p > s) {
        p--;
    }
    while (s < p && *s == ' ') {
        s++;
    }

    if (p == s && *s == ' ') {
        return 0;
    } else {
        return (int) (p + 1 - s);
    }
}

/* NOTE: n is returned from get_fort_str_len.
 *       s may include leading spaces that will be removed.
 */
static inline void copy_fort_str_to_c(char *c_str, char *s, int n)
{
    if (n > 0) {
        while (*s == ' ') {
            s++;
        }
        memcpy(c_str, s, n);
    }
    c_str[n] = '\0';
}

/* duplicate fortran string to a C string */
static inline char *MPIR_fort_dup_str(char *s, int len)
{
    int n = get_fort_str_len(s, len);
    char *c_str = (char *) malloc(n + 1);
    copy_fort_str_to_c(c_str, s, n);

    return c_str;
}

/* duplicate fortran string array (e.g. argv in MPI_Comm_spawn)
 *
 * If count > 0, we have count strings, e.g. array_of_commands.
 * Otherwise, the array is terminated by an empty/NULL string.
 *
 * When we assume the array is NULL terminated, user mistakes can easily become
 * undefined behavior. But I guess that is not much we can do.
 */
static inline char **MPIR_fort_dup_str_array(char *s, int len, int stride, int count)
{
    int num_strs;

    if (count > 0) {
        num_strs = count;
    } else {
        /* Compute the size of the array by looking for an all-blank line */
        num_strs = 0;
        char *p = s;
        while (1) {
            int n = get_fort_str_len(p, len);
            if (n == 0) {
                break;
            }
            num_strs++;
            p += stride;
        }
        count = num_strs;
        /* add terminating string */
        num_strs++;
    }

    char **strs = (char **) malloc(num_strs * sizeof(char *));

    if (count > 0) {
        char *str = (char *) malloc(num_strs * (len + 1));

        for (int i = 0; i < count; i++) {
            char *p1 = s + i * stride;
            char *p2 = str + i * (len + 1);

            int n = get_fort_str_len(p1, len);
            copy_fort_str_to_c(p2, p1, n);

            strs[i] = p2;
        }
    }

    if (num_strs > count) {
        /* Null terminate the array */
        strs[count] = NULL;
    }

    return strs;
}

static inline void MPIR_fort_free_str_array(char **strs)
{
    if (strs[0]) {
        free(strs[0]);
    }
    free(strs);
}

/* duplicate fortran string 2d array (e.g. array_of_argv in MPI_Comm_spawn_multiple) */
static inline char ***MPIR_fort_dup_str_2d_array(char *s, int len, int count)
{
    char ***str_2d_array = malloc(count * sizeof(char **));
    for (int k = 0; k < count; k++) {
        char *p = s + k * len;  /* NOTE: column-major */

        str_2d_array[k] = MPIR_fort_dup_str_array(p, len, count * len, 0);
    }

    return str_2d_array;
}

static inline void MPIR_fort_free_str_2d_array(char ***str_2d_array, int count)
{
    for (int i = 0; i < count; i++) {
        MPIR_fort_free_str_array(str_2d_array[i]);
    }
    free(str_2d_array);
}

static inline void MPIR_fort_copy_str_from_c(char *s, int len, char *c_str)
{
    int n = strlen(c_str);
    if (n > len) {
        n = len;
    }

    memcpy(s, c_str, n);
    for (int i = n; i < len; i++) {
        s[i] = ' ';
    }
}

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
#ifdef FORTRAN_EXPORTS
#define FORT_DLL_SPEC __declspec(dllexport)
#else
#define FORT_DLL_SPEC __declspec(dllimport)
#endif
#else
#define FORT_DLL_SPEC MPICH_API_PUBLIC
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
   so they are defined in src/include/mpii_fortlogical.h */
#include "mpii_fortlogical.h"


/* MPIR_F_MPI_BOTTOM is the address of the Fortran MPI_BOTTOM value */
extern FORT_DLL_SPEC int MPIR_F_NeedInit;
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
extern FORT_DLL_SPEC MPI_Fint *MPI_F_ERRCODES_IGNORE;
extern FORT_DLL_SPEC void *MPI_F_ARGV_NULL;
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
extern void mpirinitf_(void);

/*
 * These are hooks for Fortran characters.
 * MPID_FCHAR_T is the type of a Fortran character argument
 * MPID_FCHAR_LARG is the "other" argument that some Fortran compilers use
 * MPID_FCHAR_STR gives the pointer to the characters
 */
#ifdef MPID_CHARACTERS_ARE_CRAYPVP
typedef <whatever > MPID_FCHAR_T;
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

/* The F90 attr copy/delete function prototype and calling convention */
typedef void (FORT_CALL F90_CopyFunction) (MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
                                           MPI_Aint *, MPI_Fint *, MPI_Fint *);
typedef void (FORT_CALL F90_DeleteFunction) (MPI_Fint *, MPI_Fint *, MPI_Aint *, MPI_Aint *,
                                             MPI_Fint *);

void MPII_Keyval_set_f90_proxy(int keyval);

#endif /* MPI_FORTIMPL_H_INCLUDED */
