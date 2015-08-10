/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* for ATTRIBUTE */
#include "mpichconf.h"
#include "mpl.h"

/* here to prevent "has no symbols" warnings from ranlib on OS X */
static int dummy ATTRIBUTE((unused,used)) = 0;

#ifdef PAPI_MONITOR
#include <papi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <my_papi_defs.h>


unsigned long long tmp2 = 0;
int DO_TEST = 0;

int PAPI_events[NEVENTS]  = {PAPI_TOT_INS,
                             PAPI_TOT_CYC,
                             /*PAPI_RES_STL, */
                             PAPI_L1_DCM,
                             PAPI_L1_ICM,
                             PAPI_L2_LDM,
                             PAPI_L2_STM};

char PAPI_msgs[NEVENTS][PAPI_MAX_STR_LEN] = {"Number of Instructions          ",
                                             "Number of Cycles                ",
                                             /*"Number of Cycles (stalled)      ", */
                                             "Number of L1 Cache Data misses  ",
                                             "Number of L1 Cache Inst misses  ",
                                             "Number of L2 Cache load misses  ",
                                             "Number of L2 Cache store misses "};

int        PAPI_EventSet         = PAPI_NULL ;
/* long_long  PAPI_overhead_values[NEVENTS]  = { 240, 1040, /\*0,*\/ 6, 1, 0, 0 };  */

long_long  PAPI_dummy_values[NEVENTS]  = {(long_long) 0 };
long_long  PAPI_values[NEVENTS]  = {(long_long) 0 };
long_long  PAPI_values2[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values3[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values4[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values5[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values6[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values7[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values8[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values9[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values10[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values11[NEVENTS] = {(long_long) 0 };
long_long  PAPI_values12[NEVENTS] = {(long_long) 0 };

/* double  PAPI_overhead_vvalues[NEVENTS] = {240.0, 1013.0, /\*0.0,*\/ 12.0, 1.0, 0.0, 0.0};  */
double  PAPI_overhead_vvalues[NEVENTS] = {222.0, 960.0, /*0.0,*/ 5.0, 2.0, 0.0, 0.0};
long_long  PAPI_vvalues1[2][NEVENTS];
long_long  PAPI_vvalues2[2][NEVENTS];
long_long  PAPI_vvalues3[2][NEVENTS];
long_long  PAPI_vvalues4[2][NEVENTS];
long_long  PAPI_vvalues5[2][NEVENTS];
long_long  PAPI_vvalues6[2][NEVENTS];
long_long  PAPI_vvalues7[2][NEVENTS];
long_long  PAPI_vvalues8[2][NEVENTS];
long_long  PAPI_vvalues9[2][NEVENTS];
long_long  PAPI_vvalues10[2][NEVENTS];
long_long  PAPI_vvalues11[2][NEVENTS];
long_long  PAPI_vvalues12[2][NEVENTS];
long_long  PAPI_vvalues13[2][NEVENTS];
long_long  PAPI_vvalues14[2][NEVENTS];
long_long  PAPI_vvalues15[2][NEVENTS];
long_long  PAPI_vvalues16[2][NEVENTS];
long_long  PAPI_vvalues17[2][NEVENTS];

int iter11 = 0;


void my_papi_start( int myrank )
{
#ifdef PAPI_MONITOR
  int retval, index ;
  char EventCodeStr[PAPI_MAX_STR_LEN];
  
  if(myrank == 0)
    DO_TEST = 1;
  
  retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT)
    {
      fprintf(stderr, "PAPI library init error! %d\n", retval);
      exit(1);
    }
  fprintf(stderr,"PAPI Library start ...\n");

  if (PAPI_create_eventset(&PAPI_EventSet) != PAPI_OK)
    exit(1);

  for (index = 0 ; index < NEVENTS ; index ++)
    {
      PAPI_event_code_to_name(PAPI_events[index], EventCodeStr);
      /*fprintf(stderr,"Adding event %s ... ",EventCodeStr); */
      if (PAPI_add_event(PAPI_EventSet,PAPI_events[index]) != PAPI_OK)
	exit(1);
      /*fprintf(stderr," DONE !\n"); */
    }
  /*fprintf(stderr,"\n"); */

  if (PAPI_start(PAPI_EventSet) != PAPI_OK)
    exit(1);
#endif /*PAPI_MONITOR */
}

void my_papi_close(void)
{
#ifdef PAPI_MONITOR
  PAPI_shutdown();
  fprintf(stderr,"PAPI library shutdown ... \n");
#endif /*PAPI_MONITOR */
}


int
PAPI_accum_min (int EventSet, long_long *values)
{

    long_long  dummy_values[NEVENTS];
    long_long a;
    long_long b;
    int i;
    
    PAPI_read (EventSet, dummy_values);
    
    for (i = 0; i < NEVENTS; i++) {
	a = dummy_values[i];
	b = values[i];
	if (a < b)
	    values[i] = a;
    }

    return (PAPI_reset(EventSet));
}

int
PAPI_accum_var (int EventSet, long_long values[2][NEVENTS])
{

    long_long  dummy_values[NEVENTS];
    long_long a;
    int i;
    
    PAPI_read (EventSet, dummy_values);
    
    for (i = 0; i < NEVENTS; i++) {
	a = dummy_values[i];
	values[0][i] += a;
	values[1][i] += a * a;
    }

    return (PAPI_reset(EventSet));
}
#endif /*PAPI_MONITOR */
