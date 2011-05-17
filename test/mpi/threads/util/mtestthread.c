/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/*
   Define macro to override gcc strict flags,
   -D_POSIX_C_SOURCE=199506L, -std=c89 and -std=c99,
   that disallow pthread_barrier_t and friends.
*/
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpitest.h"
#include "mpithreadtest.h"

/* This file provides a portability layer for using threads.  Currently, 
   it supports POSIX threads (pthreads) and Windows threads.  Testing has 
   been performed for pthreads.
 */

/* We remember all of the threads we create; this similifies terminating 
   (joining) them. */
#ifndef MTEST_MAX_THREADS
#define MTEST_MAX_THREADS 16
#endif

static MTEST_THREAD_HANDLE threads[MTEST_MAX_THREADS];
/* access w/o a lock is broken, but "volatile" should help reduce the amount of
 * speculative loading/storing */
static volatile int nthreads = 0;

#ifdef HAVE_WINDOWS_H
int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE (*fn)(void *p),void *arg)
{
    if (nthreads >= MTEST_MAX_THREADS) {
	fprintf( stderr, "Too many threads already created: max is %d\n",
		 MTEST_MAX_THREADS );
	return 1;
    }
    threads[nthreads] = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)fn, (LPVOID)arg, 0, NULL);
    if (threads[nthreads] == NULL){
        return GetLastError();
    }
    else{ 
        nthreads++;
    }
    return 0;
}

int MTest_Join_threads( void ){
    int i, err = 0;
    for (i=0; i<nthreads; i++) {
        if(threads[i] != INVALID_HANDLE_VALUE){
            if(WaitForSingleObject(threads[i], INFINITE) == WAIT_FAILED){
                err = GetLastError();
                fprintf(stderr, "Error WaitForSingleObject(), err = %d\n", err);
            }
            else{ 
                CloseHandle(threads[i]);
            }
        }
    }
    nthreads = 0;
    return err;
}

int MTest_thread_lock_create( MTEST_THREAD_LOCK_TYPE *lock )
{
    if(lock == NULL) return -1;

    /* Create an unnamed uninheritable mutex */
    *lock = CreateMutex(NULL, FALSE, NULL);
    if(*lock == NULL) return -1;

    return 0;
}

int MTest_thread_lock( MTEST_THREAD_LOCK_TYPE *lock )
{
    if(lock == NULL) return -1;

    /* Wait infinitely for the mutex */
    if(WaitForSingleObject(*lock, INFINITE) != WAIT_OBJECT_0){
        return -1;
    }
    return 0;
}

int MTest_thread_unlock( MTEST_THREAD_LOCK_TYPE *lock )
{
    if(lock == NULL) return -1;
    if(ReleaseMutex(*lock) == 0){
        return -1;
    }
    return 0;
}
int MTest_thread_lock_free( MTEST_THREAD_LOCK_TYPE *lock )
{
    if(lock != NULL){
        if(CloseHandle(*lock) == 0){
            return -1;
        }
    }
    return 0;
}

#else
int MTest_Start_thread(MTEST_THREAD_RETURN_TYPE (*fn)(void *p),void *arg)
{
    int err;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (nthreads >= MTEST_MAX_THREADS) {
	fprintf( stderr, "Too many threads already created: max is %d\n",
		 MTEST_MAX_THREADS );
	return 1;
    }
    err = pthread_create(threads+nthreads, &attr, fn, arg);
    if (!err) {
        nthreads++;
    }
    pthread_attr_destroy(&attr);
    return err;
}
int MTest_Join_threads( void )
{
    int i, rc, err = 0;
    for (i=0; i<nthreads; i++) {
        rc = pthread_join(threads[i], 0);
        if (rc) err = rc;
    }
    nthreads = 0;
    return err;
}
int MTest_thread_lock_create( MTEST_THREAD_LOCK_TYPE *lock )
{
    int err;
    err = pthread_mutex_init( lock, NULL );
    if (err) {
	perror( "Failed to initialize lock:" );
    }
    return err;
}
int MTest_thread_lock( MTEST_THREAD_LOCK_TYPE *lock )
{
    int err;
    err = pthread_mutex_lock( lock );
    if (err) {
	perror( "Failed to acquire lock:" );
    }
    return err;
}
int MTest_thread_unlock( MTEST_THREAD_LOCK_TYPE *lock )
{
    int err;
    err = pthread_mutex_unlock( lock );
    if (err) {
	perror( "Failed to release lock:" );
    }
    return err;
}
int MTest_thread_lock_free( MTEST_THREAD_LOCK_TYPE *lock )
{
    int err;
    err = pthread_mutex_destroy( lock );
    if (err) {
	perror( "Failed to free lock:" );
    }
    return err;
}
#endif

