/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#if MPICH_TIMER_KIND == USE_GETHRTIME 
/* 
 * MPID_Time_t is hrtime_t, which under Solaris is defined as a 64bit
 * longlong_t .  However, the Solaris header files will define
 * longlong_t as a structure in some circumstances, making arithmetic
 * with hrtime_t invalid.  FIXME.  
 * To fix this, we'll need to test hrtime_t arithmetic in the configure
 * program, and if it fails, check for the Solaris defintions (
 * union { double _d; int32_t _l[2]; }.  Alternately, we may decide that
 * if hrtime_t is not supported, then neither is gethrtime.
 *
 * Note that the Solaris sys/types.h file *assumes* that no other compiler
 * supports an 8 byte long long.  We can also cast hrtime_t to long long 
 * if long long is available and 8 bytes.
 */
void MPID_Wtime( MPID_Time_t *timeval )
{
    *timeval = gethrtime();
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = 1.0e-9 * (double)( *t2 - *t1 );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = 1.0e-9 * (*t);
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += ((*t2) - (*t1));
}
double MPID_Wtick( void )
{
    /* According to the documentation, ticks should be in nanoseconds.  This 
       is untested */ 
    return 1.0e-9;
}

#elif MPICH_TIMER_KIND == USE_CLOCK_GETTIME
void MPID_Wtime( MPID_Time_t *timeval )
{
    /* POSIX timer (14.2.1, page 311) */
    clock_gettime( CLOCK_REALTIME, timeval );
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		1.0e-9 * (double) (t2->tv_nsec - t1->tv_nsec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = ((double) t->tv_sec + 1.0e-9 * (double) t->tv_nsec );
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
    int nsec, sec;
    
    nsec = t1->tv_nsec + t2->tv_nsec;
    sec  = t1->tv_sec + t2->tv_sec;
    if (nsec > 1.0e9) {
	nsec -= 1.0e9;
	sec++;
    }
    t3->tv_sec = sec;
    t3->tv_nsec = nsec;
}

/* FIXME: We need to cleanup the use of the MPID_Generic_wtick prototype */
double MPID_Generic_wtick(void);

double MPID_Wtick( void )
{
    struct timespec res;
    int rc;

    rc = clock_getres( CLOCK_REALTIME, &res );
    if (!rc) 
	/* May return -1 for unimplemented ! */
	return res.tv_sec + 1.0e-9 * res.tv_nsec;

    /* Sigh.  If not implemented (POSIX allows that), 
       then we need to use the generic tick routine */
    return MPID_Generic_wtick();
}
#define MPICH_NEEDS_GENERIC_WTICK
/* Rename the function so that we can access it */
#define MPID_Wtick MPID_Generic_wtick

#elif MPICH_TIMER_KIND == USE_GETTIMEOFDAY
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
void MPID_Wtime( MPID_Time_t *tval )
{
    gettimeofday(tval,NULL);
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 
		.000001 * (double) (t2->tv_usec - t1->tv_usec) );
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
    int usec, sec;
    
    usec = t2->tv_usec - t1->tv_usec;
    sec  = t2->tv_sec - t1->tv_sec;
    t3->tv_usec += usec;
    t3->tv_sec += sec;
    /* Handle carry to the integer seconds field */
    if (t3->tv_usec > 1.0e6) {
	t3->tv_usec -= 1.0e6;
	t3->tv_sec++;
    }
}
#define MPICH_NEEDS_GENERIC_WTICK


#elif MPICH_TIMER_KIND == USE_LINUX86_CYCLE
#include <sys/time.h>
double MPID_Seconds_per_tick=0.0;
double MPID_Wtick(void)
{
    return MPID_Seconds_per_tick;
}
int MPID_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPID_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPID_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    MPID_Seconds_per_tick = (td2 - td1) / (double)(t2 - t1);
    return 0;
}
/* Time stamps created by a macro */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = (double)( *t2 - *t1 ) * MPID_Seconds_per_tick;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    /* This returns the number of cycles as the "time".  This isn't correct
       for implementing MPI_Wtime, but it does allow us to insert cycle
       counters into test programs */
    *val = (double)*t * MPID_Seconds_per_tick;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += (*t2 - *t1);
}



