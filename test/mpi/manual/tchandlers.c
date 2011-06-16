/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2008 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
/*
   Define _ISOC99_SOURCE to get snprintf() prototype visible in <stdio.h>
   when it is compiled with --enable-stricttest.
*/
#define _ISOC99_SOURCE

#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "connectstuff.h"

static char sFnameToDelete[PATH_MAX];
static int  sWatchdogTimeout = -1;
static size_t sWatchdogStrokeCount = 0;

static void * threadLooper( void * x ) {
    int runTime = 0;
    size_t lastStrokeSeen = sWatchdogStrokeCount;
    /* allow to run for up to 2 minutes */
    while( runTime < sWatchdogTimeout ) {
        safeSleep( 5 );
        runTime += 5;
        if( lastStrokeSeen == sWatchdogStrokeCount ) {
            msg( "Watchdog not stroked for a while, printing stack: \n" );
            printStackTrace();
        }
        lastStrokeSeen = sWatchdogStrokeCount;
    }
    msg( "Watchdog about to abort with a timeout after %d\n", runTime );
    _exit( 20 );
    return NULL;
}


static void term_handler( int sig ) {
    msg( "removing file: %s\n", sFnameToDelete );
    unlink( sFnameToDelete );
}

static void segv_handler( int sig ) {
    msg( "SEGV detected!\n" );
    printStackTrace();
    _exit( 10 );
}

void indicateConnectSucceeded( void ) {
    char fnameToCreate[PATH_MAX];
    FILE * fp;
    snprintf( fnameToCreate, PATH_MAX, "%s.done", sFnameToDelete );
    msg( "Creating file: %s\n", fnameToCreate );
    fp = fopen( fnameToCreate, "wt" );
    if( fp != NULL ) {
        fclose( fp );
    }
}

void startWatchdog( int seconds ) {
    pthread_t theThread;
    sWatchdogTimeout = seconds;
    msg( "Starting watchdog - timeout in %d\n", seconds );
    pthread_create( &theThread, NULL, threadLooper, NULL );
}

void strokeWatchdog() {
    sWatchdogStrokeCount++;
}

void installSegvHandler( void ) {
    struct sigaction new_action;
    new_action.sa_handler = segv_handler;
    sigemptyset( &new_action.sa_mask );
    new_action.sa_flags = 0;
    sigaction( SIGSEGV, &new_action, NULL );
    msg( "Installed SEGV handler\n" );
}

void installExitHandler( const char * fname ) {
    /* Install signal handler */
    struct sigaction new_action;
    if( strlen( fname ) > PATH_MAX ) {
        msg( "Fname: <%s> too long - aborting", fname );
        _exit( 12 );
    }
    strncpy( sFnameToDelete, fname, PATH_MAX );
    new_action.sa_handler = term_handler;
    sigemptyset( &new_action.sa_mask );
    new_action.sa_flags = 0;
    sigaction( SIGINT, &new_action, NULL );
    sigaction( SIGTERM, &new_action, NULL );
    msg( "Installed signal handlers\n" );
}
