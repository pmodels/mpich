/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *
 *  (C) 2009 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef OPUTIL_H_INCLUDED
#define OPUTIL_H_INCLUDED

/* The MPI Standard (MPI-2.1, sec 5.9.2) defines which predfined reduction
   operators are valid by groups of types:
     C integer
     Fortran integer
     Floating point
     Logical
     Complex
     Byte

   We define an "x-macro" for each type group.  Immediately prior to
   instantiating any of these macros you should define a valid
   MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_).  The primary use for this
   is to expand a given group's list into a sequence of case statements.  The
   macro MPIR_OP_TYPE_REDUCE_CASE is available as a convenience to generate a
   case block that performs a reduction with the given operator.  */

#if 0 /* sample usage: */
#undef MPIR_OP_TYPE_MACRO
#define MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_REDUCE_CASE(mpi_type_,c_type_,MPIR_MAX)
/* or */
#define MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_) case (mpi_type_):

MPIR_OP_TYPE_GROUP(C_INTEGER)
MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER)
#undef MPIR_OP_TYPE_MACRO
#endif


/* op_macro_ is a 2-arg macro or function that preforms the reduction
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
        for ( i=0; i<len; i++ )                               \
            a[i] = op_macro_(a[i],b[i]);                      \
        break;                                                \
    }

/* helps enforce consistent naming */
#define MPIR_OP_TYPE_GROUP(group) MPIR_OP_TYPE_GROUP_##group

/* -------------------------------------------------------------------- */
/* These macros are used to disable non-existent types.  They evaluate to
   nothing if the particular feature test is false, otherwise they evaluate to
   the standard macro to be expanded like any other type. */

/* first define all wrapper macros as empty for possibly non-existent types */
#define MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX8(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_LONG_LONG(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_LONG_DOUBLE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INTEGER1_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INTEGER2_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INTEGER4_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INTEGER8_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INTEGER16_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_REAL4_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_REAL8_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_REAL16_CTYPE(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_CXX(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_CXX_COMPLEX(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_CXX_LONG_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INT8_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INT16_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INT32_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_INT64_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_UINT8_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_UINT16_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_UINT32_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_UINT64_T(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_C_BOOL(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_C_FLOAT_COMPLEX(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_C_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_)
#define MPIR_OP_TYPE_MACRO_HAVE_C_LONG_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_)

/* then redefine them to be valid based on other preprocessor definitions */
#if defined(HAVE_FORTRAN_BINDING)
#  undef MPIR_OP_TYPE_MACRO_HAVE_FORTRAN
#  undef MPIR_OP_TYPE_MACRO_HAVE_COMPLEX8
#  undef MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16
#  define MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
/* These two shouldn't really be gated on HAVE_FORTRAN_BINDING alone.  There
   should instead be an individual test like HAVE_LONG_DOUBLE, etc. */
#  define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX8(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#  define MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

#if defined(HAVE_LONG_LONG_INT)
#  undef MPIR_OP_TYPE_MACRO_HAVE_LONG_LONG
#  define MPIR_OP_TYPE_MACRO_HAVE_LONG_LONG(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

