/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef _MY_PAPI_DEFS_H
#define _MY_PAPI_DEFS_H

#ifdef PAPI_MONITOR
#undef ENABLE_PAPI_TIMING 
#else
#undef ENABLE_PAPI_TIMING 
#endif 

#ifdef ENABLE_PAPI_TIMING
#define DO_PAPI(x) x
#else  /*ENABLE_PAPI_TIMING */
#define DO_PAPI(x)
#endif /*ENABLE_PAPI_TIMING */

#define DO_PAPI2(x) /*x */

#ifdef PAPI_MONITOR
#define NEVENTS 6

#include <papi.h>
#include <math.h>

extern unsigned long long  tmp2;
extern int        DO_TEST;
extern int        PAPI_events[NEVENTS];
extern char       PAPI_msgs[NEVENTS][PAPI_MAX_STR_LEN];
extern int        PAPI_EventSet;
extern long_long  PAPI_overhead_values[NEVENTS];
extern long_long  PAPI_dummy_values[NEVENTS];
extern long_long  PAPI_values[NEVENTS];
extern long_long  PAPI_values2[NEVENTS];
extern long_long  PAPI_values3[NEVENTS];
extern long_long  PAPI_values4[NEVENTS];
extern long_long  PAPI_values5[NEVENTS];
extern long_long  PAPI_values6[NEVENTS];
extern long_long  PAPI_values7[NEVENTS];
extern long_long  PAPI_values8[NEVENTS];
extern long_long  PAPI_values9[NEVENTS];
extern long_long  PAPI_values10[NEVENTS];
extern long_long  PAPI_values11[NEVENTS];
extern long_long  PAPI_values12[NEVENTS];


extern double  PAPI_overhead_vvalues[NEVENTS];
extern long_long  PAPI_vvalues1[2][NEVENTS];
extern long_long  PAPI_vvalues2[2][NEVENTS];
extern long_long  PAPI_vvalues3[2][NEVENTS];
extern long_long  PAPI_vvalues4[2][NEVENTS];
extern long_long  PAPI_vvalues5[2][NEVENTS];
extern long_long  PAPI_vvalues6[2][NEVENTS];
extern long_long  PAPI_vvalues7[2][NEVENTS];
extern long_long  PAPI_vvalues8[2][NEVENTS];
extern long_long  PAPI_vvalues9[2][NEVENTS];
extern long_long  PAPI_vvalues10[2][NEVENTS];
extern long_long  PAPI_vvalues11[2][NEVENTS];
extern long_long  PAPI_vvalues12[2][NEVENTS];
extern long_long  PAPI_vvalues13[2][NEVENTS];
extern long_long  PAPI_vvalues14[2][NEVENTS];
extern long_long  PAPI_vvalues15[2][NEVENTS];
extern long_long  PAPI_vvalues16[2][NEVENTS];
extern long_long  PAPI_vvalues17[2][NEVENTS];

extern int iter11;


void my_papi_start( int myrank );
void my_papi_close(void);

int PAPI_accum_min (int EventSet, long_long *values);
int PAPI_accum_var (int EventSet, long_long values[2][NEVENTS]);

#endif  /*PAPI_MONITOR */
#endif /*_MY_PAPI_DEFS_H */