#elif MPICH_TIMER_KIND == USE_GCC_IA64_CYCLE
#include <sys/time.h>
double MPID_Seconds_per_tick = 0.0;
double MPID_Wtick(void)
{
    return MPID_Seconds_per_tick;
}
int MPID_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPID_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPID_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    MPID_Seconds_per_tick = (td2 - td1) / (double)(t2 - t1);
    return 0;
}
/* Time stamps created by a macro */
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = (double)( *t2 - *t1 ) * MPID_Seconds_per_tick;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    /* This returns the number of cycles as the "time".  This isn't correct
       for implementing MPI_Wtime, but it does allow us to insert cycle
       counters into test programs */
    *val = (double)*t * MPID_Seconds_per_tick;
}
void MPID_Wtime_acc( MPID_Time_t *t1,MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 += (*t2 - *t1);
}

#elif MPICH_TIMER_KIND == USE_LINUXALPHA_CYCLE
/* FIXME: This should have been fixed in the configure, rather than as a 
   make-time error message */
#error "LinuxAlpha cycle counter not supported"

#elif (MPICH_TIMER_KIND == USE_WIN86_CYCLE) || (MPICH_TIMER_KIND == USE_WIN64_CYCLE)
double MPID_Seconds_per_tick = 0.0;
double MPID_Wtick(void)
{
    return MPID_Seconds_per_tick;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *d)
{
    *d = (double)(__int64)*t * MPID_Seconds_per_tick;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff)
{
    *diff = (double)((__int64)( *t2 - *t1 )) * MPID_Seconds_per_tick;
}
int MPID_Wtime_init( void )
{
    MPID_Time_t t1, t2;
    DWORD s1, s2;
    double d;
    int i;

    MPID_Wtime(&t1);
    MPID_Wtime(&t1);

    /* time an interval using both timers */
    s1 = GetTickCount();
    MPID_Wtime(&t1);
    /*Sleep(250);*/ /* Sleep causes power saving cpu's to stop which stops the counter */
    while (GetTickCount() - s1 < 200)
    {
	for (i=2; i<1000; i++)
	    d = (double)i / (double)(i-1);
    }
    s2 = GetTickCount();
    MPID_Wtime(&t2);

    /* calculate the frequency of the assembly cycle counter */
    MPID_Seconds_per_tick = ((double)(s2 - s1) / 1000.0) / (double)((__int64)(t2 - t1));
    /*
    printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)(t2-t1), (int)(s2 - s1), MPID_Seconds_per_tick, MPID_Seconds_per_tick * 1.0e6);
    */
    return 0;
}
/*
void TIMER_INIT()
{
    TIMER_TYPE t1, t2;
    FILETIME ft1, ft2;
    SYSTEMTIME st1, st2;
    ULARGE_INTEGER u1, u2;

    t1 = 5;
    t2 = 5;

    GET_TIME(&t1);
    GET_TIME(&t1);

    GetSystemTime(&st1);
    GET_TIME(&t1);
    Sleep(500);
    GetSystemTime(&st2);
    GET_TIME(&t2);

    SystemTimeToFileTime(&st1, &ft1);
    SystemTimeToFileTime(&st2, &ft2);

    u1.QuadPart = ft1.dwHighDateTime;
    u1.QuadPart = u1.QuadPart << 32;
    u1.QuadPart |= ft1.dwLowDateTime;
    u2.QuadPart = ft2.dwHighDateTime;
    u2.QuadPart = u2.QuadPart << 32;
    u2.QuadPart |= ft2.dwLowDateTime;

    MPID_Seconds_per_tick = (1e-7 * (double)((__int64)(u2.QuadPart - u1.QuadPart))) / (double)((__int64)(t2 - t1));
    printf("t2   %10d\nt1   %10d\ndiff %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)t2, (int)t1, (int)(t2-t1), (int)(u2.QuadPart - u1.QuadPart), MPID_Seconds_per_tick, MPID_Seconds_per_tick * 1.0e6);
    printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n", 
	(int)(t2-t1), (int)(u2.QuadPart - u1.QuadPart), MPID_Seconds_per_tick, MPID_Seconds_per_tick * 1.0e6);
}
*/