#if defined(HAVE_LONG_DOUBLE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_LONG_DOUBLE
#  define MPIR_OP_TYPE_MACRO_HAVE_LONG_DOUBLE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* Fortran fixed width integer type support */
#if defined(MPIR_INTEGER1_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INTEGER1_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_INTEGER1_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_INTEGER2_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INTEGER2_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_INTEGER2_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_INTEGER4_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INTEGER4_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_INTEGER4_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_INTEGER8_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INTEGER8_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_INTEGER8_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_INTEGER16_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INTEGER16_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_INTEGER16_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* Fortran fixed width floating point type support */
#if defined(MPIR_REAL4_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_REAL4_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_REAL4_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_REAL8_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_REAL8_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_REAL8_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(MPIR_REAL16_CTYPE)
#  undef MPIR_OP_TYPE_MACRO_HAVE_REAL16_CTYPE
#  define MPIR_OP_TYPE_MACRO_HAVE_REAL16_CTYPE(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* general C++ types */
#if defined(HAVE_CXX_BINDING)
#  undef MPIR_OP_TYPE_MACRO_HAVE_CXX
#  define MPIR_OP_TYPE_MACRO_HAVE_CXX(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* C++ complex types */
#if defined(HAVE_CXX_COMPLEX)
#  undef MPIR_OP_TYPE_MACRO_HAVE_CXX_COMPLEX
#  define MPIR_OP_TYPE_MACRO_HAVE_CXX_COMPLEX(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
/* also test against MPI_DATATYPE_NULL for extra safety, 0x0c000000 is the uncasted value. */
#if defined(HAVE_CXX_COMPLEX) && (MPIR_CXX_LONG_DOUBLE_COMPLEX_VALUE != 0x0c000000)
#  undef MPIR_OP_TYPE_MACRO_HAVE_CXX_LONG_DOUBLE_COMPLEX
#  define MPIR_OP_TYPE_MACRO_HAVE_CXX_LONG_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* C99 fixed-width types */
#if defined(HAVE_INT8_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INT8_T
#  define MPIR_OP_TYPE_MACRO_HAVE_INT8_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_INT16_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INT16_T
#  define MPIR_OP_TYPE_MACRO_HAVE_INT16_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_INT32_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INT32_T
#  define MPIR_OP_TYPE_MACRO_HAVE_INT32_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_INT64_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_INT64_T
#  define MPIR_OP_TYPE_MACRO_HAVE_INT64_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_UINT8_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_UINT8_T
#  define MPIR_OP_TYPE_MACRO_HAVE_UINT8_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_UINT16_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_UINT16_T
#  define MPIR_OP_TYPE_MACRO_HAVE_UINT16_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_UINT32_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_UINT32_T
#  define MPIR_OP_TYPE_MACRO_HAVE_UINT32_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_UINT64_T)
#  undef MPIR_OP_TYPE_MACRO_HAVE_UINT64_T
#  define MPIR_OP_TYPE_MACRO_HAVE_UINT64_T(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* C boolean */
#if defined(HAVE__BOOL)
#undef MPIR_OP_TYPE_MACRO_HAVE_C_BOOL
#define MPIR_OP_TYPE_MACRO_HAVE_C_BOOL(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* C complex types */
/* Add another layer of indirection and make all of these macros evaluate to a
   common MPIR_OP_C_COMPLEX_TYPE_MACRO macro which in turn evaluates to the
   standard MPIR_OP_TYPE_MACRO.  This lets us override behavior for these
   natively handled types with a single macro redefinition instead of 3. */
#undef MPIR_OP_C_COMPLEX_TYPE_MACRO
#define MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_) MPIR_OP_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#if defined(HAVE_FLOAT__COMPLEX)
#  undef MPIR_OP_TYPE_MACRO_HAVE_C_FLOAT_COMPLEX
#  define MPIR_OP_TYPE_MACRO_HAVE_C_FLOAT_COMPLEX(mpi_type_,c_type_,type_name_) MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_DOUBLE__COMPLEX)
#  undef MPIR_OP_TYPE_MACRO_HAVE_C_DOUBLE_COMPLEX
#  define MPIR_OP_TYPE_MACRO_HAVE_C_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_) MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif
#if defined(HAVE_LONG_DOUBLE__COMPLEX)
#  undef MPIR_OP_TYPE_MACRO_HAVE_C_LONG_DOUBLE_COMPLEX
#  define MPIR_OP_TYPE_MACRO_HAVE_C_LONG_DOUBLE_COMPLEX(mpi_type_,c_type_,type_name_) MPIR_OP_C_COMPLEX_TYPE_MACRO(mpi_type_,c_type_,type_name_)
#endif

/* C types needed to support some of the complex types.

   FIXME These are a hack in most cases, but they seem to work in practice
   and it's what we were doing prior to the oputil.h refactoring. */
