/*
   (C) 2001 by Argonne National Laboratory.
       See COPYRIGHT in top-level directory.
*/
/* dbxerr.c */
/* Fortran interface file */
#include <stdio.h>
#include "mpe_conf.h"
#include "mpe_misc_conf.h"
#include "mpe_misc.h"

#ifdef MPI_BUILD_PROFILING
#ifdef F77_NAME_UPPER
#define mpe_errors_call_debugger_ PMPE_ERRORS_CALL_DEBUGGER
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_errors_call_debugger_ pmpe_errors_call_debugger__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_errors_call_debugger_ pmpe_errors_call_debugger
#else
#define mpe_errors_call_debugger_ pmpe_errors_call_debugger_
#endif
#else
#ifdef F77_NAME_UPPER
#define mpe_errors_call_debugger_ MPE_ERRORS_CALL_DEBUGGER
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_errors_call_debugger_ mpe_errors_call_debugger__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_errors_call_debugger_ mpe_errors_call_debugger
#endif
#endif

 void  mpe_errors_call_debugger_( pgm, dbg, args, __ierr )
char *pgm, *dbg, **args;
int *__ierr;
{
MPE_Errors_call_debugger(pgm,dbg,args);
}
#ifdef MPI_BUILD_PROFILING
#ifdef F77_NAME_UPPER
#define mpe_signals_call_debugger_ PMPE_SIGNALS_CALL_DEBUGGER
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_signals_call_debugger_ pmpe_signals_call_debugger__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_signals_call_debugger_ pmpe_signals_call_debugger
#else
#define mpe_signals_call_debugger_ pmpe_signals_call_debugger_
#endif
#else
#ifdef F77_NAME_UPPER
#define mpe_signals_call_debugger_ MPE_SIGNALS_CALL_DEBUGGER
#elif defined(F77_NAME_LOWER_2USCORE)
#define mpe_signals_call_debugger_ mpe_signals_call_debugger__
#elif !defined(F77_NAME_LOWER_USCORE)
#define mpe_signals_call_debugger_ mpe_signals_call_debugger
#endif
#endif

MPE_Signals_call_debugger mpe_signals_call_debugger_(__ierr )
int *__ierr;
{
*__ierr = MPE_Signals_call_debugger();
}
