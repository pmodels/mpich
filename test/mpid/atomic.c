/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/* 
 * Test performance of atomic access operations.
 * This is a *very* simple test.
 */

#include "mpiimpl.h"

/* FIXME: MPICH_SINGLE_THREADED is obsolete and no longer defined */
#if defined(MPICH_SINGLE_THREADED) || !defined(USE_ATOMIC_UPDATES)
#define MPID_Atomic_incr( count_ptr ) \
   __asm__ __volatile__ ( "lock; incl %0" \
                         : "=m" (*count_ptr) :: "memory", "cc" )

#define MPID_Atomic_decr_flag( count_ptr, nzflag )					\
   __asm__ __volatile__ ( "xor %%ax,%%ax; lock; decl %0 ; setnz %%al"			\
                         : "=m" (*count_ptr) , "=a" (nzflag) :: "memory", "cc" )
#endif

int main( int argc, char **argv )
{
    int                i, n;
    MPID_Thread_lock_t mutex;
    MPID_Time_t        start_t, end_t;
    double             time_lock, time_incr, time_single;
    int count;
    int nzflag;

    /* Set values */
    n = 10000000;

    /* Warm up */
    count = 0;
    MPID_Wtime( &start_t );
    for (i=0; i<1000; i++) {
	count ++;
    }
    MPID_Wtime( &end_t );

    /* Test nonatomic increment */
    count = 0;
    MPID_Wtime( &start_t );
    for (i=0; i<n; i++) {
	count ++;
    }
    MPID_Wtime( &end_t );
    MPID_Wtime_diff( &start_t, &end_t, &time_single );
    time_single /= n;
    if (count != n) {
	printf( "Error in nonatomic update\n" );
    }

    /* Test atomic increment using lock/unlock */
    count = 0;
    MPID_Thread_lock_init( &mutex );
    MPID_Wtime( &start_t );
    for (i=0; i<n; i++) {
	MPID_Thread_lock( &mutex );
	count ++;
	MPID_Thread_unlock( &mutex );
    }
    MPID_Wtime( &end_t );
    MPID_Wtime_diff( &start_t, &end_t, &time_lock );
    time_lock /= n;
    if (count != n) {
	printf( "Error in thread-locked atomic update\n" );
    }

    /* Test atomic increment using special instructions */
    count = 0;
    MPID_Wtime( &start_t );
    for (i=0; i<n; i++) {
	MPID_Atomic_incr( &count );
    }
    MPID_Wtime( &end_t );
    MPID_Wtime_diff( &start_t, &end_t, &time_incr );
    
    time_incr /= n;
    if (count != n) {
	printf( "Error in native atomic update (%d != %d)\n", count, n );
    }
    /* Check for dec sets zero flag */
    for (i=0; i<n-1; i++) {
	MPID_Atomic_decr_flag( &count, nzflag );
	if (!nzflag) {
	    printf( "flag not set on iteration %d\n", i );
	    break;
	}
    }
    MPID_Atomic_decr_flag( &count, nzflag );
    if (!nzflag) {
	printf( "Flag still set on final decrement\n" );
    }

    /* convert times to microseconds */
    time_single *= 1.0e6;
    time_lock *= 1.0e6;
    time_incr *= 1.0e6;
    printf ("Regular \t%f\nLock time \t%f\nAtomic time\t%f\n", 
	    time_single, time_lock, time_incr );

    {
	unsigned int low=0, high=0;
    __asm__ __volatile__  ( "rdtsc ; movl %%edx,%0 ; movl %%eax,%1" 
                            : "=r" (high), "=r" (low) ::
                            "eax", "edx" );

    printf ( "time stamp %u %u\n", high, low );
    }
    return 0;
}
