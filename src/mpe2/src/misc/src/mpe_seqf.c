/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/* mpe_seq.c */
/* Fortran interface file */
#include "mpe_conf.h"
#include "mpe_misc.h"

#include <stdio.h>

#ifdef MPI_BUILD_PROFILING
#ifdef F77_NAME_UPPER
#define mpe_seq_begin_ PMPE_SEQ_BEGIN
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_seq_begin_ pmpe_seq_begin__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_seq_begin_ pmpe_seq_begin
#else
#define mpe_seq_begin_ pmpe_seq_begin_
#endif
#else
#ifdef F77_NAME_UPPER
#define mpe_seq_begin_ MPE_SEQ_BEGIN
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_seq_begin_ mpe_seq_begin__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_seq_begin_ mpe_seq_begin
#endif
#endif

void mpe_seq_begin_ ( MPI_Comm *, int *, int * );

void  mpe_seq_begin_( MPI_Comm *comm, int *ng, int * __ierr )
{
    MPE_Seq_begin(*comm, *ng);
}
#ifdef MPI_BUILD_PROFILING
#ifdef F77_NAME_UPPER
#define mpe_seq_end_ PMPE_SEQ_END
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_seq_end_ pmpe_seq_end__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_seq_end_ pmpe_seq_end
#else
#define mpe_seq_end_ pmpe_seq_end_
#endif
#else
#ifdef F77_NAME_UPPER
#define mpe_seq_end_ MPE_SEQ_END
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_seq_end_ mpe_seq_end__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_seq_end_ mpe_seq_end
#endif
#endif

void mpe_seq_end_ ( MPI_Comm *, int *, int * );

void  mpe_seq_end_( MPI_Comm *comm, int *ng, int *__ierr )
{
    MPE_Seq_end(*comm, *ng);
}
