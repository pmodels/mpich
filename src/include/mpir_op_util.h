/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIR_OP_UTIL_H_INCLUDED
#define MPIR_OP_UTIL_H_INCLUDED

/* The MPI Standard (MPI-2.1, sec 5.9.2) defines which predefined reduction
   operators are valid by groups of types:
     C integer
     Fortran integer
     Floating point
     Logical
     Complex
     Byte

   We define an "x-macro" for each type group.  Immediately prior to
   instantiating any of these macros you should define a valid
   MPIR_OP_TYPE_MACRO(mpi_type_,c_type_).  The primary use for this
   is to expand a given group's list into a sequence of case statements.  The
   macro MPIR_OP_TYPE_REDUCE_CASE is available as a convenience to generate a
   case block that performs a reduction with the given operator.  */

#if 0   /* sample usage: */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_,c_type_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_,c_type_,MPIR_MAX)
/* or */
#define MPIR_OP_TYPE_MACRO(mpi_type_,c_type_) case (mpi_type_):

MPIR_OP_TYPE_GROUP(INTEGER)
    MPIR_OP_TYPE_GROUP(FLOATING_POINT)
#undef MPIR_OP_TYPE_MACRO
#endif
/* op_macro_ is a 2-arg macro or function that performs the reduction
   operation on a single element */
/* FIXME: This code may penalize the performance on many platforms as
   a work-around for a compiler bug in some versions of xlc.  It
   would be far better to either test for that bug or to confirm that
   the work-around is really as benign as claimed. */
/* Ideally "b" would be const, but xlc on POWER7 can't currently handle
 * "const long double _Complex * restrict" as a valid pointer type.  It just
 * emits a warning and generates invalid arithmetic code.  We could drop the
 * restrict instead, but we are more likely to get an optimization from it than
 * const.  [goodell@ 2010-12-15] */
#define MPIR_OP_TYPE_REDUCE_CASE(mpi_type_,c_type_,op_macro_) \
    case (mpi_type_): {                                       \
        c_type_ * restrict a = (c_type_ *)inoutvec;           \
        /*const*/ c_type_ * restrict b = (c_type_ *)invec;    \
        for (i=0; i<len; i++)                               \
            a[i] = (c_type_) op_macro_(a[i],b[i]);            \
        break;                                                \
    }
/* helps enforce consistent naming */
#define MPIR_OP_TYPE_GROUP(group) MPIR_OP_TYPE_GROUP_##group
/* -------------------------------------------------------------------- */
/* These macros are used to disable non-existent types.  They evaluate to
   nothing if the particular feature test is false, otherwise they evaluate to
   the standard macro to be expanded like any other type. */
