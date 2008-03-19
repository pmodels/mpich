/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include "mpidu_process_locks.h"
#include "mpidimpl.h"

#include <errno.h>
#include <string.h> 

#ifdef HAVE_WINDOWS_H
#include <winsock2.h>
#include <windows.h>
#endif

/* FIXME: This definition is wrong in two ways.  First, it violates
   the naming convention for user-visible symbols; a valid user program
   could reset this value by simply using the value in the user program.
   Second and more serious, the build scheme for this file is broken,
   and this definition illustrates the problem.  The issue is that some
   files include mpidu_process_locks.h with *different* definitions than
   are used to compile this file!  That shows up when this variable, 
   which is only used for busy locks, is only conditionally defined when
   busy locks are defined.  The problem is that code in, for example, 
   the ch3:ssm channel includes mpidu_process_locks.h with USE_BUSY_LOCKS
   defined, but the configure in the mpid/common/locks directory will
   *never* define USE_BUSY_LOCKS */
/* #ifdef USE_BUSY_LOCKS */
int g_nLockSpinCount = 100;
/* #endif */

/* To make it easier to read the code, the implementation of all of the 
   functions are separated by type (e.g., all of the USE_NT_LOCKS 
   versions are together. */


/* FIXME: Why is this here? Is this a misnamed use-inline-locks? */
#if !defined(USE_BUSY_LOCKS) && !defined(HAVE_MUTEX_INIT) && !defined(HAVE_SPARC_INLINE_PROCESS_LOCKS)

#if !defined(USE_INLINE_LOCKS)

#if   defined(USE_NT_LOCKS)

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    *lock = CreateMutex(NULL, FALSE, NULL);
    if (*lock == NULL)
    {
        MPIU_Error_printf("error in mutex_init: %d\n", GetLastError());
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
    DWORD dwRetVal;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    /*printf("nt lock %x\n", lock);fflush(stdout);*/
    dwRetVal = WaitForSingleObject(*lock, INFINITE);
    if (dwRetVal != WAIT_OBJECT_0)
    {
        if (dwRetVal == WAIT_FAILED)
            MPIU_Error_printf("error in mutex_lock: %s\n", strerror(GetLastError()));
        else
            MPIU_Error_printf("error in mutex_lock: %d\n", GetLastError());
    }
    /*printf("lock: Handle = %u\n", (unsigned long)*lock);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    if (!ReleaseMutex(*lock))
    {
        MPIU_Error_printf("error in mutex_unlock: %d\n", GetLastError());
        MPIU_Error_printf("Handle = %p\n", *lock);
    }
    /*printf("unlock: Handle = %u\n", (unsigned long)*lock);*/
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    /*printf("Free_lock: Handle = %u\n", (unsigned long)*lock);*/
    CloseHandle(*lock);
    *lock = NULL;
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}
/* End of defined(USE_NT_LOCKS) */

#elif defined(USE_SUN_MUTEX)

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
    /* should be called by one process only */
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    memset(lock, 0, sizeof(MPIDU_Process_lock_t));
    err = mutex_init(lock,USYNC_PROCESS,(void*)0);
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_init: %s\n", err );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    err = mutex_lock( lock );
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_lock: %s\n", err );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    err = _mutex_unlock( lock );
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_unlock: %s\n", err );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    err = mutex_destroy( lock );
    if ( err != 0 ) 
	MPIU_Error_printf( "error in mutex_destroy: %s\n", err );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}

/* End of defined(USE_SUN_MUTEX) */
#elif defined(USE_PTHREAD_LOCKS)

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
    /* should be called by one process only */
    int err;
    pthread_mutexattr_t attr;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
#ifdef HAVE_PTHREAD_MUTEXATTR_INIT
    err = pthread_mutexattr_init(&attr);
    if (err != 0)
      MPIU_Error_printf("error in pthread_mutexattr_init: %s\n", strerror(err));
#endif
#ifdef HAVE_PTHREAD_MUTEXATTR_SETPSHARED
    err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (err != 0)
      MPIU_Error_printf("error in pthread_mutexattr_setpshared: %s\n", strerror(err));

    err = pthread_mutex_init( lock, &attr );
#else
    err = pthread_mutex_init( lock, NULL );
#endif
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_init: %s\n", strerror(err) );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    err = pthread_mutex_lock( lock );
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_lock: %s\n", strerror(err) );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    err = pthread_mutex_unlock( lock );
    if ( err != 0 ) 
        MPIU_Error_printf( "error in mutex_unlock: %s\n", strerror(err) );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
    int err;
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    err = pthread_mutex_destroy( lock );
    if ( err != 0 ) 
	MPIU_Error_printf( "error in mutex_destroy: %s\n", strerror(err) );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}

/* USE_PTHREAD_LOCKS */
#elif defined(USE_SPARC_ASM_LOCKS)
/* Use inline templates to use assembly language to implement the locks.
   We'd like to simply include the asm directly into the routines where 
   needed, but that requires adding the inline template file to *every*
   compile *command line* of any file that might use any of these routines.
*/
#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_init( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
    MPIDUi_Process_lock_init(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_INIT);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK);
    MPIDUi_Process_lock(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_unlock
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_unlock( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_UNLOCK);
    MPIDUi_Process_unlock(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_UNLOCK);
}

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_free
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void MPIDU_Process_lock_free( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
    MPIDUi_Process_lock_free( lock );
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_FREE);
}

/* USE_SPARC_ASM_LOCKS */
#endif /* Case on lock type */

#undef FUNCNAME
#define FUNCNAME MPIDU_Process_lock_busy_wait
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
/*@
   MPIDU_Process_lock_busy_wait - 

   Parameters:
+  MPIDU_Process_lock_t *lock

   Notes:
@*/
void MPIDU_Process_lock_busy_wait( MPIDU_Process_lock_t *lock )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDI_FUNC_ENTER(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
    MPIDU_Process_lock(lock);
    MPIDU_Process_unlock(lock);
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDU_PROCESS_LOCK_BUSY_WAIT);
}

#endif /* #ifndef USE_BUSY_LOCKS */
#endif /* defined(USE_INLINE_LOCKS) */
