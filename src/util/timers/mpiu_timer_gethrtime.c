/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_timer.h"

MPL_SUPPRESS_OSX_HAS_NO_SYMBOLS_WARNING;

#if MPICH_TIMER_KIND == MPIU_TIMER_KIND__GETHRTIME

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
int MPIU_Wtime(MPIU_Time_t * timeval)
{
    *timeval = gethrtime();

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_diff(MPIU_Time_t * t1, MPIU_Time_t * t2, double *diff)
{
    *diff = 1.0e-9 * (double) (*t2 - *t1);

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_todouble(MPIU_Time_t * t, double *val)
{
    *val = 1.0e-9 * (*t);

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_acc(MPIU_Time_t * t1, MPIU_Time_t * t2, MPIU_Time_t * t3)
{
    *t3 += ((*t2) - (*t1));

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtick(double *wtick)
{
    /* According to the documentation, ticks should be in nanoseconds.  This
     * is untested */
    *wtick = 1.0e-9;

    return MPIU_TIMER_SUCCESS;
}

int MPIU_Wtime_init(void)
{
    return MPIU_TIMER_SUCCESS;
}

#endif
