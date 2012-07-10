/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/***********************************************************************
This header file provides some simple functions to use the pentium
cycle counter to take timings.  Let me know if you find any errors.
Darius Buntinas (buntinas@cis...)

Please note that version 2 (this file) has a new interface from the
original (rdtsc.h).  Here are the changes from rdtsc.h:

TIME_DECLARE and TIME_DECLARE_EXTERN are no longer used.

TIME_INIT is not needed, unless you use USECS_DELAY.  Use this as a
  statement in main() (i.e. TIME_INIT; not as a function: TIME_INIT();).

TIME_PRE(x) now takes an unsigned long long parameter.  You should
  pass this parameter to TIME_POST.  This allows you to do nested
  timings.

TIME_POST(x) works pretty much as before.  The parameter x must have
  been set by TIME_PRE previously.

USECS(x) converts processor cycles to microseconds.  Note that this
  will do some file I/O the first time it is called to determine the
  processor speed, so you probably don't want to use this between a
  TIME_PRE and TIME_POST.  If you used to use this to implement a
  delay loop, use USECS_DELAY instead (or if you are really stubborn,
  and want to use it like this, just make sure you do a TIME_INIT
  first.  It will then work as usual.);

USECS_DELAY(x) is a new function which will loop for x microseconds.
  Because this is a time-critical function, the processor speed must
  be initialized before this function is called.  This can be done
  explicitly, by doing a TIME_INIT, or implicitly, by calling USECS().
  This function returns the actual time delayed in microseconds.

To time something between modules (i.e. .o files) in one file declare
the timing variable as a global, and in the other file declare is as
extern.  Then use TIME_PRE and TIME_POST as usual.  E.g.:
fileA.c:
  unsigned long long my_timing_tmp_var;
  ...
  TIME_PRE(my_timing_tmp_var);
  ...
fileB.c
  extern unsigned long long my_timing_tmp_var;
  ...
  TIME_POST(my_timing_tmp_var);
  ...

Here is a sample program showing how to use TIME_PRE and TIME_POST.

#include <stdio.h>
#include "rdtsc2.h"

int
main() {

  unsigned long long tmp;
  int i = 0;

  TIME_PRE(tmp);
  for (i = 0; i < 1000; ++i)
    printf("hello\n");
  TIME_POST(tmp);

  printf ("1000 printfs took %5.3fus.  That's %5.3fus per printf!\n", USECS(tmp),
          USECS(tmp)/1000);

  return 0;
}
****************************************************************************/

#ifndef __RDTSC_H
#define __RDTSC_H
#include <stdlib.h>
#include <stdio.h>
/*#include "asm/msr.h" */
#define rdtsc(x) __asm__ __volatile__("rdtsc" : "=A" (x))

#define TIME_INIT do {__cpuMHz = SetMHz();} while(0)
#define TIME_PRE(cycles) rdtsc(cycles)
#define TIME_POST(cycles) do { unsigned long long __tmp;		\
                               rdtsc(__tmp);				\
                               (cycles) = __tmp - (cycles); } while (0)
#ifdef TIME_DEBUG_MHZ
static double __cpuMHz = TIME_DEBUG_MHZ;
#else
static double __cpuMHz = -1.0;
#endif

static inline double
SetMHz()
{
  double mhz;
  FILE* f;
  
  f = popen("/bin/sed -n '/cpu MHz/s/[^:]*://p' /proc/cpuinfo", "r");
  /*f = popen("/home/1/buntinas/bin/catsedn '/cpu MHz/s/[^:]*://p' /proc/cpuinfo", "r");*/

  if (!f) {
    printf ("RDTSC: Error reading cpu speed\n");
    exit (-1);
  }
  
  if (fscanf(f, "%lf", &mhz) != 1) {
    printf ("RDTSC: Error reading cpu speed.\n");
    exit (-1);
  }
  
  pclose(f);

#if TIME_DEBUG
  printf("Processor speed = %8.3f\n", mhz);
#endif

  return mhz;
}

static inline double
USECS(unsigned long long cycles)
{
  if (__cpuMHz < 0) 
    __cpuMHz = SetMHz();

  return (double)(cycles)/__cpuMHz;
} 

static inline double
USECS_DELAY(double usecs)
{
  unsigned long long tmp1, tmp2, delay;
  double elapsed;

  rdtsc(tmp1);

  if (__cpuMHz < 0) {
    printf ("USECS_DELAY: You need to do a TIME_PRE or USECS() before calling "
	    "USECS_DELAY\n");
    exit(-1);
  }
  
  delay = usecs * __cpuMHz;
  do {
    rdtsc(tmp2);
    elapsed = tmp2 - tmp1;
  } while (elapsed < delay);

  return (double)elapsed / __cpuMHz;
}
    

#endif /* __RDTSC_H */





