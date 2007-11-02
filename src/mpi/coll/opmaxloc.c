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


void MPIR_MAXLOC( 
	void *invec, 
	void *inoutvec, 
	int *Len, 
	MPI_Datatype *type )
{
    static const char FCNAME[] = "MPIR_MAXLOC";
    int i, len = *Len, flen;
    
    flen = len * 2; /* used for Fortran types */

    switch (*type) {
    /* first the C types */
    case MPI_2INT: {
        MPIR_2int_loctype *a = (MPIR_2int_loctype *)inoutvec;
        MPIR_2int_loctype *b = (MPIR_2int_loctype *)invec;
        for (i=0; i<len; i++) {
            if (a[i].value == b[i].value)
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc);
            else if (a[i].value < b[i].value) {
                a[i].value = b[i].value;
                a[i].loc   = b[i].loc;
            }
        }
        break;
    }
    case MPI_FLOAT_INT: {
        MPIR_floatint_loctype *a = (MPIR_floatint_loctype *)inoutvec;
        MPIR_floatint_loctype *b = (MPIR_floatint_loctype *)invec;
        for (i=0; i<len; i++) {
            if (a[i].value == b[i].value)
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc);
            else if (a[i].value < b[i].value) {
                a[i].value = b[i].value;
                a[i].loc   = b[i].loc;
            }
        }
        break;
    }
    case MPI_LONG_INT: {
        MPIR_longint_loctype *a = (MPIR_longint_loctype *)inoutvec;
        MPIR_longint_loctype *b = (MPIR_longint_loctype *)invec;
        for (i=0; i<len; i++) {
            if (a[i].value == b[i].value)
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc);
            else if (a[i].value < b[i].value) {
                a[i].value = b[i].value;
                a[i].loc   = b[i].loc;
            }
        }
        break;
    }
    case MPI_SHORT_INT: {
        MPIR_shortint_loctype *as = (MPIR_shortint_loctype *)inoutvec;
        MPIR_shortint_loctype *bs = (MPIR_shortint_loctype *)invec;
        for (i=0; i<len; i++) {
            if (as[i].value == bs[i].value)
                as[i].loc = MPIR_MIN(as[i].loc,bs[i].loc);
            else if (as[i].value < bs[i].value) {
                as[i].value = bs[i].value;
                as[i].loc   = bs[i].loc;
            }
        }
        break;
    }
    case MPI_DOUBLE_INT: {
        MPIR_doubleint_loctype *a = (MPIR_doubleint_loctype *)inoutvec;
        MPIR_doubleint_loctype *b = (MPIR_doubleint_loctype *)invec;
        for (i=0; i<len; i++) {
            if (a[i].value == b[i].value)
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc);
            else if (a[i].value < b[i].value) {
                a[i].value = b[i].value;
                a[i].loc   = b[i].loc;
            }
        }
        break;
    }
    
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE_INT: {
        MPIR_longdoubleint_loctype *a = (MPIR_longdoubleint_loctype *)inoutvec;
        MPIR_longdoubleint_loctype *b = (MPIR_longdoubleint_loctype *)invec;
        for (i=0; i<len; i++) {
            if (a[i].value == b[i].value)
                a[i].loc = MPIR_MIN(a[i].loc,b[i].loc);
            else if (a[i].value < b[i].value) {
                a[i].value = b[i].value;
                a[i].loc   = b[i].loc;
            }
        }
        break;
    }
#endif

    /* now the Fortran types */
#ifdef HAVE_FORTRAN_BINDING
#ifndef HAVE_NO_FORTRAN_MPI_TYPES_IN_C
    case MPI_2INTEGER: {
        int *a = (int *)inoutvec; int *b = (int *)invec;
        for ( i=0; i<flen; i+=2 ) {
            if (a[i] == b[i])
                a[i+1] = MPIR_MIN(a[i+1],b[i+1]);
            else if (a[i] < b[i]) {
                a[i]   = b[i];
                a[i+1] = b[i+1];
            }
        }
        break;
    }
    case MPI_2REAL: {
        float *a = (float *)inoutvec; float *b = (float *)invec;
        for ( i=0; i<flen; i+=2 ) {
            if (a[i] == b[i])
                a[i+1] = MPIR_MIN(a[i+1],b[i+1]);
            else if (a[i] < b[i]) {
                a[i]   = b[i];
                a[i+1] = b[i+1];
            }
        }
        break;
    }
    case MPI_2DOUBLE_PRECISION: {
        double *a = (double *)inoutvec; double *b = (double *)invec;
        for ( i=0; i<flen; i+=2 ) {
            if (a[i] == b[i])
                a[i+1] = MPIR_MIN(a[i+1],b[i+1]);
            else if (a[i] < b[i]) {
                a[i]   = b[i];
                a[i+1] = b[i+1];
            }
        }
        break;
    }
#endif
#endif
	/* --BEGIN ERROR HANDLING-- */
    default: {
        MPICH_PerThread_t *p;
        MPIR_GetPerThread(&p);
        p->op_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MAXLOC" );
        break;
    }
	/* --END ERROR HANDLING-- */
    }

}


int MPIR_MAXLOC_check_dtype( MPI_Datatype type )
{
    static const char FCNAME[] = "MPIR_MAXLOC_check_dtype";
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
        return MPI_SUCCESS;
	/* --BEGIN ERROR HANDLING-- */
    default: 
        return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_MAXLOC" );
	/* --END ERROR HANDLING-- */
    }
}

