/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#if !defined(MPL_TRMEM_H_INCLUDED)
#define MPL_TRMEM_H_INCLUDED

#if defined MPL_NEEDS_STRDUP_DECL && !defined strdup
extern char *strdup(const char *);
#endif /* MPL_NEEDS_STRDUP_DECL */

#if defined(MPL_USE_MEMORY_TRACING)
#define MPL_strdup(a)    MPL_trstrdup(a,__LINE__,__FILE__)
#elif defined(MPL_HAVE_STRDUP)
#define MPL_strdup strdup
#else
char *MPL_strdup(const char *str);
#endif /* defined(MPL_USE_MEMORY_TRACING) || defined(MPL_HAVE_STRDUP) */

#ifdef MPL_USE_MEMORY_TRACING
/*M
  MPL_malloc - Allocate memory

  Synopsis:
.vb
  void *MPL_malloc( size_t len )
.ve

  Input Parameter:
. len - Length of memory to allocate in bytes

  Return Value:
  Pointer to allocated memory, or null if memory could not be allocated.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPL_malloc(n) malloc(n)
.ve
  However, it can also be defined as
.vb
  #define MPL_malloc(n) MPL_trmalloc(n,__LINE__,__FILE__)
.ve
  where 'MPL_trmalloc' is a tracing version of 'malloc' that is included with
  MPICH.

  Module:
  Utility
  M*/
#define MPL_malloc(a)    MPL_trmalloc((a),__LINE__,__FILE__)

/*M
  MPL_calloc - Allocate memory that is initialized to zero.

  Synopsis:
.vb
    void *MPL_calloc( size_t nelm, size_t elsize )
.ve

  Input Parameters:
+ nelm - Number of elements to allocate
- elsize - Size of each element.

  Notes:
  Like 'MPL_malloc' and 'MPL_free', this will often be implemented as a
  macro but may use 'MPL_trcalloc' to provide a tracing version.

  Module:
  Utility
  M*/
#define MPL_calloc(a,b)    MPL_trcalloc((a),(b),__LINE__,__FILE__)

/*M
  MPL_free - Free memory

  Synopsis:
.vb
   void MPL_free( void *ptr )
.ve

  Input Parameter:
. ptr - Pointer to memory to be freed.  This memory must have been allocated
  with 'MPL_malloc'.

  Notes:
  This routine will often be implemented as the simple macro
.vb
  #define MPL_free(n) free(n)
.ve
  However, it can also be defined as
.vb
  #define MPL_free(n) MPL_trfree(n,__LINE__,__FILE__)
.ve
  where 'MPL_trfree' is a tracing version of 'free' that is included with
  MPICH.

  Module:
  Utility
  M*/
#define MPL_free(a)      MPL_trfree(a,__LINE__,__FILE__)

#define MPL_realloc(a,b)    MPL_trrealloc((a),(b),__LINE__,__FILE__)

#else /* MPL_USE_MEMORY_TRACING */
/* No memory tracing; just use native functions */
#define MPL_malloc(a)    malloc((size_t)(a))
#define MPL_calloc(a,b)  calloc((size_t)(a),(size_t)(b))
#define MPL_free(a)      free((void *)(a))
#define MPL_realloc(a,b)  realloc((void *)(a),(size_t)(b))

#endif /* MPL_USE_MEMORY_TRACING */


/* --------------------------------------------------------------------
 * MPL memory alignment union
 * The MPL_mem_alignment_t union is used to help internal structures or buffers
 * follow the alignment rules for all predefined datatypes. */

#if 0 /* sample usage : wrap up sample_t structure. */
typedef union {
    sample_t var;
    MPL_mem_alignment_t alignment;
} aligned_sample_t;
#endif

/* Union including all C types that possibly require the largest alignment bytes.
 * Note that we do not need other Fortran/CXX predefined types because all of them
 * are internally translated to appropriate C types. */
typedef union {
    /* Integer types.
     * We only list signed types here, because an unsigned type always require
     * the same alignment as its signed version. Fix me if this theory is wrong.*/
    long long_align;
#ifdef HAVE_LONG_LONG
    long long ll_align;
#endif
#ifdef HAVE_INT32_T
    int32_t int32_t_align;
#endif
#ifdef HAVE_INT64_T
    int64_t int64_t_align;
#endif

    /* Logical type */
#ifdef HAVE__BOOL
    _Bool bool_align;
#endif

    /* Floating-point types */
    double double_align;
#ifdef HAVE_LONG_DOUBLE
    long double ld_align;
#endif

    /* Complex types */
#ifdef HAVE_DOUBLE__COMPLEX
    double _Complex d_complex_align;
#endif
#ifdef HAVE_LONG_DOUBLE__COMPLEX
    long double _Complex ld_complex_align;
#endif
    /* MPICH handles Fortran/CXX complex types as structure (see src/include/oputil.h).
     * Because some platform may have special alignment rule for structures,
     * we include them as well. */
    struct {
        double re;
        double im;
    } mpl_d_complex_align;
#ifdef HAVE_LONG_DOUBLE
    struct {
        long double re;
        long double im;
    } mpl_ld_complex_align;
#endif
} MPL_mem_alignment_t;

/* END of MPL memory alignment union
 * -------------------------------------------------------------------- */

/* FIXME: Consider an option of specifying __attribute__((malloc)) for
   gcc - this lets gcc-style compilers know that the returned pointer
   does not alias any pointer prior to the call.
 */
void MPL_trinit(int, int);
void *MPL_trmalloc(size_t, int, const char[]);
void MPL_trfree(const void *, int, const char[]);
int MPL_trvalid(const char[]);
int MPL_trvalid2(const char[],int,const char[]);
void *MPL_trcalloc(size_t, size_t, int, const char[]);
void *MPL_trrealloc(void *, size_t, int, const char[]);
void *MPL_trstrdup(const char *, int, const char[]);

/* Make sure that FILE is defined */
#include <stdio.h>
void MPL_trdump(FILE *, int);

#endif /* !defined(MPL_TRMEM_H_INCLUDED) */