#elif MPICH_TIMER_KIND == USE_QUERYPERFORMANCECOUNTER
double MPID_Seconds_per_tick=0.0;  /* High performance counter frequency */
int MPID_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    MPID_Seconds_per_tick = 1.0 / (double)n.QuadPart;
    return 0;
}
double MPID_Wtick(void)
{
    return MPID_Seconds_per_tick;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = (double)t->QuadPart * MPID_Seconds_per_tick;
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double)n.QuadPart * MPID_Seconds_per_tick;
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
    t3->QuadPart += ((t2->QuadPart) - (t1->QuadPart));
}


#elif MPICH_TIMER_KIND == USE_MACH_ABSOLUTE_TIME
static double MPIR_Wtime_mult;
int MPID_Wtime_init(void)
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    MPIR_Wtime_mult = 1.0e-9 * ((double)info.numer / (double)info.denom);

    return MPI_SUCCESS;
}
void MPID_Wtime( MPID_Time_t *timeval )
{
    *timeval = mach_absolute_time();  
}
void MPID_Wtime_diff( MPID_Time_t *t1, MPID_Time_t *t2, double *diff )
{
    *diff = (*t2 - *t1) * MPIR_Wtime_mult;
}
void MPID_Wtime_todouble( MPID_Time_t *t, double *val )
{
    *val = *t * MPIR_Wtime_mult;
}
void MPID_Wtime_acc( MPID_Time_t *t1, MPID_Time_t *t2, MPID_Time_t *t3 )
{
    *t3 = *t1 + *t2;
}

/* FIXME: We need to cleanup the use of the MPID_Generic_wtick prototype */
double MPID_Generic_wtick(void);

double MPID_Wtick( void )
{
    return MPID_Generic_wtick();
}
#define MPICH_NEEDS_GENERIC_WTICK
/* Rename the function so that we can access it */
#define MPID_Wtick MPID_Generic_wtick


#endif

#ifdef MPICH_NEEDS_GENERIC_WTICK
/*
 * For timers that do not have defined resolutions, compute the resolution
 * by sampling the clock itself.
 *
 * Note that this uses a thread-safe initialization procedure in the
 * event that multiple threads invoke this routine
 */
#undef FUNCNAME
#define FUNCNAME MPID_Wtick
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
double MPID_Wtick( void )
{
    MPIU_THREADSAFE_INIT_DECL(initTick);
    static double tickval = -1.0;
    double timediff;  /* Keep compiler happy about timediff set before use*/
    MPID_Time_t t1, t2;
    int    cnt;
    int    icnt;

    if (initTick) {
	MPIU_THREADSAFE_INIT_BLOCK_BEGIN(initTick);
	tickval = 1.0e6;
	for (icnt=0; icnt<10; icnt++) {
	    cnt = 1000;
	    MPID_Wtime( &t1 );
	    /* Make it abundently clear to the compiler that this block is
	       executed at least once. */
	    do {
		MPID_Wtime( &t2 );
		MPID_Wtime_diff( &t1, &t2, &timediff );
		if (timediff > 0) break;
	    }
	    while (cnt--);
	    if (cnt && timediff > 0.0 && timediff < tickval) {
		MPID_Wtime_diff( &t1, &t2, &tickval );
	    }
	}
	MPIU_THREADSAFE_INIT_CLEAR(initTick);
	MPIU_THREADSAFE_INIT_BLOCK_END(initTick);
    }
    return tickval;
}
#endif