#if defined(HAVE_PTHREAD_BARRIER_INIT) && defined(USE_PTHREADS)
static MTEST_THREAD_LOCK_TYPE barrierLock;
static pthread_barrier_t barrier;
static int bcount = -1;
int MTest_thread_barrier_init( void )
{
    bcount = -1; /* must reset to force barrier re-creation */
    return MTest_thread_lock_create( &barrierLock );
}
int MTest_thread_barrier_free( void )
{
    MTest_thread_lock_free( &barrierLock );
    return pthread_barrier_destroy( &barrier );
}
/* FIXME this barrier interface should be changed to more closely match the
 * pthread interface.  Specifically, nt should not be a barrier-time
 * parameter but an init-time parameter.  The double-checked locking below
 * isn't valid according to pthreads, and it isn't guaranteed to be robust
 * in the presence of aggressive CPU/compiler optimization. */
int MTest_thread_barrier( int nt )
{
    int err;
    if (nt < 0) nt = nthreads;
    if (bcount != nt) {
	/* One thread needs to initialize the barrier */
	MTest_thread_lock( &barrierLock );
	/* Test again in case another thread already fixed the problem */
	if (bcount != nt) {
	    if (bcount > 0) {
		err = pthread_barrier_destroy( &barrier );
		if (err) return err;
	    }
	    err = pthread_barrier_init( &barrier, NULL, nt );
	    if (err) return err;
	    bcount = nt;
	}
	err = MTest_thread_unlock( &barrierLock );
	if (err) return err;
    }
    return pthread_barrier_wait( &barrier );
}
#else
static MTEST_THREAD_LOCK_TYPE barrierLock;
static volatile int   phase=0;
static int            c[2] = {-1,-1};
int MTest_thread_barrier_init( void )
{
    return MTest_thread_lock_create( &barrierLock );
}
int MTest_thread_barrier_free( void )
{
    return MTest_thread_lock_free( &barrierLock );
}
/* This is a generic barrier implementation.  To ensure that tests don't 
   silently fail, this both prints an error message and returns an error
   result on any failure. */
int MTest_thread_barrier( int nt )
{
    volatile int *cntP;
    int          err = 0;

    if (nt < 0) nt = nthreads;
    /* Force a write barrier by using lock/unlock */
    err = MTest_thread_lock( &barrierLock );
    if (err) { fprintf( stderr, "Lock failed in barrier!\n" ); return err; }
    cntP = &c[phase];
    err = MTest_thread_unlock( &barrierLock );
    if (err) { fprintf( stderr, "Unlock failed in barrier!\n" ); return err; }

    /* printf( "[%d] cnt = %d, phase = %d\n", pthread_self(), *cntP, phase ); */
    err = MTest_thread_lock( &barrierLock );
    if (err) { fprintf( stderr, "Lock failed in barrier!\n" ); return err; }
    /* The first thread to enter will reset the counter */
    if (*cntP < 0) *cntP = nt;
    /* printf( "phase = %d, cnt = %d\n", phase, *cntP ); */
    /* The last thread to enter will force the counter to be negative */
    if (*cntP == 1) { 
	/* printf( "[%d] changing phase from %d\n", pthread_self(), phase ); */
	phase = !phase; c[phase] = -1; *cntP = 0;
    }
    /* Really need a write barrier here */
    *cntP = *cntP - 1;
    err = MTest_thread_unlock( &barrierLock );
    if (err) { fprintf( stderr, "Unlock failed in barrier!\n" ); return err; }
    while (*cntP > 0) ;
    
    return err;
}
#endif /* Default barrier routine */
