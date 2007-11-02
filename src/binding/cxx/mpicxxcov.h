/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2004 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
/* style: c++ header */
/* style: allow:printf:1 sig:0 */
//
// Include for the simple coverage analysis
//
#ifdef USE_COVERAGE_PRINT
#include <stdio.h>
#define COVERAGE_INITIALIZE()
#define COVERAGE_START(a,b)  printf( "%s-%d\n", #a, b ); fflush(stdout)
#define COVERAGE_END(a,b)
#define COVERAGE_FINALIZE()
#elif defined(USE_COVERAGE_SIMPLE)
#include "mpicovsimple.h"
#define COVERAGE_INITIALIZE()
#define COVERAGE_START(a,b) MPIR_Cov.Add( #a, b, __FILE__, __LINE__ )
#define COVERAGE_END(a,b)   MPIR_Cov.AddEnd( #a, b, __FILE__, __LINE__ )
#define COVERAGE_FINALIZE_NEEDED 1
#define COVERAGE_FINALIZE() MPIR_Cov.FileMerge( "cov.dat" )
#else
// Just make these empty
#define COVERAGE_INITIALIZE()
#define COVERAGE_START(a,b)
#define COVERAGE_END(a,b)
#define COVERAGE_FINALIZE()
#endif