typedef struct { 
    float re;
    float im;
} s_complex;

#if defined(HAVE_FORTRAN_BINDING)
typedef struct {
    MPIR_FC_REAL_CTYPE re;
    MPIR_FC_REAL_CTYPE im;
} s_fc_complex;

typedef struct {
    MPIR_FC_DOUBLE_CTYPE re;
    MPIR_FC_DOUBLE_CTYPE im;
} d_fc_complex;
#endif

typedef struct {
    double re;
    double im;
} d_complex;

#if defined(HAVE_LONG_DOUBLE)
typedef struct {
    long double re;
    long double im;
} ld_complex;
#endif

/* -------------------------------------------------------------------- */
/* type group macros

   Implementation note: it is important that no MPI type show up more than once
   among all the lists.  Otherwise it will be easy to end up with two case
   statements with the same value, which is erroneous in C.  Duplicate C types
   in this list are not a problem. */

/* c integer group */
#define MPIR_OP_TYPE_GROUP_C_INTEGER                                                                                  \
    MPIR_OP_TYPE_MACRO(MPI_INT, int, mpir_typename_int)                                                               \
    MPIR_OP_TYPE_MACRO(MPI_LONG, long, mpir_typename_long)                                                            \
    MPIR_OP_TYPE_MACRO(MPI_SHORT, short, mpir_typename_short)                                                         \
    MPIR_OP_TYPE_MACRO(MPI_UNSIGNED_SHORT, unsigned short, mpir_typename_unsigned_short)                              \
    MPIR_OP_TYPE_MACRO(MPI_UNSIGNED, unsigned, mpir_typename_unsigned)                                                \
    MPIR_OP_TYPE_MACRO(MPI_UNSIGNED_LONG, unsigned long, mpir_typename_unsigned_long)                                 \
    MPIR_OP_TYPE_MACRO_HAVE_LONG_LONG(MPI_LONG_LONG, long long, mpir_typename_long_long)                              \
    MPIR_OP_TYPE_MACRO_HAVE_LONG_LONG(MPI_UNSIGNED_LONG_LONG, unsigned long long, mpir_typename_unsigned_long_long)   \
    MPIR_OP_TYPE_MACRO(MPI_SIGNED_CHAR, signed char, mpir_typename_signed_char)                                       \
    MPIR_OP_TYPE_MACRO(MPI_UNSIGNED_CHAR, unsigned char, mpir_typename_unsigned_char)                                 \
    MPIR_OP_TYPE_MACRO_HAVE_INT8_T(MPI_INT8_T, int8_t, mpir_typename_int8_t)                                          \
    MPIR_OP_TYPE_MACRO_HAVE_INT16_T(MPI_INT16_T, int16_t, mpir_typename_int16_t)                                      \
    MPIR_OP_TYPE_MACRO_HAVE_INT32_T(MPI_INT32_T, int32_t, mpir_typename_int32_t)                                      \
    MPIR_OP_TYPE_MACRO_HAVE_INT64_T(MPI_INT64_T, int64_t, mpir_typename_int64_t)                                      \
    MPIR_OP_TYPE_MACRO_HAVE_UINT8_T(MPI_UINT8_T, uint8_t, mpir_typename_uint8_t)                                      \
    MPIR_OP_TYPE_MACRO_HAVE_UINT16_T(MPI_UINT16_T, uint16_t, mpir_typename_uint16_t)                                  \
    MPIR_OP_TYPE_MACRO_HAVE_UINT32_T(MPI_UINT32_T, uint32_t, mpir_typename_uint32_t)                                  \
    MPIR_OP_TYPE_MACRO_HAVE_UINT64_T(MPI_UINT64_T, uint64_t, mpir_typename_uint64_t)                                  \
/* The MPI Standard doesn't include these types in the C integer group for
   predefined operations but MPICH supports them when possible. */
