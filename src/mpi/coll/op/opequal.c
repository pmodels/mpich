/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/* Equal structures */
/* For reusing code, using int type instead of bool*/
typedef struct MPIR_2int_eqltype {
    int value;
    int isEqual;
} MPIR_2int_eqltype;

typedef struct MPIR_floatint_eqltype {
    float value;
    int isEqual;
} MPIR_floatint_eqltype;

typedef struct MPIR_longint_eqltype {
    long value;
    int isEqual;
} MPIR_longint_eqltype;

typedef struct MPIR_shortint_eqltype {
    short value;
    int isEqual;
} MPIR_shortint_eqltype;

typedef struct MPIR_doubleint_eqltype {
    double value;
    int isEqual;
} MPIR_doubleint_eqltype;

#if defined(HAVE_LONG_DOUBLE)
typedef struct MPIR_longdoubleint_eqltype {
    long double value;
    int isEqual;
} MPIR_longdoubleint_eqltype;
#endif

/* Note a subtlety in these two macros which avoids compiler warnings.
   The compiler complains about using == on floats, but the standard
   requires that we set loc to min of the locs if the two values are
   equal.  So we do "if a<b {} else if a<=b Y" which is the same as
   "if a<b X else if a==b Y" but avoids the warning. */
#define MPIR_EQUAL_C_CASE(c_type_) {                    \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i=0; i<len; i++) {                         \
            if (0 == b[i].isEqual){                     \
                a[i].isEqual = b[i].isEqual;            \
            }else if (a[i].value != b[i].value){        \
                a[i].isEqual = 0;                       \
            }else{                                      \
                a[i].isEqual = 1;                       \
            }                                           \
        }                                               \
    }                                                   \
    break

#define MPIR_EQUAL_F_CASE(f_type_) {                    \
        f_type_ *a = (f_type_ *)inoutvec;               \
        f_type_ *b = (f_type_ *)invec;                  \
        for (i=0; i<flen; i+=2) {                       \
            if(0 == b[i+1]){                            \
                a[i+1] = b[i+1];                        \
            }else if (a[i] != b[i]){                    \
                a[i+1] = 0;                             \
            }else{                                      \
                a[i+1] = 1;                             \
            }                                           \
        }                                               \
    }                                                   \
    break


void MPIR_EQUAL(void *invec, void *inoutvec, int *Len, MPI_Datatype * type)
{
    int i, len = *Len;

#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
    int flen = len * 2;         /* used for Fortran types */
#endif
#endif
    
    switch (*type) {
            /* first the C types */
        case MPI_INT:
            MPIR_EQUAL_C_CASE(MPIR_2int_eqltype);
        case MPI_FLOAT_INT:
            MPIR_EQUAL_C_CASE(MPIR_floatint_eqltype);
        case MPI_LONG_INT:
            MPIR_EQUAL_C_CASE(MPIR_longint_eqltype);
        case MPI_SHORT_INT:
            MPIR_EQUAL_C_CASE(MPIR_shortint_eqltype);
        case MPI_DOUBLE_INT:
            MPIR_EQUAL_C_CASE(MPIR_doubleint_eqltype);
#if defined(HAVE_LONG_DOUBLE)
        case MPI_LONG_DOUBLE_INT:
            MPIR_EQUAL_C_CASE(MPIR_longdoubleint_eqltype);
#endif

            /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
        case MPI_2INTEGER:
            MPIR_EQUAL_F_CASE(MPI_Fint);
        case MPI_2REAL:
            MPIR_EQUAL_F_CASE(MPIR_FC_REAL_CTYPE);
        case MPI_2DOUBLE_PRECISION:
            MPIR_EQUAL_F_CASE(MPIR_FC_DOUBLE_CTYPE);
#endif
#endif
        default:
            MPIR_Assert(0);
            break;
    }

}


int MPIR_EQUAL_check_dtype(MPI_Datatype type)
{
    int mpi_errno = MPI_SUCCESS;

    switch (type) {
            /* first the C types */
        case MPI_2INT:
        case MPI_FLOAT_INT:
        case MPI_LONG_INT:
        case MPI_SHORT_INT:
        case MPI_DOUBLE_INT:
#if defined(HAVE_LONG_DOUBLE)
        case MPI_LONG_DOUBLE_INT:
#endif
            /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
        case MPI_2INTEGER:
        case MPI_2REAL:
        case MPI_2DOUBLE_PRECISION:
#endif
#endif
            break;

        default:
            MPIR_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_EQUAL");
    }

    return mpi_errno;
}
