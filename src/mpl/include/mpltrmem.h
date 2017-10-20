/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPLTRMEM_H_INCLUDED
#define MPLTRMEM_H_INCLUDED

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
void MPL_trinit(int);
void *MPL_trmalloc(size_t, int, const char[]);
void MPL_trfree(void *, int, const char[]);
int MPL_trvalid(const char[]);
int MPL_trvalid2(const char[],int,const char[]);
void MPL_trspace(size_t *, size_t *);
void MPL_trid(int);
void MPL_trlevel(int);
void MPL_trDebugLevel(int);
void *MPL_trcalloc(size_t, size_t, int, const char[]);
void *MPL_trrealloc(void *, size_t, int, const char[]);
void *MPL_trstrdup(const char *, int, const char[]);
void MPL_TrSetMaxMem(size_t);

/* Make sure that FILE is defined */
#include <stdio.h>
void MPL_trdump(FILE *, int);
void MPL_trSummary(FILE *, int);
void MPL_trdumpGrouped(FILE *, int);

#endif /* !defined(MPLTRMEM_H_INCLUDED) */
