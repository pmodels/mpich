/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

/*
 * For timers that do not have defined resolutions, compute the resolution
 * by sampling the clock itself.
 *
 */
static double tickval = -1.0;

static void init_wtick(void) ATTRIBUTE((unused));
static void init_wtick(void)
{
    double timediff;
    MPIU_Time_t t1, t2;
    int cnt;
    int icnt;

    tickval = 1.0e6;
    for (icnt = 0; icnt < 10; icnt++) {
        cnt = 1000;
        MPIU_Wtime(&t1);
        do {
            MPIU_Wtime(&t2);
            MPIU_Wtime_diff(&t1, &t2, &timediff);
            if (timediff > 0)
                break;
        }
        while (cnt--);
        if (cnt && timediff > 0.0 && timediff < tickval) {
            MPIU_Wtime_diff(&t1, &t2, &tickval);
        }
    }
}

#if MPICH_TIMER_KIND == MPIU_GETHRTIME
/*
 * MPIU_Time_t is hrtime_t, which under Solaris is defined as a 64bit
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
void MPIU_Wtime(MPIU_Time_t * timeval)
{
    *timeval = gethrtime();
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = 1.0e-9 * (double) (*t2 - *t1);
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = 1.0e-9 * (*t);
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += ((*t2) - (*t1));
}

double MPIU_Wtick(void)
{
    /* According to the documentation, ticks should be in nanoseconds.  This
     * is untested */
    return 1.0e-9;
}

int MPIU_Wtime_init(void)
{
    return 0;
}

#elif MPICH_TIMER_KIND == MPIU_CLOCK_GETTIME
void MPIU_Wtime(MPIU_Time_t * timeval)
{
    /* POSIX timer (14.2.1, page 311) */
    clock_gettime(CLOCK_REALTIME, timeval);
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + 1.0e-9 * (double) (t2->tv_nsec - t1->tv_nsec));
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = ((double) t->tv_sec + 1.0e-9 * (double) t->tv_nsec);
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    int nsec, sec;

    nsec = t2->tv_nsec - t1->tv_nsec;
    sec = t2->tv_sec - t1->tv_sec;

    t3->tv_sec += sec;
    t3->tv_nsec += nsec;
    while (t3->tv_nsec > 1000000000) {
        t3->tv_nsec -= 1000000000;
        t3->tv_sec++;
    }
}

double MPIU_Wtick(void)
{
    struct timespec res;
    int rc;

    rc = clock_getres(CLOCK_REALTIME, &res);
    if (!rc)
        /* May return -1 for unimplemented ! */
        return res.tv_sec + 1.0e-9 * res.tv_nsec;

    /* Sigh.  If not implemented (POSIX allows that),
     * then we need to return the generic tick value */
    return tickval;
}

int MPIU_Wtime_init(void)
{
    init_wtick();
    return 0;
}


#elif MPICH_TIMER_KIND == MPIU_GETTIMEOFDAY
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
void MPIU_Wtime(MPIU_Time_t * tval)
{
    gettimeofday(tval, NULL);
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = ((double) (t2->tv_sec - t1->tv_sec) + .000001 * (double) (t2->tv_usec - t1->tv_usec));
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = (double) t->tv_sec + .000001 * (double) t->tv_usec;
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    int usec, sec;

    usec = t2->tv_usec - t1->tv_usec;
    sec = t2->tv_sec - t1->tv_sec;
    t3->tv_usec += usec;
    t3->tv_sec += sec;
    /* Handle carry to the integer seconds field */
    while (t3->tv_usec > 1000000) {
        t3->tv_usec -= 1000000;
        t3->tv_sec++;
    }
}

double MPIU_Wtick(void)
{
    return tickval;
}

int MPIU_Wtime_init(void)
{
    init_wtick();
    return 0;
}


#elif MPICH_TIMER_KIND == MPIU_LINUX86_CYCLE
#include <sys/time.h>
double MPIU_Seconds_per_tick = 0.0;
double MPIU_Wtick(void)
{
    return MPIU_Seconds_per_tick;
}

int MPIU_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPIU_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPIU_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    MPIU_Seconds_per_tick = (td2 - td1) / (double) (t2 - t1);
    return 0;
}

/* Time stamps created by a macro */
void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (double) (*t2 - *t1) * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    /* This returns the number of cycles as the "time".  This isn't correct
     * for implementing MPI_Wtime, but it does allow us to insert cycle
     * counters into test programs */
    *val = (double) *t * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += (*t2 - *t1);
}