#define MPIR_OP_TYPE_GROUP_C_INTEGER_EXTRA      \
    MPIR_OP_TYPE_MACRO(MPI_CHAR, char, mpir_typename_char)

/* fortran integer group */
#define MPIR_OP_TYPE_GROUP_FORTRAN_INTEGER                                            \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_INTEGER, MPI_Fint, mpir_typename_integer)     \
    MPIR_OP_TYPE_MACRO(MPI_AINT, MPI_Aint, mpir_typename_aint)                        \
    MPIR_OP_TYPE_MACRO(MPI_OFFSET, MPI_Offset, mpir_typename_offset)                  \
    MPIR_OP_TYPE_MACRO(MPI_COUNT, MPI_Count, mpir_typename_count)
/* The MPI Standard doesn't include these types in the Fortran integer group for
   predefined operations but MPICH supports them when possible. */
#define MPIR_OP_TYPE_GROUP_FORTRAN_INTEGER_EXTRA                                                      \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_CHARACTER, char, mpir_typename_character)                     \
    MPIR_OP_TYPE_MACRO_HAVE_INTEGER1_CTYPE(MPI_INTEGER1, MPIR_INTEGER1_CTYPE, mpir_typename_integer1) \
    MPIR_OP_TYPE_MACRO_HAVE_INTEGER2_CTYPE(MPI_INTEGER2, MPIR_INTEGER2_CTYPE, mpir_typename_integer2) \
    MPIR_OP_TYPE_MACRO_HAVE_INTEGER4_CTYPE(MPI_INTEGER4, MPIR_INTEGER4_CTYPE, mpir_typename_integer4) \
    MPIR_OP_TYPE_MACRO_HAVE_INTEGER8_CTYPE(MPI_INTEGER8, MPIR_INTEGER8_CTYPE, mpir_typename_integer8) \
    MPIR_OP_TYPE_MACRO_HAVE_INTEGER16_CTYPE(MPI_INTEGER16, MPIR_INTEGER16_CTYPE, mpir_typename_integer16)

/* floating point group */
/* FIXME: REAL need not be float, nor DOUBLE_PRECISION be double.
   Fortran types are not synonyms for the C types */
#define MPIR_OP_TYPE_GROUP_FLOATING_POINT                                                                             \
    MPIR_OP_TYPE_MACRO(MPI_FLOAT, float, mpir_typename_float)                                                         \
    MPIR_OP_TYPE_MACRO(MPI_DOUBLE, double, mpir_typename_double)                                                      \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_REAL, MPIR_FC_REAL_CTYPE, mpir_typename_real)                                 \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_DOUBLE_PRECISION, MPIR_FC_DOUBLE_CTYPE, mpir_typename_double_precision)       \
    MPIR_OP_TYPE_MACRO_HAVE_LONG_DOUBLE(MPI_LONG_DOUBLE, long double, mpir_typename_long_double)                      \
/* The MPI Standard doesn't include these types in the floating point group for
   predefined operations but MPICH supports them when possible. */
#define MPIR_OP_TYPE_GROUP_FLOATING_POINT_EXTRA                                               \
    MPIR_OP_TYPE_MACRO_HAVE_REAL4_CTYPE(MPI_REAL4, MPIR_REAL4_CTYPE, mpir_typename_real4)     \
    MPIR_OP_TYPE_MACRO_HAVE_REAL8_CTYPE(MPI_REAL8, MPIR_REAL8_CTYPE, mpir_typename_real8)     \
    MPIR_OP_TYPE_MACRO_HAVE_REAL16_CTYPE(MPI_REAL16, MPIR_REAL16_CTYPE, mpir_typename_real16)

/* logical group */
/* FIXME Is MPI_Fint really OK here? */
#define MPIR_OP_TYPE_GROUP_LOGICAL                                                    \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_LOGICAL, MPI_Fint, mpir_typename_logical)     \
    MPIR_OP_TYPE_MACRO_HAVE_C_BOOL(MPI_C_BOOL, _Bool, mpir_typename_c_bool)           \
    MPIR_OP_TYPE_MACRO_HAVE_CXX(MPIR_CXX_BOOL_VALUE, MPIR_CXX_BOOL_CTYPE, mpir_typename_cxx_bool_value)
