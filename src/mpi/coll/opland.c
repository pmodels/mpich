/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#ifdef HAVE_FORTRAN_BINDING
#include "mpi_fortlogical.h"
#endif
/* 
 * In MPI-1, this operation is valid only for  C integer, Fortran logical
 * data items (4.9.2 Predefined reduce operations)
 */
#ifndef MPIR_LLAND
#define MPIR_LLAND(a,b) ((a)&&(b))
#endif
void MPIR_LAND ( 
    void *invec, 
    void *inoutvec, 
    int *Len, 
    MPI_Datatype *type )
{
    static const char FCNAME[] = "MPIR_LAND";
    int i, len = *Len;
    
    switch (*type) {
    case MPI_INT: {
        int * restrict a = (int *)inoutvec; 
        int * restrict b = (int *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED: {
        unsigned * restrict a = (unsigned *)inoutvec; 
        unsigned * restrict b = (unsigned *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_LONG: {
        long * restrict a = (long *)inoutvec; 
        long * restrict b = (long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: {
	/* case MPI_LONG_LONG_INT: defined to be the same as long_long */
        long long * restrict a = (long long *)inoutvec; 
        long long * restrict b = (long long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
    case MPI_UNSIGNED_LONG: {
        unsigned long * restrict a = (unsigned long *)inoutvec; 
        unsigned long * restrict b = (unsigned long *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_SHORT: {
        short * restrict a = (short *)inoutvec; 
        short * restrict b = (short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_SHORT: {
        unsigned short * restrict a = (unsigned short *)inoutvec; 
        unsigned short * restrict b = (unsigned short *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_CHAR: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_CHARACTER: 
#endif
    {
        char * restrict a = (char *)inoutvec; 
        char * restrict b = (char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_SIGNED_CHAR: {
        signed char * restrict a = (signed char *)inoutvec; 
        signed char * restrict b = (signed char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_UNSIGNED_CHAR: {
        unsigned char * restrict a = (unsigned char *)inoutvec; 
        unsigned char * restrict b = (unsigned char *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_FLOAT: 
#ifdef HAVE_FORTRAN_BINDING
	/* FIXME: This assume C float = Fortran real */
    case MPI_REAL: 
#endif
    {
        float * restrict a = (float *)inoutvec; 
        float * restrict b = (float *)invec;
        for ( i=0; i<len; i++ )
            a[i] = (float)MPIR_LLAND(a[i],b[i]);
        break;
    }
    case MPI_DOUBLE: 
#ifdef HAVE_FORTRAN_BINDING
	/* FIXME: This assume C double = Fortran double precision */
    case MPI_DOUBLE_PRECISION: 
#endif
    {
        double * restrict a = (double *)inoutvec; 
        double * restrict b = (double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#ifdef MPIR_INTEGER1_CTYPE
    case MPI_INTEGER1: {
        MPIR_INTEGER1_CTYPE * restrict a = (MPIR_INTEGER1_CTYPE *)inoutvec; 
        MPIR_INTEGER1_CTYPE * restrict b = (MPIR_INTEGER1_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_INTEGER2_CTYPE
    case MPI_INTEGER2: {
        MPIR_INTEGER2_CTYPE * restrict a = (MPIR_INTEGER2_CTYPE *)inoutvec; 
        MPIR_INTEGER2_CTYPE * restrict b = (MPIR_INTEGER2_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_INTEGER4_CTYPE
    case MPI_INTEGER4: {
        MPIR_INTEGER4_CTYPE * restrict a = (MPIR_INTEGER4_CTYPE *)inoutvec; 
        MPIR_INTEGER4_CTYPE * restrict b = (MPIR_INTEGER4_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_INTEGER8_CTYPE
    case MPI_INTEGER8: {
        MPIR_INTEGER8_CTYPE * restrict a = (MPIR_INTEGER8_CTYPE *)inoutvec; 
        MPIR_INTEGER8_CTYPE * restrict b = (MPIR_INTEGER8_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_INTEGER16_CTYPE
    case MPI_INTEGER16: {
        MPIR_INTEGER16_CTYPE * restrict a = (MPIR_INTEGER16_CTYPE *)inoutvec; 
        MPIR_INTEGER16_CTYPE * restrict b = (MPIR_INTEGER16_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_REAL4_CTYPE
    case MPI_REAL4: {
        MPIR_REAL4_CTYPE * restrict a = (MPIR_REAL4_CTYPE *)inoutvec; 
        MPIR_REAL4_CTYPE * restrict b = (MPIR_REAL4_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_REAL8_CTYPE
    case MPI_REAL8: {
        MPIR_REAL8_CTYPE * restrict a = (MPIR_REAL8_CTYPE *)inoutvec; 
        MPIR_REAL8_CTYPE * restrict b = (MPIR_REAL8_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef MPIR_REAL16_CTYPE
    case MPI_REAL16: {
        MPIR_REAL16_CTYPE * restrict a = (MPIR_REAL16_CTYPE *)inoutvec; 
        MPIR_REAL16_CTYPE * restrict b = (MPIR_REAL16_CTYPE *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: {
        long double * restrict a = (long double *)inoutvec; 
        long double * restrict b = (long double *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPI_LOGICAL: {
        MPI_Fint * restrict a = (MPI_Fint *)inoutvec; 
        MPI_Fint * restrict b = (MPI_Fint *)invec;
        for (i=0; i<len; i++) 
            a[i] = MPIR_TO_FLOG(MPIR_LLAND(MPIR_FROM_FLOG(a[i]),
                                           MPIR_FROM_FLOG(b[i])));
        break;
    }
    case MPI_INTEGER: {
        MPI_Fint * restrict a = (MPI_Fint *)inoutvec; 
        MPI_Fint * restrict b = (MPI_Fint *)invec;
        for ( i=0; i<len; i++ )
            a[i] = MPIR_LLAND(a[i],b[i]);
        break;
    }
#endif
	/* --BEGIN ERROR HANDLING-- */
    default: {
        MPICH_PerThread_t *p;
        MPIR_GetPerThread(&p);
        p->op_errno = MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_LAND" );
        break;
    }
	/* --END ERROR HANDLING-- */
    }
}


int MPIR_LAND_check_dtype ( MPI_Datatype type )
{
    static const char FCNAME[] = "MPIR_LAND_check_dtype";
    
    switch (type) {
    case MPI_INT: 
    case MPI_UNSIGNED: 
    case MPI_LONG: 
#if defined(HAVE_LONG_LONG_INT)
    case MPI_LONG_LONG: 
#endif
    case MPI_UNSIGNED_LONG: 
    case MPI_SHORT: 
    case MPI_UNSIGNED_SHORT: 
    case MPI_CHAR: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_CHARACTER: 
#endif
    case MPI_SIGNED_CHAR: 
    case MPI_UNSIGNED_CHAR: 
    case MPI_FLOAT: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_REAL: 
#endif
    case MPI_DOUBLE: 
#ifdef HAVE_FORTRAN_BINDING
    case MPI_DOUBLE_PRECISION: 
#endif
#if defined(HAVE_LONG_DOUBLE)
    case MPI_LONG_DOUBLE: 
#endif
#ifdef HAVE_FORTRAN_BINDING
    case MPI_LOGICAL: 
    case MPI_INTEGER: 
#endif
/* The length type can be provided without Fortran, so we do so */
#ifdef MPIR_INTEGER1_CTYPE
    case MPI_INTEGER1:
#endif
#ifdef MPIR_INTEGER2_CTYPE
    case MPI_INTEGER2:
#endif
#ifdef MPIR_INTEGER4_CTYPE
    case MPI_INTEGER4:
#endif
#ifdef MPIR_INTEGER8_CTYPE
    case MPI_INTEGER8:
#endif
#ifdef MPIR_INTEGER16_CTYPE
    case MPI_INTEGER16:
#endif
#ifdef MPIR_REAL4_CTYPE
    case MPI_REAL4:
#endif
#ifdef MPIR_REAL8_CTYPE
    case MPI_REAL8:
#endif
#ifdef MPIR_REAL16_CTYPE
    case MPI_REAL16:
#endif
        return MPI_SUCCESS;
	/* --BEGIN ERROR HANDLING-- */
    default: 
        return MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OP, "**opundefined","**opundefined %s", "MPI_LAND" );
	/* --END ERROR HANDLING-- */
    }
}