#elif MPICH_TIMER_KIND == MPIU_GCC_IA64_CYCLE
#include <sys/time.h>
double MPIU_Seconds_per_tick = 0.0;
double MPIU_Wtick(void)
{
    return MPIU_Seconds_per_tick;
}

int MPIU_Wtime_init(void)
{
    unsigned long long t1, t2;
    struct timeval tv1, tv2;
    double td1, td2;

    gettimeofday(&tv1, NULL);
    MPIU_Wtime(&t1);
    usleep(250000);
    gettimeofday(&tv2, NULL);
    MPIU_Wtime(&t2);

    td1 = tv1.tv_sec + tv1.tv_usec / 1000000.0;
    td2 = tv2.tv_sec + tv2.tv_usec / 1000000.0;

    MPIU_Seconds_per_tick = (td2 - td1) / (double) (t2 - t1);
    return 0;
}

/* Time stamps created by a macro */
void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (double) (*t2 - *t1) * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    /* This returns the number of cycles as the "time".  This isn't correct
     * for implementing MPI_Wtime, but it does allow us to insert cycle
     * counters into test programs */
    *val = (double) *t * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += (*t2 - *t1);
}

#elif MPICH_TIMER_KIND == MPIU_LINUXALPHA_CYCLE
/* FIXME: This should have been fixed in the configure, rather than as a
   make-time error message */
#error "LinuxAlpha cycle counter not supported"

#elif (MPICH_TIMER_KIND == MPIU_WIN86_CYCLE) || (MPICH_TIMER_KIND == MPIU_WIN64_CYCLE)
double MPIU_Seconds_per_tick = 0.0;
double MPIU_Wtick(void)
{
    return MPIU_Seconds_per_tick;
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *d)
{
    *d = (double) (__int64) * t * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (double) ((__int64) (*t2 - *t1)) * MPIU_Seconds_per_tick;
}

int MPIU_Wtime_init(void)
{
    MPIU_Time_t t1, t2;
    DWORD s1, s2;
    double d;
    int i;

    MPIU_Wtime(&t1);
    MPIU_Wtime(&t1);

    /* time an interval using both timers */
    s1 = GetTickCount();
    MPIU_Wtime(&t1);
    /*Sleep(250); *//* Sleep causes power saving cpu's to stop which stops the counter */
    while (GetTickCount() - s1 < 200) {
        for (i = 2; i < 1000; i++)
            d = (double) i / (double) (i - 1);
    }
    s2 = GetTickCount();
    MPIU_Wtime(&t2);

    /* calculate the frequency of the assembly cycle counter */
    MPIU_Seconds_per_tick = ((double) (s2 - s1) / 1000.0) / (double) ((__int64) (t2 - t1));
    /*
     * printf("t2-t1 %10d\nsystime diff %d\nfrequency %g\n CPU MHz %g\n",
     * (int)(t2-t1), (int)(s2 - s1), MPIU_Seconds_per_tick, MPIU_Seconds_per_tick * 1.0e6);
     */
    return 0;
}
#elif MPICH_TIMER_KIND == MPIU_QUERYPERFORMANCECOUNTER
double MPIU_Seconds_per_tick = 0.0;     /* High performance counter frequency */
int MPIU_Wtime_init(void)
{
    LARGE_INTEGER n;
    QueryPerformanceFrequency(&n);
    MPIU_Seconds_per_tick = 1.0 / (double) n.QuadPart;
    return 0;
}

double MPIU_Wtick(void)
{
    return MPIU_Seconds_per_tick;
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = (double) t->QuadPart * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    LARGE_INTEGER n;
    n.QuadPart = t2->QuadPart - t1->QuadPart;
    *diff = (double) n.QuadPart * MPIU_Seconds_per_tick;
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    t3->QuadPart += ((t2->QuadPart) - (t1->QuadPart));
}


#elif MPICH_TIMER_KIND == MPIU_MACH_ABSOLUTE_TIME
static double MPIR_Wtime_mult;

int MPIU_Wtime_init(void)
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    MPIR_Wtime_mult = 1.0e-9 * ((double) info.numer / (double) info.denom);
    init_wtick();
    return 0;
}

void MPIU_Wtime(MPIU_Time_t * timeval)
{
    *timeval = mach_absolute_time();
}

void MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = (*t2 - *t1) * MPIR_Wtime_mult;
}

void MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = *t * MPIR_Wtime_mult;
}

void MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += *t2 - *t1;
}

double MPIU_Wtick(void)
{
    return tickval;
}

#endif
