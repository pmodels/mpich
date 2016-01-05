/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIU_TIMER_DEVICE_H_INCLUDED
#define MPIU_TIMER_DEVICE_H_INCLUDED

#define MPIUI_WTIME_IS_A_FUNCTION

/*
 * If the timer capability is provided by the device, it would need to
 * expose it in two ways:
 *
 *    1. It would expose it through the MPID_ functionality.  This
 *       model would be used by the MPICH internal code, and would
 *       give fast access to the timers.
 *
 *    2. It would expose it through the MPIU_Timer_ function pointers.
 *       This model would be used by "external" code segments (such as
 *       MPIU) which do not have direct access to the MPICH devices.
 *       This model might be slightly slower, but would give the same
 *       functionality.
 */
extern int (*MPIU_Wtime_fn)(MPIU_Time_t *timeval);
extern int (*MPIU_Wtime_diff_fn)(MPIU_Time_t *t1, MPIU_Time_t *t2, double *diff);
extern int (*MPIU_Wtime_acc_fn)(MPIU_Time_t *t1, MPIU_Time_t *t2, MPIU_Time_t *t3);
extern int (*MPIU_Wtime_todouble_fn)(MPIU_Time_t *timeval, double *seconds);
extern int (*MPIU_Wtick_fn)(double *tick);

#endif