#if defined(MPIR_INT128_CTYPE)
#undef MPIR_OP_TYPE_MACRO_HAVE_INT128
#define MPIR_OP_TYPE_MACRO_HAVE_INT128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_INT128(mpi_type_,c_type_)
#endif
#if defined(MPIR_FLOAT16_CTYPE)
#undef MPIR_OP_TYPE_MACRO_HAVE_FLOAT16
#define MPIR_OP_TYPE_MACRO_HAVE_FLOAT16(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_FLOAT16(mpi_type_,c_type_)
#endif
#if defined(MPIR_FLOAT128_CTYPE)
#undef MPIR_OP_TYPE_MACRO_HAVE_FLOAT128
#define MPIR_OP_TYPE_MACRO_HAVE_FLOAT128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_FLOAT128(mpi_type_,c_type_)
#endif
/* non IEEE 784 alterante float (e.g. long double) */
#if defined(MPIR_ALT_FLOAT96_CTYPE)
#undef MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT96
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT96(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT96(mpi_type_,c_type_)
#endif
#if defined(MPIR_ALT_FLOAT128_CTYPE)
#undef MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT128
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT128(mpi_type_,c_type_)
#endif
/* complex types */
#if defined(MPIR_COMPLEX16_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX16(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX16(mpi_type_,c_type_)
#if defined(MPIR_FLOAT16_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(mpi_type_,c_type_)
#endif
#endif
#if defined(MPIR_COMPLEX32_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX32(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX32(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX32(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX32(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#endif
#if defined(MPIR_COMPLEX64_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX64(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX64(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX64(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX64(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#endif
#if defined(MPIR_COMPLEX128_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX128(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX128(mpi_type_,c_type_)
#if defined(MPIR_FLOAT128_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX128(mpi_type_,c_type_)
#endif
#endif
/* alternative complex types */
#if defined(MPIR_ALT_COMPLEX96_CTYPE)   /* long double complex on i386 */
#define MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX96(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX96(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX96(mpi_type_,c_type_)
#if defined(MPIR_ALT_FLOAT128_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX96(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX96(mpi_type_,c_type_)
#endif
#endif
#if defined(MPIR_ALT_COMPLEX128_CTYPE)  /* long double complex on x86-64 */
#define MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX128(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX128(mpi_type_,c_type_)
#if defined(MPIR_ALT_FLOAT128_CTYPE)
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX128(mpi_type_,c_type_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_)
#else
#define MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX128(mpi_type_,c_type_)
#endif
#endif
/* C types needed to support some of the complex types.

   FIXME These are a hack in most cases, but they seem to work in practice
   and it's what we were doing prior to the mpir_op_util.h refactoring. */
#if defined(MPIR_FLOAT16_CTYPE)
struct my_complex4 {
    MPIR_FLOAT16_CTYPE re;
    MPIR_FLOAT16_CTYPE im;
};
#endif

struct my_complex8 {
    float re;
    float im;
};

struct my_complex16 {
    double re;
    double im;
};

#if defined(MPIR_FLOAT128_CTYPE)
struct my_complex32 {
    MPIR_FLOAT128_CTYPE re;
    MPIR_FLOAT128_CTYPE im;
};
#endif

#if defined(MPIR_ALT_FLOAT96_CTYPE) || defined(MPIR_ALT_FLOAT128_CTYPE)
struct my_ld_complex {
    long double re;
    long double im;
};
#endif

/* -------------------------------------------------------------------- */
/* type group macros

   Implementation note: it is important that no MPI type show up more than once
   among all the lists.  Otherwise it will be easy to end up with two case
   statements with the same value, which is erroneous in C.  Duplicate C types
   in this list are not a problem. */

/* c integer group */
#define MPIR_OP_TYPE_GROUP_INTEGER                                                                                  \
    MPIR_OP_TYPE_MACRO(MPIR_INT8, MPIR_INT8_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_INT16, MPIR_INT16_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_INT32, MPIR_INT32_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_INT64, MPIR_INT64_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_INT128(MPIR_INT128, MPIR_INT128_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_UINT8, MPIR_UINT8_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_UINT16, MPIR_UINT16_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_UINT32, MPIR_UINT32_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_UINT64, MPIR_UINT64_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_INT128(MPIR_UINT128, MPIR_UINT128_CTYPE)


/* floating point group */
/* FIXME: REAL need not be float, nor DOUBLE_PRECISION be double.
   Fortran types are not synonyms for the C types */
#define MPIR_OP_TYPE_GROUP_FLOATING_POINT                                                                             \
    MPIR_OP_TYPE_MACRO_HAVE_FLOAT16(MPIR_FLOAT16, MPIR_FLOAT16_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_FLOAT32, MPIR_FLOAT32_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_FLOAT64, MPIR_FLOAT64_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_FLOAT128(MPIR_FLOAT128, MPIR_FLOAT128_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT96(MPIR_ALT_FLOAT96, MPIR_ALT_FLOAT96_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_ALT_FLOAT128(MPIR_ALT_FLOAT128, MPIR_ALT_FLOAT128_CTYPE)


/* complex group */
/* the C_COMPLEX uses C native complex, while the other group does it manually */
#define MPIR_OP_TYPE_GROUP_C_COMPLEX \
    MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX16(MPIR_COMPLEX16, MPIR_COMPLEX16_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX32(MPIR_COMPLEX32, MPIR_COMPLEX32_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX64(MPIR_COMPLEX64, MPIR_COMPLEX64_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_C_COMPLEX128(MPIR_COMPLEX128, MPIR_COMPLEX128_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX96(MPIR_ALT_COMPLEX96, MPIR_ALT_COMPLEX96_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_C_ALT_COMPLEX128(MPIR_ALT_COMPLEX128, MPIR_ALT_COMPLEX128_CTYPE)

#define MPIR_OP_TYPE_GROUP_COMPLEX \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(MPIR_COMPLEX16, struct my_complex4) \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX32(MPIR_COMPLEX32, struct my_complex8) \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX64(MPIR_COMPLEX64, struct my_complex16) \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX128(MPIR_COMPLEX128, struct my_complex32) \
    MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX96(MPIR_ALT_COMPLEX96, struct my_ld_complex) \
    MPIR_OP_TYPE_MACRO_HAVE_ALT_COMPLEX128(MPIR_ALT_COMPLEX128, struct my_ld_complex)

/* fortran logical group */
#define MPIR_OP_TYPE_GROUP_FORTRAN_LOGICAL \
    MPIR_OP_TYPE_MACRO(MPIR_FORTRAN_LOGICAL8, MPIR_INT8_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_FORTRAN_LOGICAL16, MPIR_INT16_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_FORTRAN_LOGICAL32, MPIR_INT32_CTYPE) \
    MPIR_OP_TYPE_MACRO(MPIR_FORTRAN_LOGICAL64, MPIR_INT64_CTYPE) \
    MPIR_OP_TYPE_MACRO_HAVE_INT128(MPIR_FORTRAN_LOGICAL128, MPIR_INT128_CTYPE)

/* convenience macro that just is all non-extra groups concatenated */
#define MPIR_OP_TYPE_GROUP_ALL \
    MPIR_OP_TYPE_GROUP(INTEGER)       \
    MPIR_OP_TYPE_GROUP(FLOATING_POINT)  \
    MPIR_OP_TYPE_GROUP(COMPLEX)         \

#endif /* MPIR_OP_UTIL_H_INCLUDED */