#define MPIR_OP_TYPE_GROUP_LOGICAL_EXTRA /* empty, provided for consistency */

/* complex group */
#define MPIR_OP_TYPE_GROUP_COMPLEX                                                                                    \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_COMPLEX, s_fc_complex, mpir_typename_complex)                                 \
    MPIR_OP_TYPE_MACRO_HAVE_C_FLOAT_COMPLEX(MPI_C_FLOAT_COMPLEX, float _Complex, mpir_typename_c_float_complex)       \
    MPIR_OP_TYPE_MACRO_HAVE_C_DOUBLE_COMPLEX(MPI_C_DOUBLE_COMPLEX, double _Complex, mpir_typename_c_double_complex)   \
    MPIR_OP_TYPE_MACRO_HAVE_C_LONG_DOUBLE_COMPLEX(MPI_C_LONG_DOUBLE_COMPLEX, long double _Complex, mpir_typename_c_long_double_complex)
#define MPIR_OP_TYPE_GROUP_COMPLEX_EXTRA                                                                                      \
    MPIR_OP_TYPE_MACRO_HAVE_FORTRAN(MPI_DOUBLE_COMPLEX, d_fc_complex, mpir_typename_double_complex)                           \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX8(MPI_COMPLEX8, s_complex, mpir_typename_complex8)                                         \
    MPIR_OP_TYPE_MACRO_HAVE_COMPLEX16(MPI_COMPLEX16, d_complex, mpir_typename_complex16)                                      \
    MPIR_OP_TYPE_MACRO_HAVE_CXX_COMPLEX(MPIR_CXX_COMPLEX_VALUE, s_complex, mpir_typename_cxx_complex_value)                   \
    MPIR_OP_TYPE_MACRO_HAVE_CXX_COMPLEX(MPIR_CXX_DOUBLE_COMPLEX_VALUE, d_complex, mpir_typename_cxx_double_complex_value)     \
    MPIR_OP_TYPE_MACRO_HAVE_CXX_LONG_DOUBLE_COMPLEX(MPIR_CXX_LONG_DOUBLE_COMPLEX_VALUE, ld_complex, mpir_typename_cxx_long_double_complex_value)

/* byte group */
#define MPIR_OP_TYPE_GROUP_BYTE         \
    MPIR_OP_TYPE_MACRO(MPI_BYTE, unsigned char, mpir_typename_byte)
#define MPIR_OP_TYPE_GROUP_BYTE_EXTRA /* empty, provided for consistency */

/* convenience macro that just is all non-extra groups concatenated */
#define MPIR_OP_TYPE_GROUP_ALL_BASIC    \
    MPIR_OP_TYPE_GROUP(C_INTEGER)       \
    MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER) \
    MPIR_OP_TYPE_GROUP(FLOATING_POINT)  \
    MPIR_OP_TYPE_GROUP(LOGICAL)         \
    MPIR_OP_TYPE_GROUP(COMPLEX)         \
    MPIR_OP_TYPE_GROUP(BYTE)

/* this macro includes just the extra type groups */
#define MPIR_OP_TYPE_GROUP_ALL_EXTRA          \
    MPIR_OP_TYPE_GROUP(C_INTEGER_EXTRA)       \
    MPIR_OP_TYPE_GROUP(FORTRAN_INTEGER_EXTRA) \
    MPIR_OP_TYPE_GROUP(FLOATING_POINT_EXTRA)  \
    MPIR_OP_TYPE_GROUP(LOGICAL_EXTRA)         \
    MPIR_OP_TYPE_GROUP(COMPLEX_EXTRA)         \
    MPIR_OP_TYPE_GROUP(BYTE_EXTRA)

#endif /* OPUTIL_H_INCLUDED */
