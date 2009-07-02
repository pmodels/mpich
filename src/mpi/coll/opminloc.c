/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

/* MINLOC and MAXLOC structures */
typedef struct MPIR_2int_loctype {
  int  value;
  int  loc;
} MPIR_2int_loctype;

typedef struct MPIR_floatint_loctype {
  float  value;
  int    loc;
} MPIR_floatint_loctype;

typedef struct MPIR_longint_loctype {
  long  value;
  int    loc;
} MPIR_longint_loctype;

typedef struct MPIR_shortint_loctype {
  short  value;
  int    loc;
} MPIR_shortint_loctype;

typedef struct MPIR_doubleint_loctype {
  double  value;
  int     loc;
} MPIR_doubleint_loctype;

#if defined(HAVE_LONG_DOUBLE)
typedef struct MPIR_longdoubleint_loctype {
  long double   value;
  int           loc;
} MPIR_longdoubleint_loctype;
#endif

/* Note a subtlety in these two macros which avoids compiler warnings.
   The compiler complains about using == on floats, but the standard
   requires that we set loc to min of the locs if the two values are
   equal.  So we do "if a>b {} else if a>=b Y" which is the same as
   "if a>b X else if a==b Y" but avoids the warning. */
#define MPIR_MINLOC_C_CASE(c_type_) {                   \
        c_type_ *a = (c_type_ *)inoutvec;               \
        c_type_ *b = (c_type_ *)invec;                  \
        for (i=0; i<len; i++) {                         \
            if (a[i].value > b[i].value) {              \
                a[i].value = b[i].value;                \
                a[i].loc   = b[i].loc;                  \
            } else if (a[i].value >= b[i].value)        \
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc); \
        }                                               \
    }                                                   \
    break

#define MPIR_MINLOC_F_CASE(f_type_) {                   \
        f_type_ *a = (f_type_ *)inoutvec;               \
        f_type_ *b = (f_type_ *)invec;                  \
        for ( i=0; i<flen; i+=2 ) {                     \
            if (a[i] > b[i]) {                          \
                a[i]   = b[i];                          \
                a[i+1] = b[i+1];                        \
            } else if (a[i] >= b[i])                    \
                a[i+1] = MPIR_MIN(a[i+1],b[i+1]);       \
        }                                               \
    }                                                   \
    break

#undef FUNCNAME
#define FUNCNAME MPIR_MINLOC
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
void MPIR_MINLOC( 
	void *invec, 
	void *inoutvec, 
	int *Len, 
	MPI_Datatype *type )
{
    int mpi_errno = MPI_SUCCESS;
    int i, len = *Len, flen;
    
    flen = len * 2; /* used for Fortran types */

    switch (*type) {
    /* first the C types */
    case MPI_2INT:       MPIR_MINLOC_C_CASE(MPIR_2int_loctype);        
    case MPI_FLOAT_INT:  MPIR_MINLOC_C_CASE(MPIR_floatint_loctype);
    case MPI_LONG_INT:   MPIR_MINLOC_C_CASE(MPIR_longint_loctype);
    case MPI_SHORT_INT:  MPIR_MINLOC_C_CASE(MPIR_shortint_loctype);
    case MPI_DOUBLE_INT: MPIR_MINLOC_C_CASE(MPIR_doubleint_loctype);
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE_INT: MPIR_MINLOC_C_CASE(MPIR_longdoubleint_loctype);
#endif

    /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
    case MPI_2INTEGER:          MPIR_MINLOC_F_CASE(int);
    case MPI_2REAL:             MPIR_MINLOC_F_CASE(float);
    case MPI_2DOUBLE_PRECISION: MPIR_MINLOC_F_CASE(double);
#endif
#endif
	/* --BEGIN ERROR HANDLING-- */
    default: {
	MPIU_THREADPRIV_DECL;
	MPIU_THREADPRIV_GET;
        MPIU_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MINLOC" );
        MPIU_THREADPRIV_FIELD(op_errno) = mpi_errno;
        break;
    }
	/* --END ERROR HANDLING-- */
    }

}




#undef FUNCNAME
#define FUNCNAME MPIR_MINLOC_check_dtype
#undef FCNAME
#define FCNAME MPIU_QUOTE(FUNCNAME)
int MPIR_MINLOC_check_dtype( MPI_Datatype type )
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

    default: MPIU_ERR_SET1(mpi_errno, MPI_ERR_OP, "**opundefined", "**opundefined %s", "MPI_MINLOC");
    }
    
    return mpi_errno;
}
